// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "GameModes/Overload/Types/OverloadTeamIds.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "TimerManager.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

UOverloadWaveSpawnerComponent::UOverloadWaveSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UOverloadWaveSpawnerComponent::Initialize(AOverloadLaneSpline* InLane, const TArray<TSoftObjectPtr<UModularPawnData>>& InDefinitions)
{
	Lane = InLane;
	UnitDefinitions = InDefinitions;
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!Lane.IsValid() || UnitDefinitions.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Overload wave spawner %s cannot initialize: lane=%s definitions=%d"), *GetName(),
			*GetNameSafe(Lane.Get()), UnitDefinitions.Num());
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &UOverloadWaveSpawnerComponent::SpawnWave, WaveInterval, true, InitialDelay);
	UE_LOG(LogTemp, Log, TEXT("Overload lane %s scheduled waves: first in %.1fs, interval %.1fs"), *GetNameSafe(Lane.Get()), InitialDelay,
		WaveInterval);
}

void UOverloadWaveSpawnerComponent::ApplyEnemyDifficulty(float CountMultiplier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	EnemyWaveMultiplier = FMath::IsFinite(CountMultiplier) ? FMath::Max(0.5f, CountMultiplier) : 1.0f;
	// EnemyCountMultiplier affects enemy population, never the shared wave cadence.
}

void UOverloadWaveSpawnerComponent::SpawnWave()
{
	if (!GetOwner()->HasAuthority() || !Lane.IsValid() || UnitDefinitions.IsEmpty())
	{
		return;
	}
	PruneTrackedUnits();
	SpawnWaveForTeam(Lane->GetSourceTeamId(), true);
	SpawnWaveForTeam(Lane->GetTargetTeamId(), false);
}

float UOverloadWaveSpawnerComponent::GetSecondsUntilNextWave() const
{
	return GetWorld() && WaveTimer.IsValid() ? GetWorld()->GetTimerManager().GetTimerRemaining(WaveTimer) : -1.0f;
}

int32 UOverloadWaveSpawnerComponent::GetAliveUnitCount(int32 TeamId) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ASunriseUnit>& UnitPtr : SpawnedUnits)
	{
		const ASunriseUnit* Unit = UnitPtr.Get();
		if (Unit && Unit->IsAlive() && Unit->GetTeamId() == TeamId)
		{
			++Count;
		}
	}
	return Count;
}

