// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/PawnComponent.h"

#include "SunriseTopDownCameraComponent.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;

/** Optional orthographic camera and movement feature added to SunrisePawn by an Experience. */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USunriseTopDownCameraComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	static USunriseTopDownCameraComponent* Find(const AActor* Actor);
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	void SetZoom(float Value);
	void RotateYaw(float AxisValue, float DeltaSeconds);
	UCameraComponent* GetCamera() const { return Camera; }

private:
	UPROPERTY(EditAnywhere, Category = "Sunrise|Camera")
	FVector CameraRelativeLocation = FVector(-1600.0f, 0.0f, 1600.0f);

	UPROPERTY(EditAnywhere, Category = "Sunrise|Camera")
	FRotator CameraRelativeRotation = FRotator(-45.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Sunrise|Camera", meta = (ClampMin = "0.0", Units = "deg/s"))
	float YawRotationSpeed = 90.0f;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(Transient)
	TObjectPtr<UFloatingPawnMovement> Movement;
	bool bOwnsCamera = false;
	bool bOwnsMovement = false;
};
