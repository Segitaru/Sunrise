#include "Environment/PlaceableActor.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vitality/Attributes/SunriseCombatSet.h"
#include "Vitality/Attributes/SunriseHealthSet.h"
#include "Weapons/Effects/SunriseWeaponEffects.h"

APlaceableActor::APlaceableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	PlacementComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Placement"));
	PlacementComponent->SetupAttachment(SceneRoot);
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	HealthSet = CreateDefaultSubobject<USunriseHealthSet>(TEXT("HealthAttributes"));
	CombatSet = CreateDefaultSubobject<USunriseCombatSet>(TEXT("CombatAttributes"));
	VitalityComponent = CreateDefaultSubobject<UVitalityComponent>(TEXT("Vitality"));
	VitalityComponent->OnVitalityStateChanged.AddDynamic(this, &ThisClass::HandleVitalityStateChanged);
}

void APlaceableActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetMaxHealthAttribute(), FMath::Max(1.0f, InitialMaxHealth));
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetHealthAttribute(), FMath::Max(1.0f, InitialMaxHealth));
	VitalityComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	if (AActor* Source = GetInstigator())
	{
		if (IIControllableEntity* Controllable = Cast<IIControllableEntity>(Source))
		{
			SetControllingAgent(Controllable->GetControllingAgent());
		}
		if (IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(Source))
		{
			SetGenericTeamId(TeamAgent->GetGenericTeamId());
		}
	}
}

void APlaceableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlaceableActor, TeamId);
	DOREPLIFETIME(APlaceableActor, ControllingAgentActor);
}


float APlaceableActor::TakeDamage(float DamageAmount, const FDamageEvent&, AController*, AActor* DamageCauser)
{
	if (!HasAuthority() || !IsAlive() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || !AbilitySystemComponent)
	{
		return 0.0f;
	}
	const float Before = GetHealth();
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(DamageCauser);
	FGameplayEffectSpecHandle Handle = AbilitySystemComponent->MakeOutgoingSpec(USunriseDamageEffect::StaticClass(), 1.0f, Context);
	if (FGameplayEffectSpec* Spec = Handle.Data.Get())
	{
		Spec->SetSetByCallerMagnitude(USunriseDamageEffect::GetMagnitudeDataName(), -DamageAmount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
	return Before - GetHealth();
}

float APlaceableActor::GetHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

float APlaceableActor::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : 0.0f;
}

bool APlaceableActor::IsAlive() const
{
	return VitalityComponent && VitalityComponent->IsHealthy();
}
void APlaceableActor::HandleVitalityStateChanged(AActor*, EVitalityState OldState, EVitalityState NewState)
{
	if (NewState == EVitalityState::Dying && HasAuthority())
	{
		VitalityComponent->FinishDeath();
	}
	BP_VitalityStateChanged(OldState, NewState);
}

void APlaceableActor::OnRep_TeamId(FGenericTeamId OldTeamId)
{
	IModularTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, OldTeamId, TeamId);
}

void APlaceableActor::OnRep_ControllingAgent(AActor* OldAgentActor)
{
	TScriptInterface<IIControllableEntity> OldAgent;
	OldAgent.SetObject(OldAgentActor);
	OldAgent.SetInterface(Cast<IIControllableEntity>(OldAgentActor));
	TScriptInterface<IIControllableEntity> Self;
	Self.SetObject(this);
	Self.SetInterface(this);
	IIControllableEntity::ConditionalBroadcastControllingAgentChange(Self, OldAgent, GetControllingAgent());
}
TScriptInterface<IIControllableEntity> APlaceableActor::GetControllingAgent()
{
	TScriptInterface<IIControllableEntity> Result;
	Result.SetObject(ControllingAgentActor);
	Result.SetInterface(Cast<IIControllableEntity>(ControllingAgentActor));
	return Result;
}

void APlaceableActor::SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent)
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

void APlaceableActor::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	if (!HasAuthority() || TeamId == NewTeamId)
	{
		return;
	}
	const FGenericTeamId OldTeam = TeamId;
	TeamId = NewTeamId;
	IModularTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, OldTeam, TeamId);
}

void APlaceableActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetOwnedGameplayTags(TagContainer);
	}
}

bool APlaceableActor::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TagToCheck);
}

bool APlaceableActor::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasAllMatchingGameplayTags(TagContainer);
}

bool APlaceableActor::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasAnyMatchingGameplayTags(TagContainer);
}
