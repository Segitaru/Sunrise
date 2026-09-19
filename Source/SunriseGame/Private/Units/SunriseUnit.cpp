// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/SunriseUnit.h"

#include <BehaviorTree/BlackboardComponent.h>

#include "AIController.h"
#include "Abilities/SunriseDeathAbility.h"
#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "Components/DecalComponent.h"
#include "Components/ModularPawnExtensionComponent.h"
#include "Components/SphereComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "Pawn/UserFacingModularPawnDefinition.h"
#include "Player/SunrisePlayerController.h"
#include "Teams/Components/ModularTeamActorComponent.h"
#include "Units/AI/SunriseUnitAIController.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Vitality/Attributes/SunriseCombatSet.h"
#include "Vitality/Attributes/SunriseHealthSet.h"
#include "Vitality/Attributes/SunriseMovementSet.h"
#include "Vitality/VitalityComponent.h"
#include "Weapons/Effects/SunriseWeaponEffects.h"
#include "Weapons/SunriseWeapon.h"

ASunriseUnit::ASunriseUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ASunriseUnitAIController::StaticClass();
	bReplicates = true;

	PawnExtensionComponent = CreateDefaultSubobject<UModularPawnExtensionComponent>("ExtensionComponent");

	AbilitySystemComponent = CreateDefaultSubobject<UModularAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USunriseHealthSet::GetHealthAttribute())
		.AddUObject(this, &ASunriseUnit::HandleHealthAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USunriseMovementSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ASunriseUnit::HandleMoveSpeedAttributeChanged);

	HealthSet = CreateDefaultSubobject<USunriseHealthSet>(TEXT("HealthAttributes"));
	CombatSet = CreateDefaultSubobject<USunriseCombatSet>(TEXT("CombatAttributes"));
	MovementSet = CreateDefaultSubobject<USunriseMovementSet>(TEXT("MovementAttributes"));

	VitalityComponent = CreateDefaultSubobject<UVitalityComponent>(TEXT("Vitality"));
	VitalityComponent->OnVitalityStateChanged.AddDynamic(this, &ThisClass::HandleVitalityStateChanged);

	ControllableComponent = CreateDefaultSubobject<UControllableComponent>(TEXT("Controllable"));
	TeamComponent = CreateDefaultSubobject<UModularTeamActorComponent>(TEXT("Team"));
	TeamComponent->OnTeamChanged.AddDynamic(this, &ThisClass::HandleTeamChanged);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(RootComponent);
	InteractionRange->SetSphereRadius(100.0f);
	InteractionRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecal"));
	SelectionDecal->SetupAttachment(RootComponent);
	SelectionDecal->DecalSize = FVector(16.0f, 64.0f, 64.0f);
	SelectionDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	SelectionDecal->SetVisibility(false);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GravityScale = 1.5f;
	Movement->MaxAcceleration = 1000.0f;
	Movement->BrakingFrictionFactor = 1.0f;
	Movement->BrakingDecelerationWalking = 1000.0f;
	Movement->bUseFlatBaseForFloorChecks = true;
	Movement->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	Movement->bOrientRotationToMovement = true;
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 120.0f;
	Movement->AvoidanceWeight = 0.5f;
	Movement->SetFixedBrakingDistance(150.0f);
	Movement->SetFixedBrakingDistance(true);
}

void ASunriseUnit::BeginPlay()
{
	InitialSpawnTransform = GetActorTransform();
	Super::BeginPlay();

	if (bUseRoleDefaults)
	{
		ApplyRoleDefaults();
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	// Heroes expose effect durations to HUD consumers even when owned by GameState.
	if (IsHero())
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	}
	InitializeAbilityAttributes();
	if (HasAuthority())
	{
		UClass* LifecycleClass = DeathAbilityClass ? DeathAbilityClass.Get()
								 : IsHero()		   ? USunriseRespawnAbility::StaticClass()
												   : USunriseDeathAbility::StaticClass();
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(LifecycleClass, 1, INDEX_NONE, this));
	}
	VitalityComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	EquipDefaultWeaponForRole();

	DecisionTimeRemaining = FMath::FRandRange(0.0f, DecisionInterval);

	if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
	{
		UnitManager->RegisterUnit(this);
	}
	OnHealthChanged.Broadcast(this, 1.0f);
}

void ASunriseUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !IsAlive())
	{
		return;
	}

	DecisionTimeRemaining -= DeltaSeconds;
	ActionTimeRemaining = FMath::Max(0.0f, ActionTimeRemaining - DeltaSeconds);
	if (bExternalInteractionActive)
	{
		return;
	}
	if (const ASunriseUnitAIController* UnitController = Cast<ASunriseUnitAIController>(Controller);
		UnitController && UnitController->IsStateTreeDrivingDecisions())
	{
		return;
	}
	if (DecisionTimeRemaining <= 0.0f)
	{
		DecisionTimeRemaining = FMath::Max(0.1f, DecisionInterval);
		UpdateOrder(DecisionInterval);
	}
}

void ASunriseUnit::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	AIController = Cast<AAIController>(Controller);
	if (AIController)
	{
		if (UPathFollowingComponent* Path = AIController->GetPathFollowingComponent())
		{
			Path->OnRequestFinished.RemoveAll(this);
			Path->OnRequestFinished.AddUObject(this, &ASunriseUnit::OnMoveFinished);
		}
	}
	PawnExtensionComponent->HandleControllerChanged();
}

void ASunriseUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASunriseUnit, RespawnReadyTime);
	DOREPLIFETIME(ASunriseUnit, ActionTarget);
	DOREPLIFETIME(ASunriseUnit, ControllingAgentActor);
}

float ASunriseUnit::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	const float Before = GetHealth();
	ASunriseUnit* SourceUnit = Cast<ASunriseUnit>(DamageCauser);
	ApplyHealthGameplayEffect(SourceUnit, USunriseDamageEffect::StaticClass(), USunriseDamageEffect::GetMagnitudeDataName(), -DamageAmount);
	const float Applied = Before - GetHealth();

	if (!bPlayerOrderActive && !ActionTarget && DamageCauser)
	{
		ASunriseUnit* Attacker = SourceUnit;
		if (IsValidActionTarget(Attacker))
		{
			ActionTarget = Attacker;
			OrderState = ESunriseOrderState::Attacking;
		}
	}
	return Applied;
}

UAbilitySystemComponent* ASunriseUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASunriseUnit::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
}

void ASunriseUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

bool ASunriseUnit::CanBeSelectedBy_Implementation(const APlayerController* InController) const
{
	const ASunrisePlayerController* SunriseController = Cast<ASunrisePlayerController>(InController);
	const UControllableEntitiesManager* Manager =
		SunriseController ? UControllableEntitiesManager::FindControllableEntitiesManager(SunriseController) : nullptr;
	return IsAlive() && Manager && Manager->CanControlEntity(this);
}

void ASunriseUnit::SetSunriseSelected_Implementation(bool bInSelected)
{
	bSelected = bInSelected && IsAlive();
	SelectionDecal->SetVisibility(bSelected);
	GetMesh()->SetRenderCustomDepth(bSelected);
	if (bSelected)
	{
		BP_UnitSelected();
	}
	else
	{
		BP_UnitDeselected();
	}
}

void ASunriseUnit::IssueMoveOrder_Implementation(const FVector& Destination)
{
	IssueMoveOrderInternal(Destination, true);
}

void ASunriseUnit::IssueTargetOrder_Implementation(AActor* TargetActor)
{
	IssueTargetOrderInternal(Cast<ASunriseUnit>(TargetActor), true);
}

