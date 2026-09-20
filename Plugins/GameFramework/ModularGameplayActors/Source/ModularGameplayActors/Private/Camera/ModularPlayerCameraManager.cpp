// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/ModularPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ModularCameraComponent.h"
#include "Camera/ModularUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AModularPlayerCameraManager::AModularPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = MODULAR_CAMERA_DEFAULT_FOV;
	ViewPitchMin = MODULAR_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = MODULAR_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UModularUICameraManagerComponent>(UICameraComponentName);
}

UModularUICameraManagerComponent* AModularPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AModularPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
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

void AModularPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("ModularPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UModularCameraComponent* CameraComponent = UModularCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}

