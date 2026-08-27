// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"

#include "Abilities/SunriseHeroSquadAbility.h"
#include "Components/SplineComponent.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "Units/SunriseUnit.h"

UOverloadLaneFollowerComponent::UOverloadLaneFollowerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.35f;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UOverloadLaneFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Unit || !Unit->HasAuthority() || !Unit->IsAlive() || !Lane.IsValid())
	{
		return;
	}
	if (Unit->HasActivePlayerOrder())
	{
		return;
	}
	USplineComponent* Spline = Lane->GetLaneSpline();
	CurrentDistance = Spline->GetDistanceAlongSplineAtLocation(Unit->GetActorLocation(), ESplineCoordinateSpace::World);

	if (Unit->IsHero() && Unit->GetHeroSquadAbilityClass())
	{
		USunriseHeroSquadAbility::ActivateForHero(Unit, Unit->GetHeroSquadAbilityClass());
	}

	if (!Unit->IsHero() && (Unit->GetOrderState() == ESunriseOrderState::Attacking || Unit->GetOrderState() == ESunriseOrderState::Healing))
	{
		return;
	}

	if (AOverloadGuardTower* Tower = Lane->FindNextHostileTower(Unit->GetTeamId(), CurrentDistance, TravelDirection))
	{
		if (UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>())
		{
			Interactor->RequestHack(Tower);
			return;
		}
	}

	const float NextDistance = FMath::Clamp(CurrentDistance + TravelDirection * WaypointSpacing, 0.0f, Spline->GetSplineLength());
	FVector NextLocation = Spline->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::World);
	const FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::World);
	NextLocation += Right * LateralOffset;
	Unit->IssueAutonomousMoveOrder(NextLocation);
}

void UOverloadLaneFollowerComponent::Initialize(AOverloadLaneSpline* InLane, float InLateralOffset)
{
	Unit = Cast<ASunriseUnit>(GetOwner());
	Lane = InLane;
	LateralOffset = InLateralOffset;
	TravelDirection = Unit && Lane.IsValid() && Unit->GetTeamId() == Lane->GetTargetTeamId() ? -1.0f : 1.0f;
	CurrentDistance = TravelDirection > 0.0f || !Lane.IsValid() ? 0.0f : Lane->GetLaneSpline()->GetSplineLength();
	IssuedDistance = -1.0f;
}
