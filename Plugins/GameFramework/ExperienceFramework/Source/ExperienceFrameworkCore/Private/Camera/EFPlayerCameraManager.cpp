// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/EFPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Camera/EFCameraComponent.h"
#include "Camera/EFUICameraManagerComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AEFPlayerCameraManager::AEFPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = EF_CAMERA_DEFAULT_FOV;
	ViewPitchMin = EF_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = EF_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UEFUICameraManagerComponent>(UICameraComponentName);
}

UEFUICameraManagerComponent* AEFPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AEFPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AEFPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("EFPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UEFCameraComponent* CameraComponent = UEFCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}
