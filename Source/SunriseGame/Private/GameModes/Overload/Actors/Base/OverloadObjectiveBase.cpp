// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Actors/Base/OverloadObjectiveBase.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TFTeamActorComponent.h"
#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"
#include "GameModes/Overload/Effects/OverloadEffects.h"
#include "Net/UnrealNetwork.h"

AOverloadObjectiveBase::AOverloadObjectiveBase()
{
	bReplicates = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	LegacyAttributes = CreateDefaultSubobject<UOverloadAttributeSet>(TEXT("ObjectiveAttributes"));
	IntegrityAttributes = CreateDefaultSubobject<UOverloadIntegritySet>(TEXT("IntegrityAttributes"));
	DefenseAttributes = CreateDefaultSubobject<UOverloadDefenseSet>(TEXT("DefenseAttributes"));
	HackAttributes = CreateDefaultSubobject<UOverloadHackSet>(TEXT("HackAttributes"));
	EnergyAttributes = CreateDefaultSubobject<UOverloadEnergySet>(TEXT("EnergyAttributes"));
	TeamComponent = CreateDefaultSubobject<UTFTeamActorComponent>(TEXT("Team"));
}

void AOverloadObjectiveBase::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
}

void AOverloadObjectiveBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOverloadObjectiveBase, OriginalTeamId);
}
UAbilitySystemComponent* AOverloadObjectiveBase::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
int32 AOverloadObjectiveBase::GetTeamId() const
{
	return TeamComponent->GetTeamId();
}
int32 AOverloadObjectiveBase::GetOriginalTeamId() const
{
	return OriginalTeamId;
}

void AOverloadObjectiveBase::InitializeTeam(int32 TeamId)
{
	if (!HasAuthority())
	{
		return;
	}
	OriginalTeamId = TeamId;
	TeamComponent->SetTeamId(TeamId);
}

void AOverloadObjectiveBase::ApplyDynamicScaling(float AttackMultiplier, float ArmorMultiplier, float ResistanceMultiplier)
{
	if (ScalingEffectHandle.IsValid())
	{
		AbilitySystem->RemoveActiveGameplayEffect(ScalingEffectHandle);
	}
	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UOverloadObjectiveScalingEffect::StaticClass(), 1.0f, Context);
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(UOverloadObjectiveScalingEffect::AttackName(), FMath::Max(0.05f, AttackMultiplier));
	Spec.Data->SetSetByCallerMagnitude(UOverloadObjectiveScalingEffect::ArmorName(), FMath::Max(0.05f, ArmorMultiplier));
	Spec.Data->SetSetByCallerMagnitude(UOverloadObjectiveScalingEffect::ResistanceName(), FMath::Max(0.05f, ResistanceMultiplier));
	ScalingEffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void AOverloadObjectiveBase::InitializeObjectiveAttributes(
	float MaxIntegrity, float AttackPower, float Armor, float HackResistance, float MaxOverloadEnergy)
{
	AbilitySystem->SetNumericAttributeBase(UOverloadIntegritySet::GetMaxIntegrityAttribute(), FMath::Max(1.0f, MaxIntegrity));
	AbilitySystem->SetNumericAttributeBase(UOverloadIntegritySet::GetIntegrityAttribute(), FMath::Max(1.0f, MaxIntegrity));
	AbilitySystem->SetNumericAttributeBase(UOverloadDefenseSet::GetAttackPowerAttribute(), FMath::Max(0.0f, AttackPower));
	AbilitySystem->SetNumericAttributeBase(UOverloadDefenseSet::GetArmorAttribute(), FMath::Max(0.0f, Armor));
	AbilitySystem->SetNumericAttributeBase(UOverloadHackSet::GetHackResistanceAttribute(), FMath::Max(0.1f, HackResistance));
	AbilitySystem->SetNumericAttributeBase(UOverloadEnergySet::GetMaxOverloadEnergyAttribute(), FMath::Max(1.0f, MaxOverloadEnergy));
	AbilitySystem->SetNumericAttributeBase(UOverloadEnergySet::GetOverloadEnergyAttribute(), 0.0f);
}

float AOverloadObjectiveBase::ApplyInstantEffect(TSubclassOf<UGameplayEffect> EffectClass, FName MagnitudeName, float Magnitude)
{
	if (!EffectClass || FMath::IsNearlyZero(Magnitude))
	{
		return 0.0f;
	}
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(EffectClass, 1.0f, AbilitySystem->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return 0.0f;
	}
	Spec.Data->SetSetByCallerMagnitude(MagnitudeName, Magnitude);
	AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return Magnitude;
}
