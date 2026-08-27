// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SplineComponent.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Units/SunriseUnit.h"

UOverloadWaveSpawnerComponent::UOverloadWaveSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOverloadWaveSpawnerComponent::Initialize(AOverloadLaneSpline* InLane, TSubclassOf<ASunriseUnit> InUnitClass)
{
	Lane = InLane;
	UnitClass = InUnitClass;
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!Lane.IsValid() || !UnitClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Overload wave spawner %s cannot initialize: lane=%s unit class=%s"), *GetName(),
			*GetNameSafe(Lane.Get()), *GetNameSafe(UnitClass.Get()));
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &UOverloadWaveSpawnerComponent::SpawnWave, WaveInterval, true, InitialDelay);
	UE_LOG(LogTemp, Log, TEXT("Overload lane %s scheduled waves: first in %.1fs, interval %.1fs"), *GetNameSafe(Lane.Get()), InitialDelay,
		WaveInterval);
}

void UOverloadWaveSpawnerComponent::ApplyEnemyDifficulty(float CountMultiplier)
{
	EnemyWaveMultiplier = FMath::Max(0.5f, CountMultiplier);
	WaveInterval = FMath::Max(5.0f, WaveInterval / EnemyWaveMultiplier);
}

void UOverloadWaveSpawnerComponent::SpawnWave()
{
	if (!GetOwner()->HasAuthority() || !Lane.IsValid() || !UnitClass)
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
	static constexpr ESunriseUnitRole Roles[] = {
		ESunriseUnitRole::Melee, ESunriseUnitRole::Ranged, ESunriseUnitRole::Healer, ESunriseUnitRole::Vanguard};
	const int32 AliveCount = GetAliveUnitCount(TeamId);
	const int32 WaveSize = FMath::Max(1, FMath::RoundToInt(UE_ARRAY_COUNT(Roles) * (TeamId == 0 ? 1.0f : EnemyWaveMultiplier)));
	if (AliveCount + WaveSize > FMath::RoundToInt(MaxAliveUnitsPerTeam * (TeamId == 0 ? 1.0f : EnemyWaveMultiplier)))
	{
		UE_LOG(LogTemp, Verbose, TEXT("Overload lane %s skipped team %d wave: population %d/%d"), *GetNameSafe(Lane.Get()), TeamId,
			AliveCount, MaxAliveUnitsPerTeam);
		return;
	}
	USplineComponent* Spline = Lane->GetLaneSpline();
	const float SplineLength = Spline->GetSplineLength();
	const float SafeInset = FMath::Min(SpawnInsetDistance, SplineLength * 0.25f);
	const float SpawnDistance = bSpawnAtSplineStart ? SafeInset : SplineLength - SafeInset;
	const FVector Start = Spline->GetLocationAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World);
	const FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World);
	const FRotator Facing = (bSpawnAtSplineStart ? Spline->GetDirectionAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World)
												 : -Spline->GetDirectionAtDistanceAlongSpline(SpawnDistance, ESplineCoordinateSpace::World))
								.Rotation();
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < WaveSize; ++Index)
	{
		const float LateralOffset = (Index - 1.5f) * UnitSpacing;
		FVector Location = Start + Right * LateralOffset;
		FNavLocation Projected;
		if (Navigation && Navigation->ProjectPointToNavigation(Location, Projected, FVector(250.0f, 250.0f, 500.0f)))
		{
			Location = Projected.Location;
		}
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		const FTransform SpawnTransform(Facing, Location);
		ASunriseUnit* Unit = GetWorld()->SpawnActorDeferred<ASunriseUnit>(
			UnitClass, SpawnTransform, nullptr, nullptr, Parameters.SpawnCollisionHandlingOverride);
		if (!Unit)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overload lane %s could not place role %d for team %d without collision"),
				*GetNameSafe(Lane.Get()), static_cast<int32>(Roles[Index % UE_ARRAY_COUNT(Roles)]), TeamId);
			continue;
		}
		Unit->SetTeamId(TeamId);
		Unit->ConfigureControl(ESunriseUnitKind::Creep, nullptr);
		// Stats are prepared now; GAS attributes are initialized safely during FinishSpawning/BeginPlay.
		Unit->SetUnitRole(Roles[Index % UE_ARRAY_COUNT(Roles)], true);
		UGameplayStatics::FinishSpawningActor(Unit, Unit->GetActorTransform());
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
	UE_LOG(LogTemp, Log, TEXT("Overload lane %s spawned %d/%d units for team %d at %s"), *GetNameSafe(Lane.Get()), SpawnedCount,
		UE_ARRAY_COUNT(Roles), TeamId, bSpawnAtSplineStart ? TEXT("start") : TEXT("end"));
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
