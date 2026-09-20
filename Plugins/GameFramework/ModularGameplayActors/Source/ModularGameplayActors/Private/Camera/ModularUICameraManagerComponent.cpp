// Copyright Epic Games, Inc. All Rights Reserved.

#include "Camera/ModularUICameraManagerComponent.h"

#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ModularPlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularUICameraManagerComponent)

class AActor;
class FDebugDisplayInfo;

UModularUICameraManagerComponent* UModularUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	if (PC != nullptr)
	{
		if (AModularPlayerCameraManager* PCCamera = Cast<AModularPlayerCameraManager>(PC->PlayerCameraManager))
		{
			return PCCamera->GetUICameraComponent();
		}
	}

	return nullptr;
}

UModularUICameraManagerComponent::UModularUICameraManagerComponent()
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

void UModularUICameraManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UModularUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	TGuardValue<bool> UpdatingViewTargetGuard(bUpdatingViewTarget, true);

	ViewTarget = InViewTarget;
	CastChecked<AModularPlayerCameraManager>(GetOwner())->SetViewTarget(ViewTarget, TransitionParams);
}

bool UModularUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UModularUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
}

void UModularUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
}
