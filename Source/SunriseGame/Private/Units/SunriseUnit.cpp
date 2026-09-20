// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/SunriseUnit.h"

#include "AIController.h"
#include "Abilities/SunriseDeathAbility.h"
#include "Abilities/SunriseUnitOrderExecutionAbility.h"
#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "Components/DecalComponent.h"
#include "Components/ModularPawnExtensionComponent.h"
#include "Components/SphereComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "NPC_Optimizator/Public/OptimizationComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "Pawn/UserFacingModularPawnDefinition.h"
#include "Player/SunrisePlayerController.h"
#include "Teams/Components/ModularTeamActorComponent.h"
#include "TimerManager.h"
#include "Units/AI/SunriseUnitAIController.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Vitality/Attributes/SunriseCombatSet.h"
#include "Vitality/Attributes/SunriseHealthSet.h"
#include "Vitality/Attributes/SunriseMovementSet.h"
#include "Vitality/VitalityComponent.h"
#include "Weapons/Effects/SunriseWeaponEffects.h"
#include "Weapons/SunriseWeapon.h"

namespace
{
	const FName DeathUnitsStorageTag(TEXT("DEATH_UNITS_STORAGE"));

	ATargetPoint* FindDeathUnitsStorage(UWorld* World)
	{
		for (TActorIterator<ATargetPoint> It(World); It; ++It)
		{
			if (It->ActorHasTag(DeathUnitsStorageTag))
			{
				return *It;
			}
		}
		return nullptr;
	}
} // namespace

ASunriseUnit::ASunriseUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ASunriseUnitAIController::StaticClass();
	bReplicates = true;

	PawnExtensionComponent = CreateDefaultSubobject<UModularPawnExtensionComponent>("ExtensionComponent");
	//OptimizationProxy = CreateDefaultSubobject<UOptimizationProxyComponent>("OptimizationProxy");
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
		if (!AbilitySystemComponent->FindAbilitySpecFromClass(USunriseUnitOrderExecutionAbility::StaticClass()))
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(USunriseUnitOrderExecutionAbility::StaticClass(), 1, INDEX_NONE, this));
		}
	}
	VitalityComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	EquipDefaultWeaponForRole();

	if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
	{
		UnitManager->RegisterUnit(this);
	}
	OnHealthChanged.Broadcast(this, 1.0f);
}

void ASunriseUnit::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
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
	if (!HasAuthority() || !IsAlive() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	const float Before = GetHealth();
	ASunriseUnit* SourceUnit = Cast<ASunriseUnit>(DamageCauser);
	ApplyHealthGameplayEffect(SourceUnit, USunriseDamageEffect::StaticClass(), USunriseDamageEffect::GetMagnitudeDataName(), -DamageAmount);
	return Before - GetHealth();
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
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController()))
	{
		AI->IssueMoveOrder(Destination, true);
	}
}

void ASunriseUnit::IssueTargetOrder_Implementation(AActor* InTargetActor)
{
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController()))
	{
		AI->IssueTargetOrder(Cast<ASunriseUnit>(InTargetActor), true);
	}
}

void ASunriseUnit::StopOrder_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController()))
	{
		AI->StopOrders();
	}
	StopMoving();
}

bool ASunriseUnit::IssueAutonomousMoveOrder(const FVector& Destination)
{
	ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController());
	return AI && AI->IssueMoveOrder(Destination, false);
}

bool ASunriseUnit::HasActivePlayerOrder() const
{
	const ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController());
	return AI && AI->HasActivePlayerOrder();
}

FVector ASunriseUnit::GetMovementGoal() const
{
	const ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController());
	return AI ? AI->GetMovementGoal() : GetActorLocation();
}

void ASunriseUnit::SetAIOrderPresentation(ASunriseUnit* Target, ESunriseOrderState State)
{
	if (HasAuthority())
	{
		ActionTarget = IsAlive() ? Target : nullptr;
		OrderState = IsAlive() ? State : ESunriseOrderState::Dead;
	}
}

void ASunriseUnit::StopMoving()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	BP_StopAnimation();
}

bool ASunriseUnit::ApplyFocusTarget(ASunriseUnit* Target, float Duration)
{
	ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController());
	return AI && AI->ApplyFocusTarget(Target, Duration);
}

void ASunriseUnit::SetExternalInteractionActive(bool bActive)
{
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController()))
	{
		AI->SetExternalInteractionActive(bActive);
	}
}

bool ASunriseUnit::IsExternalInteractionActive() const
{
	const ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController());
	return AI && AI->IsExternalInteractionActive();
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
	if (IsValid(Interactor))
	{
		if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(Interactor->GetController()))
		{
			if (AI->IssueTargetOrder(this, false))
			{
				BP_InteractionBehavior(Interactor);
			}
		}
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
	if (!HasAuthority() || !CanTargetWithWeapon(Target) || Damage <= 0.0f)
	{
		return;
	}
	Target->ApplyHealthGameplayEffect(this, USunriseDamageEffect::StaticClass(), USunriseDamageEffect::GetMagnitudeDataName(), -Damage);
}

void ASunriseUnit::DealWeaponHealing(ASunriseUnit* Target, float Healing)
{
	if (!HasAuthority() || !IsValid(Target) || !Target->IsAlive() || Healing <= 0.0f || Target->GetTeamId() != GetTeamId())
	{
		return;
	}
	Target->ApplyHealthGameplayEffect(this, USunriseHealingEffect::StaticClass(), USunriseHealingEffect::GetMagnitudeDataName(), Healing);
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
		if (AAIController* AI = Cast<AAIController>(GetController()); HasAuthority() && AI)
		{
			if (UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(AI->GetPathFollowingComponent()))
			{
				Crowd->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
			}
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->RestartLogic();
			}
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

void ASunriseUnit::Die(AController* KillerController, AActor* DamageCauser)
{
	if (OrderState == ESunriseOrderState::Dead)
	{
		return;
	}
	OrderState = ESunriseOrderState::Dead;
	if (HasAuthority())
	{
		AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetHealthAttribute(), 0.0f);
	}
	ActionTarget = nullptr;
	bSelected = false;
	SelectionDecal->SetVisibility(false);
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(GetController()))
	{
		AI->StopOrders();
	}
	StopMoving();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	GetCharacterMovement()->DisableMovement();
	if (HasAuthority())
	{
		ControllableComponent->SetPlayerControllable(false);
		if (AAIController* AI = Cast<AAIController>(GetController()); AI)
		{
			if (UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(AI->GetPathFollowingComponent()))
			{
				Crowd->SetCrowdSimulationState(ECrowdSimulationState::Disabled);
			}
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->StopLogic(TEXT("Pawn death"));
			}
		}
	}
	OnDied.Broadcast(this);
	BP_UnitDied();
	if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
	{
		UnitManager->NotifyUnitDied(this);
	}
	if (HasAuthority() && GetWorld())
	{
		ATargetPoint* Storage = FindDeathUnitsStorage(GetWorld());
		if (!Storage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Dead unit %s cannot be moved: no ATargetPoint tagged DEATH_UNITS_STORAGE"), *GetName());
			return;
		}
		const TWeakObjectPtr<ATargetPoint> WeakStorage = Storage;
		// Respawn ability captures the death transform synchronously after this notification.
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
			[this, WeakStorage]()
			{
				if (!IsAlive() && WeakStorage.IsValid())
				{
					SetActorLocation(WeakStorage->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
					ForceNetUpdate();
				}
			}));
	}
}