void ASunriseUnit::StopOrder_Implementation()
{
	ActionTarget = nullptr;
	bForcedTarget = false;
	SetPlayerOrderActive(false);
	OrderState = IsAlive() ? ESunriseOrderState::Idle : ESunriseOrderState::Dead;
	StopMoving();
}

bool ASunriseUnit::IssueAutonomousMoveOrder(const FVector& Destination)
{
	return IssueMoveOrderInternal(Destination, false);
}

void ASunriseUnit::StopMoving()
{
	if (AIController)
	{
		AIController->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	BP_StopAnimation();
}

bool ASunriseUnit::ApplyFocusTarget(ASunriseUnit* Target, float Duration)
{
	if (!HasAuthority() || bPlayerOrderActive || !IsValidActionTarget(Target) || Duration <= 0.0f || !GetWorld())
	{
		return false;
	}
	FocusTarget = Target;
	FocusTargetExpiryTime = GetWorld()->GetTimeSeconds() + Duration;
	ActionTarget = Target;
	bForcedTarget = false;
	OrderState = ESunriseOrderState::Attacking;
	return true;
}

void ASunriseUnit::SetExternalInteractionActive(bool bActive)
{
	if (bExternalInteractionActive == bActive)
	{
		return;
	}
	if (bActive && bPlayerOrderActive)
	{
		return;
	}
	bExternalInteractionActive = bActive;
	if (bExternalInteractionActive)
	{
		ActionTarget = nullptr;
		bForcedTarget = false;
		OrderState = ESunriseOrderState::Idle;
		StopMoving();
	}
}

void ASunriseUnit::UnitSelected()
{
	ISunriseSelectable::Execute_SetSunriseSelected(this, true);
}

void ASunriseUnit::UnitDeselected()
{
	ISunriseSelectable::Execute_SetSunriseSelected(this, false);
}

void ASunriseUnit::Interact(ASunriseUnit* Interactor)
{
	if (Interactor)
	{
		Interactor->IssueTargetOrderInternal(this, false);
		BP_InteractionBehavior(Interactor);
	}
}

int32 ASunriseUnit::GetTeamId() const
{
	return TeamComponent->GetTeamId();
}

void ASunriseUnit::SetTeamId(int32 NewTeamId)
{
	TeamComponent->SetTeamId(NewTeamId);
}

void ASunriseUnit::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamComponent->SetGenericTeamId(NewTeamId);
}

FGenericTeamId ASunriseUnit::GetGenericTeamId() const
{
	return TeamComponent->GetGenericTeamId();
}

FOnTeamIndexChangedDelegate* ASunriseUnit::GetOnTeamIndexChangedDelegate()
{
	return TeamComponent->GetOnTeamIndexChangedDelegate();
}

TScriptInterface<IIControllableEntity> ASunriseUnit::GetControllingAgent()
{
	TScriptInterface<IIControllableEntity> Result;
	Result.SetObject(ControllingAgentActor);
	Result.SetInterface(Cast<IIControllableEntity>(ControllingAgentActor));
	return Result;
}

void ASunriseUnit::SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent)
{
	AActor* NewAgentActor = Cast<AActor>(NewAgent.GetObject());
	if (!HasAuthority() || ControllingAgentActor == NewAgentActor)
	{
		return;
	}
	AActor* OldAgentActor = ControllingAgentActor;
	ControllingAgentActor = NewAgentActor;
	OnRep_ControllingAgent(OldAgentActor);
}

bool ASunriseUnit::HasPawnTag(FGameplayTag Tag) const
{
	const UModularPawnData* Data = PawnExtensionComponent->GetPawnData<UModularPawnData>();
	return Data && Data->Specification.HasTag(Tag);
}