void UOverloadWaveSpawnerComponent::SpawnWaveForTeam(int32 TeamId, bool bSpawnAtSplineStart)
{
	if (!OverloadTeamIds::IsPlayable(TeamId))
	{
		return;
	}
	const int32 AliveCount = GetAliveUnitCount(TeamId);
	const float CountMultiplier = TeamId == OverloadTeamIds::InitialPlayer ? 1.0f : EnemyWaveMultiplier;
	const int32 WaveSize = FMath::Max(1, FMath::RoundToInt(UnitDefinitions.Num() * CountMultiplier));
	const int32 PopulationLimit = GetMaxAliveUnitsForTeam(TeamId);
	if (AliveCount + WaveSize > PopulationLimit)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Overload lane %s skipped team %d wave: population %d/%d"), *GetNameSafe(Lane.Get()), TeamId,
			AliveCount, PopulationLimit);
		return;
	}
	USplineComponent* Spline = Lane->GetLaneSpline();
	const float SplineLength = Spline->GetSplineLength();
	const float SafeInset = FMath::Min(SpawnInsetDistance, SplineLength * 0.25f);
	const float SpawnDistance = bSpawnAtSplineStart ? SafeInset : SplineLength - SafeInset;
	const FVector Start = Spline->GetLocationAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World);
	const FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World);
	FRotator Facing = (bSpawnAtSplineStart ? Spline->GetDirectionAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World)
										   : -Spline->GetDirectionAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World))
						  .Rotation();
	Facing.Pitch = 0.0f;
	Facing.Roll = 0.0f;
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(this))
	{
		return;
	}
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < WaveSize; ++Index)
	{
		const float LateralOffset = (Index - (WaveSize - 1) * 0.5f) * UnitSpacing;
		FVector Location = Start + Right * LateralOffset;
		FNavLocation Projected;
		if (!Navigation->ProjectPointToNavigation(Location, Projected, FVector(250.0f, 250.0f, 500.0f)))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Overload wave skipped: no navigation for team %d at %s"), TeamId, *Location.ToString());
			continue;
		}
		FHitResult GroundHit;
		const FVector TraceStart = Projected.Location + FVector(0.0f, 0.0f, 100.0f);
		const FVector TraceEnd = Projected.Location - FVector(0.0f, 0.0f, 200.0f);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverloadWaveSpawnGround), false, GetOwner());
		if (!GetWorld()->LineTraceSingleByObjectType(
				GroundHit, TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams) ||
			!GroundHit.GetComponent() || GroundHit.GetComponent()->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Overload wave skipped: no blocking ground for team %d at %s"), TeamId, *Location.ToString());
			continue;
		}
		Location = GroundHit.ImpactPoint;
		const UModularPawnData* Data = UnitDefinitions[Index % UnitDefinitions.Num()].LoadSynchronous();
		if (!Data || !Data->Specification.HasTag(SunrisePawnTags::Kind_Creep))
		{
			continue;
		}
		UClass* PawnClass = Data->PawnClass.LoadSynchronous();
		if (!PawnClass || !PawnClass->IsChildOf(ASunriseUnit::StaticClass()))
		{
			continue;
		}
		const ASunriseUnit* Defaults = PawnClass->GetDefaultObject<ASunriseUnit>();
		if (!Defaults->GetCharacterMovement()->IsWalkable(GroundHit))
		{
			continue;
		}
		Location.Z += Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f;
		ASunriseUnit* Unit = USunriseUnitManagerComponent::SpawnUnit(Data, FTransform(Facing, Location), GetOwner());
		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overload lane %s could not place definition %d for team %d without collision"),
				*GetNameSafe(Lane.Get()), Index % UnitDefinitions.Num(), TeamId);
			continue;
		}
		Unit->SetGenericTeamId(IntegerToGenericTeamId(TeamId));
		if (bUseNonBlockingPawnCollision)
		{
			Unit->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		}
		SpawnedUnits.Add(Unit);
		++SpawnedCount;

		UOverloadInteractorComponent* Interactor = NewObject<UOverloadInteractorComponent>(Unit, TEXT("OverloadInteractor"));
		Interactor->RegisterComponent();
		Interactor->InitializeForUnit();
		UOverloadLaneFollowerComponent* Follower = NewObject<UOverloadLaneFollowerComponent>(Unit, TEXT("OverloadLaneFollower"));
		Follower->RegisterComponent();
		Follower->Initialize(Lane.Get(), LateralOffset);
	}
	UE_LOG(LogTemp, Log, TEXT("Overload lane %s spawned %d/%d units for team %d at %s"), *GetNameSafe(Lane.Get()), SpawnedCount, WaveSize,
		TeamId, bSpawnAtSplineStart ? TEXT("start") : TEXT("end"));
}

void UOverloadWaveSpawnerComponent::PruneTrackedUnits()
{
	SpawnedUnits.RemoveAllSwap(
		[](const TWeakObjectPtr<ASunriseUnit>& UnitPtr)
		{
			const ASunriseUnit* Unit = UnitPtr.Get();
			return !Unit || !Unit->IsAlive();
		});
}

int32 UOverloadWaveSpawnerComponent::GetMaxAliveUnitsForTeam(int32 TeamId) const
{
	if (!OverloadTeamIds::IsPlayable(TeamId))
	{
		return 0;
	}
	const float Multiplier = TeamId == OverloadTeamIds::InitialPlayer ? 1.0f : EnemyWaveMultiplier;
	return FMath::RoundToInt(MaxAliveUnitsPerTeam * Multiplier);
}

void UOverloadWaveSpawnerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, EnemyWaveMultiplier);
}
