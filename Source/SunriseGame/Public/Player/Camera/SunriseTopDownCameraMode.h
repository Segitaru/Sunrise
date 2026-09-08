// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Camera/ModularCameraMode.h"

#include "SunriseTopDownCameraMode.generated.h"

/** Orthographic RTS view around the camera pawn's ground pivot. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseTopDownCameraMode : public UModularCameraMode
{
	GENERATED_BODY()
public:
	USunriseTopDownCameraMode();

protected:
	virtual void UpdateView(float DeltaTime) override;
	UPROPERTY(EditDefaultsOnly, Category = "View")
	FVector PivotOffset = FVector(-1600.0f, 0.0f, 1600.0f);
	UPROPERTY(EditDefaultsOnly, Category = "View", meta = (ClampMin = "-89", ClampMax = "-1"))
	float Pitch = -45.0f;
};
