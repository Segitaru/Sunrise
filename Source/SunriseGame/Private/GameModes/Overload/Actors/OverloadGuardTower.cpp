// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Actors/OverloadGuardTower.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameModes/Overload/Abilities/OverloadTowerAttackAbility.h"
#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"
#include "GameModes/Overload/Components/OverloadCaptureComponent.h"
#include "GameModes/Overload/Components/OverloadTowerDefenseComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/SunriseUnit.h"
#include "Weapons/Effects/SunriseWeaponEffects.h"

AOverloadGuardTower::AOverloadGuardTower()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TowerDummyMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (TowerDummyMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(TowerDummyMesh.Object);
	}
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	VisualMesh->SetRelativeScale3D(FVector(1.25f, 1.25f, 3.0f));

	TerminalPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TerminalPoint"));
	TerminalPoint->SetupAttachment(SceneRoot);
	// Tower local X follows the spline tangent; local Y keeps the terminal equally distant along the lane for both teams.
	TerminalPoint->SetRelativeLocation(FVector(0.0f, 180.0f, 0.0f));
	TerminalVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalVisualMesh"));
	TerminalVisualMesh->SetupAttachment(TerminalPoint);
	TerminalVisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
	TerminalVisualMesh->SetRelativeScale3D(FVector(0.35f, 0.55f, 1.1f));
	if (TowerDummyMesh.Succeeded())
	{
		TerminalVisualMesh->SetStaticMesh(TowerDummyMesh.Object);
	}
	CaptureComponent = CreateDefaultSubobject<UOverloadCaptureComponent>(TEXT("CaptureTerminal"));
	DefenseComponent = CreateDefaultSubobject<UOverloadTowerDefenseComponent>(TEXT("TowerDefense"));
	TierOneStats = {350.0f, 24.0f, 10.0f, 1.0f};
}

void AOverloadGuardTower::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		AttackAbilityHandle = AbilitySystem->GiveAbility(FGameplayAbilitySpec(UOverloadTowerAttackAbility::StaticClass(), 1));
	}
}

void AOverloadGuardTower::InitializeTower(int32 TeamId, int32 InTierIndex, AOverloadLaneSpline* InLane)
{
	InitializeTeam(TeamId);
	TierIndex = FMath::Clamp(InTierIndex, 1, 5);
	Lane = InLane;
	const float TierMultiplier = 1.0f + (TierIndex - 1) * PerTierStatGrowth;
	InitializeObjectiveAttributes(TierOneStats.MaxIntegrity * TierMultiplier, TierOneStats.AttackPower * TierMultiplier,
		TierOneStats.Armor * TierMultiplier, TierOneStats.HackResistance * TierMultiplier);
	CaptureComponent->OnCaptured.AddDynamic(this, &AOverloadGuardTower::HandleCaptured);
}

void AOverloadGuardTower::ApplyBalanceMultipliers(float DefenderMultiplier, float LeaderWeakeningMultiplier)
{
	const float Combined = FMath::Max(0.1f, DefenderMultiplier * LeaderWeakeningMultiplier);
	ApplyDynamicScaling(Combined, Combined, Combined);
}

void AOverloadGuardTower::TryTowerAttack(ASunriseUnit* Target)
{
	if (!HasAuthority() || !Target || !Target->IsAlive() || Target->GetTeamId() == GetTeamId())
	{
		return;
	}
	PendingAttackTarget = Target;
	AbilitySystem->TryActivateAbility(AttackAbilityHandle);
}

void AOverloadGuardTower::ExecuteTowerAttack()
{
	ASunriseUnit* Target = PendingAttackTarget;
	if (!Target || !Target->IsAlive() || Target->GetTeamId() == GetTeamId())
	{
		return;
	}
	FGameplayEffectSpecHandle Spec =
		AbilitySystem->MakeOutgoingSpec(USunriseDamageEffect::StaticClass(), 1.0f, AbilitySystem->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(USunriseDamageEffect::GetMagnitudeDataName(), -DefenseAttributes->GetAttackPower());
	Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	BP_TowerAttack(Target);
}

bool AOverloadGuardTower::CanBeHackedByTeam_Implementation(int32 TeamId) const
{
	return TeamId >= 0 && TeamId != GetTeamId();
}

FVector AOverloadGuardTower::GetHackLocation_Implementation() const
{
	return TerminalPoint->GetComponentLocation();
}
void AOverloadGuardTower::RegisterHacker_Implementation(ASunriseUnit* Unit)
{
	CaptureComponent->RegisterHacker(Unit);
}
void AOverloadGuardTower::UnregisterHacker_Implementation(ASunriseUnit* Unit)
{
	CaptureComponent->UnregisterHacker(Unit);
}

void AOverloadGuardTower::HandleCaptured(int32 PreviousTeamId, int32 NewTeamId)
{
	OnTowerCaptured.Broadcast(this, PreviousTeamId, NewTeamId);
}
