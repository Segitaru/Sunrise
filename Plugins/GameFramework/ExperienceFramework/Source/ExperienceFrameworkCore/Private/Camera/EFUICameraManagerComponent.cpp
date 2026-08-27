// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/EFUICameraManagerComponent.h"

#include "Camera/EFPlayerCameraManager.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFUICameraManagerComponent)

class AActor;
class FDebugDisplayInfo;

UEFUICameraManagerComponent* UEFUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	if (PC != nullptr)
	{
		if (AEFPlayerCameraManager* PCCamera = Cast<AEFPlayerCameraManager>(PC->PlayerCameraManager))
		{
			return PCCamera->GetUICameraComponent();
		}
	}

	return nullptr;
}

UEFUICameraManagerComponent::UEFUICameraManagerComponent()
{
	bWantsInitializeComponent = true;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Register "showdebug" hook.
		if (!IsRunningDedicatedServer())
		{
			AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::OnShowDebugInfo);
		}
	}
}

void UEFUICameraManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UEFUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	TGuardValue<bool> UpdatingViewTargetGuard(bUpdatingViewTarget, true);

	ViewTarget = InViewTarget;
	CastChecked<AEFPlayerCameraManager>(GetOwner())->SetViewTarget(ViewTarget, TransitionParams);
}

bool UEFUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UEFUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
}

void UEFUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
}
