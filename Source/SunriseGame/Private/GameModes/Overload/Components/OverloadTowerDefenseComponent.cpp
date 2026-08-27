// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadTowerDefenseComponent.h"

#include "Engine/OverlapResult.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "TimerManager.h"
#include "Units/SunriseUnit.h"

UOverloadTowerDefenseComponent::UOverloadTowerDefenseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOverloadTowerDefenseComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DefenseTimer, this, &UOverloadTowerDefenseComponent::ExecuteDefensePulse, DefensePulseInterval, true, DefensePulseInterval);
	}
}

void UOverloadTowerDefenseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DefenseTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UOverloadTowerDefenseComponent::ExecuteDefensePulse()
{
	AOverloadGuardTower* Tower = Cast<AOverloadGuardTower>(GetOwner());
	if (!Tower)
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OverloadTowerDefense), false, Tower);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Tower->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ScanRadius), Params);
	ASunriseUnit* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Overlap.GetActor());
		if (!Unit || !Unit->IsAlive() || Unit->GetTeamId() < 0 || Unit->GetTeamId() == Tower->GetTeamId())
		{
			continue;
		}
		const float Distance = FVector::DistSquared2D(Unit->GetActorLocation(), Tower->GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = Unit;
			BestDistance = Distance;
		}
	}
	if (Best)
	{
		Tower->TryTowerAttack(Best);
	}
}