FText ASunriseUnit::GetUnitClassDisplayName() const
{
	const UModularPawnData* Data = PawnExtensionComponent ? PawnExtensionComponent->GetPawnData<UModularPawnData>() : nullptr;
	const FSoftObjectPath UIPath = Data ? Data->PawnUIDefinition.ToSoftObjectPath() : FSoftObjectPath();
	if (CachedPawnUIPath != UIPath)
	{
		CachedPawnUIPath = UIPath;
		CachedPawnUIDefinition = Data ? Data->PawnUIDefinition.LoadSynchronous() : nullptr;
	}
	return CachedPawnUIDefinition ? CachedPawnUIDefinition->PawnDisplayedName : FText::GetEmpty();
}

float ASunriseUnit::GetHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

float ASunriseUnit::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : FMath::Max(1.0f, Stats.MaxHealth);
}

void ASunriseUnit::ConfigureControl(TScriptInterface<IIControllableEntity> NewAgent)
{
	if (!HasAuthority())
	{
		return;
	}
	SetControllingAgent(NewAgent);
	ControllableComponent->SetPlayerControllable(!HasPawnTag(SunrisePawnTags::Kind_Creep) && NewAgent.GetInterface() != nullptr);
}

float ASunriseUnit::GetRespawnSeconds() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return RespawnReadyTime >= 0.0f && GameState ? FMath::Max(0.0f, RespawnReadyTime - GameState->GetServerWorldTimeSeconds()) : -1.0f;
}

void ASunriseUnit::SetRespawnReadyTime(float Time)
{
	if (HasAuthority())
	{
		RespawnReadyTime = Time;
		ForceNetUpdate();
	}
}

bool ASunriseUnit::RestoreAfterDeath(const FTransform& Transform)
{
	if (!HasAuthority() || !GetWorld() || IsAlive())
	{
		return false;
	}
	FVector Location = Transform.GetLocation();
	FRotator Rotation = Transform.Rotator();
	SetActorEnableCollision(true);
	if (!GetWorld()->FindTeleportSpot(this, Location, Rotation))
	{
		SetActorEnableCollision(false);
		return false;
	}
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
	VitalityComponent->RestoreVitality();
	ConfigureControl(GetControllingAgent());
	if (AController* Agent = Cast<AController>(GetControllingAgent().GetObject()))
	{
		if (const IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(Agent->PlayerState))
		{
			SetGenericTeamId(TeamAgent->GetGenericTeamId());
		}
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Agent))
		{
			Manager->RegisterControlledEntity(this);
		}
	}
	if (USunriseUnitManagerComponent* Manager = USunriseUnitManagerComponent::Find(this))
	{
		Manager->OnArmyCountChanged.Broadcast(Manager->GetFriendlyAlive(), Manager->GetEnemyAlive());
	}
	ForceNetUpdate();
	return true;
}

bool ASunriseUnit::CanTargetWithWeapon(const ASunriseUnit* Target) const
{
	return Weapon ? Weapon->CanTarget(Target) : false;
}

float ASunriseUnit::GetHealthPercent() const
{
	return GetMaxHealth() > 0.0f ? FMath::Clamp(GetHealth() / GetMaxHealth(), 0.0f, 1.0f) : 0.0f;
}

bool ASunriseUnit::IsAlive() const
{
	return OrderState != ESunriseOrderState::Dead && GetHealth() > 0.0f;
}

float ASunriseUnit::ReceiveHealing(float Amount, ASunriseUnit* Healer)
{
	if (!IsAlive() || Amount <= 0.0f || !Healer || Healer->GetTeamId() != GetTeamId())
	{
		return 0.0f;
	}
	const float Before = GetHealth();
	ApplyHealthGameplayEffect(Healer, USunriseHealingEffect::StaticClass(), USunriseHealingEffect::GetMagnitudeDataName(), Amount);
	return GetHealth() - Before;
}

void ASunriseUnit::ApplyDifficultyScaling(float HealthMultiplier, float PowerMultiplier)
{
	if (!AbilitySystemComponent || !HealthSet || !CombatSet)
	{
		return;
	}
	const float HealthPercent = GetHealthPercent();
	const float NewMaxHealth = FMath::Max(1.0f, HealthSet->GetMaxHealth() * HealthMultiplier);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetMaxHealthAttribute(), NewMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetHealthAttribute(), NewMaxHealth * HealthPercent);
	AbilitySystemComponent->SetNumericAttributeBase(
		USunriseCombatSet::GetAttackPowerAttribute(), FMath::Max(0.0f, CombatSet->GetAttackPower() * PowerMultiplier));
}

