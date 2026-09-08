// Copyright Epic Games, Inc. All Rights Reserved.
#include "Player/Camera/SunriseTopDownCameraMode.h"

#include "Units/SunrisePawn.h"

USunriseTopDownCameraMode::USunriseTopDownCameraMode()
{
	BlendTime = 0.25f;
}

void USunriseTopDownCameraMode::UpdateView(float DeltaTime)
{
	const AActor* Target = GetTargetActor();
	if (!Target)
	{
		Super::UpdateView(DeltaTime);
		return;
	}
	const FRotator Yaw(0.0f, Target->GetActorRotation().Yaw, 0.0f);
	View.Location = Target->GetActorLocation() + Yaw.RotateVector(PivotOffset);
	View.Rotation = FRotator(Pitch, Yaw.Yaw, 0.0f);
	View.ControlRotation = Yaw;
	View.FieldOfView = FieldOfView;
	View.ProjectionMode = ECameraProjectionMode::Orthographic;
	const ASunrisePawn* Pawn = Cast<ASunrisePawn>(Target);
	View.OrthoWidth = Pawn ? Pawn->GetCameraZoom() : 1500.0f;
}
