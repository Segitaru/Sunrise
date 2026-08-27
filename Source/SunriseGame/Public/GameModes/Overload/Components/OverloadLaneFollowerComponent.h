// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "OverloadLaneFollowerComponent.generated.h"

class AOverloadLaneSpline;
class ASunriseUnit;

UCLASS(ClassGroup = (Overload), BlueprintType)
class SUNRISEGAME_API UOverloadLaneFollowerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOverloadLaneFollowerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Initialize(AOverloadLaneSpline* InLane, float InLateralOffset = 0.0f);

protected:
	/** Legacy tuning retained for serialized assets; terminal-first movement no longer uses spline waypoint stepping. */
	UPROPERTY(EditAnywhere, Category = "Overload|Lane", meta = (ClampMin = "100.0", Units = "cm"))
	float WaypointSpacing = 500.0f;
	/** Legacy tuning retained for serialized assets; terminal-first movement no longer uses spline waypoint stepping. */
	UPROPERTY(EditAnywhere, Category = "Overload|Lane", meta = (ClampMin = "100.0", Units = "cm"))
	float ObjectiveNoticeDistance = 650.0f;

private:
	TObjectPtr<ASunriseUnit> Unit;
	TWeakObjectPtr<AOverloadLaneSpline> Lane;
	float CurrentDistance = 0.0f;
	float IssuedDistance = -1.0f;
	float LateralOffset = 0.0f;
	float TravelDirection = 1.0f;
};