void ASunriseUnit::ApplyWeaponAttributes(float Damage, float Range, float Interval)
{
	Stats.Power = FMath::Max(0.0f, Damage);
	Stats.ActionRange = FMath::Max(0.0f, Range);
	Stats.ActionInterval = FMath::Max(0.05f, Interval);
	if (!AbilitySystemComponent)
	{
		return;
	}
	if (!HasAuthority())
	{
		return;
	}
	const float PowerScale = IsHero() ? HeroPowerMultiplier : 1.0f;
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetAttackPowerAttribute(), Stats.Power * PowerScale);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetActionRangeAttribute(), Stats.ActionRange);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetActionIntervalAttribute(), Stats.ActionInterval);
}

float ASunriseUnit::GetAttackPowerAttribute() const
{
	return CombatSet ? CombatSet->GetAttackPower() : Stats.Power;
}

void ASunriseUnit::DealWeaponDamage(ASunriseUnit* Target, float Damage)
{
	if (!IsValidActionTarget(Target) || Damage <= 0.0f)
	{
		return;
	}
	Target->ApplyHealthGameplayEffect(this, USunriseDamageEffect::StaticClass(), USunriseDamageEffect::GetMagnitudeDataName(), -Damage);
}

void ASunriseUnit::DealWeaponHealing(ASunriseUnit* Target, float Healing)
{
	if (!Target || Healing <= 0.0f || Target->GetTeamId() != GetTeamId())
	{
		return;
	}
	Target->ApplyHealthGameplayEffect(this, USunriseHealingEffect::StaticClass(), USunriseHealingEffect::GetMagnitudeDataName(), Healing);
	if (Target->GetHealthPercent() >= 0.999f && ActionTarget == Target)
	{
		ActionTarget = nullptr;
		bForcedTarget = false;
		SetPlayerOrderActive(false);
		OrderState = ESunriseOrderState::Idle;
	}
}

void ASunriseUnit::NotifyWeaponAction(ASunriseUnit* Target, bool bHealing, bool bAreaAction)
{
	(void)bAreaAction;
	BP_CombatAction(Target, bHealing);
}

void ASunriseUnit::ApplyRoleDefaults()
{
	if (HasPawnTag(SunrisePawnTags::Class_Swordsman))
	{
		Stats = {180.0f, 28.0f, 250.0f, 0.9f, 440.0f, 1100.0f};
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Archer))
	{
		Stats = {115.0f, 20.0f, 850.0f, 1.25f, 410.0f, 1450.0f};
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Healer))
	{
		Stats = {115.0f, 12.0f, 525.0f, 1.6f, 420.0f, 1150.0f};
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Mage))
	{
		Stats = {130.0f, 24.0f, 600.0f, 1.1f, 425.0f, 1300.0f};
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Vanguard))
	{
		Stats = {240.0f, 22.0f, 285.0f, 0.95f, 385.0f, 1050.0f};
	}
}

void ASunriseUnit::EquipDefaultWeaponForRole()
{
	if (Weapon)
	{
		Weapon->Uninitialize();
	}

	UClass* WeaponClass = USunriseSwordWeapon::StaticClass();
	if (HasPawnTag(SunrisePawnTags::Class_Archer))
	{
		WeaponClass = USunriseBowWeapon::StaticClass();
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Healer))
	{
		WeaponClass = USunriseDrumsWeapon::StaticClass();
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Mage))
	{
		WeaponClass = USunriseStaffWeapon::StaticClass();
	}
	else if (HasPawnTag(SunrisePawnTags::Class_Vanguard))
	{
		WeaponClass = USunriseSpearShieldWeapon::StaticClass();
	}
	Weapon = NewObject<USunriseWeapon>(this, WeaponClass);
	if (Weapon)
	{
		Weapon->Initialize(this);
	}
}

