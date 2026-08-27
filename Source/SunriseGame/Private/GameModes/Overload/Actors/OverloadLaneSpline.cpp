// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Actors/OverloadLaneSpline.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"
#include "UObject/ConstructorHelpers.h"

AOverloadLaneSpline::AOverloadLaneSpline()
{
	LaneSpline = CreateDefaultSubobject<USplineComponent>(TEXT("LaneSpline"));
	SetRootComponent(LaneSpline);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StartDummyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EndDummyMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	StartMarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartMarkerMesh"));
	StartMarkerMesh->SetupAttachment(LaneSpline);
	StartMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StartMarkerMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.15f));
	if (StartDummyMesh.Succeeded())
	{
		StartMarkerMesh->SetStaticMesh(StartDummyMesh.Object);
	}

	EndMarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndMarkerMesh"));
	EndMarkerMesh->SetupAttachment(LaneSpline);
	EndMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EndMarkerMesh->SetRelativeScale3D(FVector(0.7f));
	if (EndDummyMesh.Succeeded())
	{
		EndMarkerMesh->SetStaticMesh(EndDummyMesh.Object);
	}

	WaveSpawner = CreateDefaultSubobject<UOverloadWaveSpawnerComponent>(TEXT("WaveSpawner"));
}

void AOverloadLaneSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!LaneSpline || !StartMarkerMesh || !EndMarkerMesh)
	{
		return;
	}

	const float SplineLength = LaneSpline->GetSplineLength();
	StartMarkerMesh->SetRelativeLocationAndRotation(LaneSpline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::Local),
		LaneSpline->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::Local));
	EndMarkerMesh->SetRelativeLocationAndRotation(LaneSpline->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local),
		LaneSpline->GetRotationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::Local));
}

float AOverloadLaneSpline::GetCheckpointDistance(int32 ZeroBasedIndex) const
{
	return LaneSpline->GetSplineLength() * (FMath::Clamp(ZeroBasedIndex, 0, CheckpointCount - 1) + 1.0f) / (CheckpointCount + 1.0f);
}

UOverloadWaveSpawnerComponent* AOverloadLaneSpline::GetOrCreateWaveSpawner()
{
	if (IsValid(WaveSpawner))
	{
		return WaveSpawner;
	}
	WaveSpawner = FindComponentByClass<UOverloadWaveSpawnerComponent>();
	if (!WaveSpawner && HasAuthority())
	{
		WaveSpawner = NewObject<UOverloadWaveSpawnerComponent>(this, TEXT("WaveSpawnerRuntime"));
		AddInstanceComponent(WaveSpawner);
		WaveSpawner->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("Overload lane %s repaired its missing WaveSpawner component"), *GetName());
	}
	return WaveSpawner;
}

void AOverloadLaneSpline::SetSpawnedTowers(const TArray<AOverloadGuardTower*>& Towers)
{
	SpawnedTowerStorage.Reset();
	for (AOverloadGuardTower* Tower : Towers)
	{
		if (IsValid(Tower))
		{
			SpawnedTowerStorage.Add(Tower);
		}
	}
}

AOverloadGuardTower* AOverloadLaneSpline::FindNextHostileTower(int32 UnitTeamId, float FromDistance, float TravelDirection) const
{
	AOverloadGuardTower* Best = nullptr;
	float BestForwardDistance = TNumericLimits<float>::Max();
	const float Direction = TravelDirection < 0.0f ? -1.0f : 1.0f;
	for (AOverloadGuardTower* Tower : SpawnedTowerStorage)
	{
		if (!IsValid(Tower) || Tower->GetTeamId() == UnitTeamId)
		{
			continue;
		}
		const float Distance = FindTowerDistance(Tower);
		const float ForwardDistance = (Distance - FromDistance) * Direction;
		if (ForwardDistance >= -100.0f && ForwardDistance < BestForwardDistance)
		{
			Best = Tower;
			BestForwardDistance = ForwardDistance;
		}
	}
	return Best;
}

float AOverloadLaneSpline::FindTowerDistance(const AOverloadGuardTower* Tower) const
{
	if (!Tower)
	{
		return TNumericLimits<float>::Max();
	}
	return LaneSpline->GetDistanceAlongSplineAtLocation(Tower->GetActorLocation(), ESplineCoordinateSpace::World);
}
