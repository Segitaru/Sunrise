// Copyright Epic Games, Inc. All Rights Reserved.

#include "Foundation/FoundationControllerDisconnectedScreen.h"

#include "CommonButtonBase.h"
#include "CommonUISettings.h"
#include "Components/HorizontalBox.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "ICommonUIModule.h"
#include "NativeGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundationControllerDisconnectedScreen, All, All);

#if WITH_EDITOR
#include "CommonUIVisibilitySubsystem.h"
#endif // WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(FoundationControllerDisconnectedScreen)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_Input_HasStrictControllerPairing, "Platform.Trait.Input.HasStrictControllerPairing");

UFoundationControllerDisconnectedScreen::UFoundationControllerDisconnectedScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// By default, only strict pairing platforms will need this button.
	PlatformSupportsUserChangeTags.AddTag(TAG_Platform_Trait_Input_HasStrictControllerPairing);
}

void UFoundationControllerDisconnectedScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (!HBox_SwitchUser)
	{
		UE_LOG(LogFoundationControllerDisconnectedScreen, Error, TEXT("Unable to find HBox_SwitchUser on Widget %s"), *GetNameSafe(this));
		return;
	}

	if (!Button_ChangeUser)
	{
		UE_LOG(LogFoundationControllerDisconnectedScreen, Error, TEXT("Unable to find Button_ChangeUser on Widget %s"), *GetNameSafe(this));
		return;
	}

	HBox_SwitchUser->SetVisibility(ESlateVisibility::Collapsed);
	Button_ChangeUser->SetVisibility(ESlateVisibility::Hidden);

	if (ShouldDisplayChangeUserButton())
	{
		// This is the platform user for "unpaired" input devices. Not every platform supports this, so
		// only set this to visible if the unpaired user is valid.
		const FPlatformUserId UnpairedUserId = IPlatformInputDeviceMapper::Get().GetUserForUnpairedInputDevices();
		if (UnpairedUserId.IsValid())
		{
			HBox_SwitchUser->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Button_ChangeUser->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	Button_ChangeUser->OnClicked().AddUObject(this, &ThisClass::HandleChangeUserClicked);
}

bool UFoundationControllerDisconnectedScreen::ShouldDisplayChangeUserButton() const
{
	bool bRequiresChangeUserButton = ICommonUIModule::GetSettings().GetPlatformTraits().HasAll(PlatformSupportsUserChangeTags);

	// Check the tags that we may be emulating in the editor too
#if WITH_EDITOR
	const FGameplayTagContainer& PlatformEmulationTags = UCommonUIVisibilitySubsystem::Get(GetOwningLocalPlayer())->GetVisibilityTags();
	bRequiresChangeUserButton |= PlatformEmulationTags.HasAll(PlatformSupportsUserChangeTags);
#endif // WITH_EDITOR

	return bRequiresChangeUserButton;
}

void UFoundationControllerDisconnectedScreen::HandleChangeUserClicked()
{
	ensure(ShouldDisplayChangeUserButton());

	UE_LOG(LogFoundationControllerDisconnectedScreen, Log, TEXT("[%hs] Change user requested!"), __func__);

	const FPlatformUserId OwningPlayerId = GetOwningLocalPlayer()->GetPlatformUserId();
	const FInputDeviceId DeviceId = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(OwningPlayerId);

	FGenericPlatformApplicationMisc::ShowPlatformUserSelector(DeviceId, EPlatformUserSelectorFlags::Default,
		[this](const FPlatformUserSelectionCompleteParams& Params)
		{
			HandleChangeUserCompleted(Params);
		});
}

void UFoundationControllerDisconnectedScreen::HandleChangeUserCompleted(const FPlatformUserSelectionCompleteParams& Params)
{
	UE_LOG(LogFoundationControllerDisconnectedScreen, Log, TEXT("[%hs] User change complete!"), __func__);

	// TODO: Handle any user changing logic in your game here
}