void ASunriseUnit::InitializeAbilityAttributes(float HealthPercent)
{
	if (!HasAuthority() || !AbilitySystemComponent || !HealthSet || !CombatSet || !MovementSet)
	{
		return;
	}
	Stats.MaxHealth = FMath::Max(1.0f, Stats.MaxHealth);
	const float KindHealthScale = IsHero() ? HeroHealthMultiplier : 1.0f;
	const float KindPowerScale = IsHero() ? HeroPowerMultiplier : 1.0f;
	const float MaxHealth = Stats.MaxHealth * KindHealthScale;
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetAttackPowerAttribute(), Stats.Power * KindPowerScale);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetActionRangeAttribute(), Stats.ActionRange);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetActionIntervalAttribute(), Stats.ActionInterval);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseMovementSet::GetMoveSpeedAttribute(), Stats.MoveSpeed);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseCombatSet::GetAggroRadiusAttribute(), Stats.AggroRadius);
	AbilitySystemComponent->SetNumericAttributeBase(
		USunriseHealthSet::GetHealthAttribute(), MaxHealth * FMath::Clamp(HealthPercent, 0.0f, 1.0f));
	GetCharacterMovement()->MaxWalkSpeed = MovementSet->GetMoveSpeed();
	InteractionRange->SetSphereRadius(CombatSet->GetAggroRadius());
}

float ASunriseUnit::ApplyHealthGameplayEffect(
	ASunriseUnit* Source, TSubclassOf<UGameplayEffect> EffectClass, FName MagnitudeDataName, float SignedMagnitude)
{
	if (!AbilitySystemComponent || !EffectClass || FMath::IsNearlyZero(SignedMagnitude))
	{
		return 0.0f;
	}
	UAbilitySystemComponent* SourceASC = Source && Source->AbilitySystemComponent ? Source->AbilitySystemComponent : AbilitySystemComponent;
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(Source ? static_cast<UObject*>(Source) : this);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
	if (!Spec.IsValid())
	{
		return 0.0f;
	}
	Spec.Data->SetSetByCallerMagnitude(MagnitudeDataName, SignedMagnitude);
	if (SignedMagnitude < 0.0f)
	{
		LastDamageSource = Source;
	}
	const float Before = GetHealth();
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return GetHealth() - Before;
}

void ASunriseUnit::HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(this, GetHealthPercent());
}

void ASunriseUnit::HandleMoveSpeedAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, ChangeData.NewValue);
}

void ASunriseUnit::HandleVitalityStateChanged(AActor*, EVitalityState OldState, EVitalityState NewState)
{
	if (NewState == EVitalityState::Dying || NewState == EVitalityState::Dead)
	{
		Die(LastDamageSource.IsValid() ? LastDamageSource->GetController() : nullptr, LastDamageSource.Get());
	}
	else if (NewState == EVitalityState::Healthy && OldState != EVitalityState::Healthy)
	{
		OrderState = ESunriseOrderState::Idle;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (HasAuthority() && AIController && AIController->GetBrainComponent())
		{
			AIController->GetBrainComponent()->RestartLogic();
		}
		BP_UnitRespawned();
	}
}

void ASunriseUnit::OnRep_ControllingAgent(AActor* OldAgentActor)
{
	TScriptInterface<IIControllableEntity> OldAgent;
	OldAgent.SetObject(OldAgentActor);
	OldAgent.SetInterface(Cast<IIControllableEntity>(OldAgentActor));
	const TScriptInterface<IIControllableEntity> NewAgent = GetControllingAgent();
	TScriptInterface<IIControllableEntity> Self;
	Self.SetObject(this);
	Self.SetInterface(this);
	IIControllableEntity::ConditionalBroadcastControllingAgentChange(Self, OldAgent, NewAgent);
}

