// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/Components/SunriseTopDownCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/Pawn.h"

USunriseTopDownCameraComponent* USunriseTopDownCameraComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USunriseTopDownCameraComponent>() : nullptr;
}

void USunriseTopDownCameraComponent::OnRegister()
{
	Super::OnRegister();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!ensureMsgf(Pawn, TEXT("SunriseTopDownCameraComponent can only be added to a Pawn")) || !Pawn->GetRootComponent())
	{
		return;
	}

	Camera = Pawn->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		Camera = NewObject<UCameraComponent>(Pawn, TEXT("TopDownCamera"));
		bOwnsCamera = true;
		Camera->SetupAttachment(Pawn->GetRootComponent());
		Camera->RegisterComponent();
	}
	Camera->SetRelativeLocation(CameraRelativeLocation);
	Camera->SetRelativeRotation(CameraRelativeRotation);
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 1500.0f;
	Camera->AutoPlaneShift = 1.0f;
	Camera->bUpdateOrthoPlanes = false;

	Movement = Pawn->FindComponentByClass<UFloatingPawnMovement>();
	if (!Movement)
	{
		Movement = NewObject<UFloatingPawnMovement>(Pawn, TEXT("TopDownMovement"));
		bOwnsMovement = true;
		Movement->bConstrainToPlane = true;
		Movement->SetPlaneConstraintNormal(FVector::UpVector);
		Movement->SetPlaneConstraintOrigin(FVector::ZeroVector);
		Movement->MaxSpeed = 2200.0f;
		Movement->Acceleration = 8000.0f;
		Movement->Deceleration = 8000.0f;
		Movement->RegisterComponent();
	}
}

void USunriseTopDownCameraComponent::OnUnregister()
{
	if (bOwnsMovement && Movement)
	{
		Movement->DestroyComponent();
	}
	if (bOwnsCamera && Camera)
	{
		Camera->DestroyComponent();
	}
	Movement = nullptr;
	Camera = nullptr;
	bOwnsMovement = false;
	bOwnsCamera = false;
	Super::OnUnregister();
}

void USunriseTopDownCameraComponent::SetZoom(float Value)
{
	if (Camera)
	{
		Camera->SetOrthoWidth(Value);
	}
}

void USunriseTopDownCameraComponent::RotateYaw(float AxisValue, float DeltaSeconds)
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && !FMath::IsNearlyZero(AxisValue))
	{
		Pawn->AddActorWorldRotation(FRotator(0.0f, AxisValue * YawRotationSpeed * DeltaSeconds, 0.0f));
	}
}
