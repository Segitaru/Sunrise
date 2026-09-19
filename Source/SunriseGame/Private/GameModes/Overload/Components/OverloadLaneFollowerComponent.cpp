// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"

#include "Components/SplineComponent.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "Units/SunriseUnit.h"

UOverloadLaneFollowerComponent::UOverloadLaneFollowerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UOverloadLaneFollowerComponent::RequestNextMove()
{
	if (!Unit || !Unit->HasAuthority() || !Unit->IsAlive() || !Lane.IsValid() || Unit->HasActivePlayerOrder())
	{
		return false;
	}

	if (!Unit->IsHero() && (Unit->GetOrderState() == ESunriseOrderState::Attacking || Unit->GetOrderState() == ESunriseOrderState::Healing))
	{
		return false;
	}

	USplineComponent* Spline = Lane->GetLaneSpline();
	const float CurrentDistance = Spline->GetDistanceAlongSplineAtLocation(Unit->GetActorLocation(), ESplineCoordinateSpace::World);
	if (AOverloadGuardTower* Tower = Lane->FindNextHostileTower(Unit->GetTeamId(), CurrentDistance, TravelDirection))
	{
		if (UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>())
		{
			if (Interactor->RequestHack(Tower))
			{
				return false;
			}
		}
		return false;
	}

	const float NextDistance = FMath::Clamp(CurrentDistance + TravelDirection * WaypointSpacing, 0.0f, Spline->GetSplineLength());
	if (FMath::IsNearlyEqual(NextDistance, CurrentDistance, 1.0f))
	{
		return false;
	}

	const FVector NextLocation = Spline->GetLocationAtDistanceAlongSpline(NextDistance, ESplineCoordinateSpace::World);
	return Unit->IssueAutonomousMoveOrder(NextLocation);
}

void UOverloadLaneFollowerComponent::Initialize(AOverloadLaneSpline* InLane, const FVector& InLateralOffset)
{
	Unit = Cast<ASunriseUnit>(GetOwner());
	Lane = InLane;
	LateralOffset = InLateralOffset;
	TravelDirection = Unit && Lane.IsValid() && Unit->GetTeamId() == Lane->GetTargetTeamId() ? -1.0f : 1.0f;
}