void ASunriseUnit::HandleTeamChanged(UObject* TeamAgent, int32 PreviousTeamId, int32 NewTeamId)
{
	if (GetTeamId() != 0 && bSelected)
	{
		ISunriseSelectable::Execute_SetSunriseSelected(this, false);
	}
}

void ASunriseUnit::UpdateOrder(float DeltaSeconds)
{
	if (bPlayerOrderActive && OrderState == ESunriseOrderState::Moving &&
		FVector::Dist2D(GetActorLocation(), CurrentMovementGoal) <= MovementAcceptanceRadius * 2.0f)
	{
		SetPlayerOrderActive(false);
		OrderState = ESunriseOrderState::Idle;
		HandleMoveFinished();
	}
	if (FocusTarget.IsValid() && GetWorld() && GetWorld()->GetTimeSeconds() < FocusTargetExpiryTime && !bPlayerOrderActive &&
		IsValidActionTarget(FocusTarget.Get()))
	{
		ActionTarget = FocusTarget.Get();
	}
	else if (GetWorld() && GetWorld()->GetTimeSeconds() >= FocusTargetExpiryTime)
	{
		FocusTarget.Reset();
	}
	const bool bPlayerEnemyFocus =
		bPlayerOrderActive && ActionTarget && ActionTarget->IsAlive() && !HasPawnTag(SunrisePawnTags::Class_Healer);
	if (ActionTarget && !IsValidActionTarget(ActionTarget) && !bPlayerEnemyFocus)
	{
		ActionTarget = nullptr;
		bForcedTarget = false;
		SetPlayerOrderActive(false);
		OrderState = ESunriseOrderState::Idle;
	}
	if (bPlayerOrderActive && !ActionTarget)
	{
		return;
	}

	if (!ActionTarget && !bPlayerOrderActive && bAutoAcquireTargets && OrderState != ESunriseOrderState::Moving)
	{
		AcquireAutomaticTarget();
	}
	if (!ActionTarget)
	{
		return;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), ActionTarget->GetActorLocation());
	if (Distance > CombatSet->GetActionRange() * 1.15f)
	{
		MoveTowardActor(ActionTarget);
		return;
	}

	if (AIController)
	{
		AIController->StopMovement();
	}
	const FVector Direction = ActionTarget->GetActorLocation() - GetActorLocation();
	if (!Direction.IsNearlyZero())
	{
		SetActorRotation(Direction.Rotation());
	}
	OrderState = HasPawnTag(SunrisePawnTags::Class_Healer) ? ESunriseOrderState::Healing : ESunriseOrderState::Attacking;
	PerformAction(ActionTarget);
}

void ASunriseUnit::AcquireAutomaticTarget()
{
	if (bPlayerOrderActive)
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SunriseAggro), false, this);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps, GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(CombatSet->GetAggroRadius()), Params);

	ASunriseUnit* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASunriseUnit* Candidate = Cast<ASunriseUnit>(Overlap.GetActor());
		if (!IsValidActionTarget(Candidate))
		{
			continue;
		}
		const float Distance = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
		const float Score = HasPawnTag(SunrisePawnTags::Class_Healer) ? Candidate->GetHealthPercent() * 100000000.0f + Distance : Distance;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	if (Best)
	{
		ActionTarget = Best;
		bForcedTarget = false;
		OrderState = HasPawnTag(SunrisePawnTags::Class_Healer) ? ESunriseOrderState::Healing : ESunriseOrderState::Attacking;
	}
}

bool ASunriseUnit::IsValidActionTarget(const ASunriseUnit* Candidate) const
{
	return CanTargetWithWeapon(Candidate);
}

void ASunriseUnit::PerformAction(ASunriseUnit* Target)
{
	if (!IsValidActionTarget(Target))
	{
		return;
	}
	if (Weapon)
	{
		Weapon->TryActivatePrimaryAttack(Target);
	}
}

void ASunriseUnit::MoveTowardActor(ASunriseUnit* Target)
{
	if (AIController && Target)
	{
		const FAIRequestID RequestId =
			AIController->MoveToActor(Target, CombatSet->GetActionRange() * 0.95f, true, true, true, nullptr, true);
		if (RequestId == FAIRequestID::InvalidRequest)
		{
			AIController->MoveToLocation(
				Target->GetActorLocation(), CombatSet->GetActionRange() * 0.95f, true, true, true, true, nullptr, true);
		}
	}
}

void ASunriseUnit::Die(AController* KillerController, AActor* DamageCauser)
{
	if (OrderState == ESunriseOrderState::Dead)
	{
		return;
	}
	OrderState = ESunriseOrderState::Dead;
	SetPlayerOrderActive(false);
	if (HasAuthority())
	{
		AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetHealthAttribute(), 0.0f);
	}
	ActionTarget = nullptr;
	bSelected = false;
	SelectionDecal->SetVisibility(false);
	if (AIController)
	{
		AIController->StopMovement();
	}
	StopMoving();
	FocusTarget.Reset();
	bExternalInteractionActive = false;
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	GetCharacterMovement()->DisableMovement();
	if (HasAuthority())
	{
		ControllableComponent->SetPlayerControllable(false);
		if (AIController && AIController->GetBrainComponent())
		{
			AIController->GetBrainComponent()->StopLogic(TEXT("Pawn death"));
		}
	}
	OnDied.Broadcast(this);
	BP_UnitDied();
	if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
	{
		UnitManager->NotifyUnitDied(this);
	}
}

void ASunriseUnit::HandleMoveFinished()
{
	OnMoveCompleted.Broadcast(this);
}

void ASunriseUnit::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (OrderState == ESunriseOrderState::Moving)
	{
		SetPlayerOrderActive(false);
		OrderState = ESunriseOrderState::Idle;
		HandleMoveFinished();
		UpdateOrder(0.0f);
	}
}

bool ASunriseUnit::IssueMoveOrderInternal(const FVector& Destination, bool bFromPlayer)
{
	if (!AIController)
	{
		return false;
	}

	AIController->GetBlackboardComponent()->SetValueAsVector(
		bFromPlayer ? TEXT("PlayerOrderTargetLocation") : TEXT("TargetLocation"), Destination);
	return true;
}

void ASunriseUnit::IssueTargetOrderInternal(ASunriseUnit* Unit, bool bFromPlayer)
{
	const bool bPlayerEnemyFocus = bFromPlayer && Unit && Unit->IsAlive() && Unit != this && !HasPawnTag(SunrisePawnTags::Class_Healer);
	if (!IsAlive() || (!bFromPlayer && bPlayerOrderActive) || !IsValid(Unit) || Unit == this ||
		(!IsValidActionTarget(Unit) && !bPlayerEnemyFocus))
	{
		return;
	}
	ActionTarget = Unit;
	bForcedTarget = true;
	SetPlayerOrderActive(bFromPlayer);
	OrderState = HasPawnTag(SunrisePawnTags::Class_Healer) ? ESunriseOrderState::Healing : ESunriseOrderState::Attacking;
	UpdateOrder(0.0f);
}

void ASunriseUnit::SetPlayerOrderActive(bool bActive)
{
	if (bPlayerOrderActive == bActive)
	{
		return;
	}
	bPlayerOrderActive = bActive;
	if (ASunriseUnitAIController* UnitController = Cast<ASunriseUnitAIController>(Controller))
	{
		if (bActive)
		{
			UnitController->SuspendDecisionLogicForPlayerOrder();
		}
		else
		{
			UnitController->ResumeDecisionLogicAfterPlayerOrder();
		}
	}
}

void ASunriseUnit::OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	// Retained as a no-op compatibility endpoint for existing template Blueprint assets.
}
