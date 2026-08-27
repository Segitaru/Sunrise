// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSettingsShared.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Rendering/SlateRenderer.h"
#include "SubtitleDisplaySubsystem.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsShared)

static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");
static TMap<TObjectPtr<ULocalPlayer>, TStrongObjectPtr<UGameSettingsShared>> SettingsForPlayer;

namespace GameSettingsSharedCVars
{
	static float DefaultGamepadLeftStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadLeftStickInnerDeadZone(
		TEXT("gpad.DefaultLeftStickInnerDeadZone"), DefaultGamepadLeftStickInnerDeadZone, TEXT("Gamepad left stick inner deadzone"));

	static float DefaultGamepadRightStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadRightStickInnerDeadZone(
		TEXT("gpad.DefaultRightStickInnerDeadZone"), DefaultGamepadRightStickInnerDeadZone, TEXT("Gamepad right stick inner deadzone"));
} // namespace GameSettingsSharedCVars

UGameSettingsShared::UGameSettingsShared()
{
	FInternationalization::Get().OnCultureChanged().AddUObject(this, &ThisClass::OnCultureChanged);

	GamepadMoveStickDeadZone = GameSettingsSharedCVars::DefaultGamepadLeftStickInnerDeadZone;
	GamepadLookStickDeadZone = GameSettingsSharedCVars::DefaultGamepadRightStickInnerDeadZone;
}


UGameSettingsShared* UGameSettingsShared::GetSharedSettings(ULocalPlayer* FromPlayer)
{
	if (!IsValid(FromPlayer))
	{
		return nullptr;
	}

	if (TStrongObjectPtr<UGameSettingsShared>* FoundSettings = SettingsForPlayer.Find(FromPlayer); FoundSettings)
	{
		if (UGameSettingsShared* CurrentSettings = FoundSettings->Get(); IsValid(CurrentSettings))
		{
			return CurrentSettings;
		}
	}

	// On PC it's okay to use the sync load because it only checks the disk
	// This could use a platform tag to check for proper save support instead
	UGameSettingsShared* CreatedSettings = nullptr;
	if (constexpr bool bCanLoadBeforeLogin = PLATFORM_DESKTOP)
	{
		CreatedSettings = LoadOrCreateSettings(FromPlayer);
	}
	else
	{
		// We need to wait for user login to get the real settings so return temp ones
		CreatedSettings = CreateTemporarySettings(FromPlayer);
	}
	if (IsValid(CreatedSettings))
	{
		SettingsForPlayer.Add(FromPlayer, TStrongObjectPtr<UGameSettingsShared>(CreatedSettings));
	}
	return CreatedSettings;
}

int32 UGameSettingsShared::GetLatestDataVersion() const
{
	// 0 = before subclassing ULocalPlayerSaveGame
	// 1 = first proper version
	return 1;
}

UGameSettingsShared* UGameSettingsShared::CreateTemporarySettings(const ULocalPlayer* LocalPlayer)
{
	// This is not loaded from disk but should be set up to save
	UGameSettingsShared* SharedSettings = Cast<UGameSettingsShared>(
		CreateNewSaveGameForLocalPlayer(UGameSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();

	return SharedSettings;
}

UGameSettingsShared* UGameSettingsShared::LoadOrCreateSettings(const ULocalPlayer* LocalPlayer)
{
	// This will stall the main thread while it loads
	UGameSettingsShared* SharedSettings =
		Cast<UGameSettingsShared>(LoadOrCreateSaveGameForLocalPlayer(StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();

	return SharedSettings;
}

bool UGameSettingsShared::AsyncLoadOrCreateSettings(const ULocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate)
{
	FOnLocalPlayerSaveGameLoadedNative Lambda = FOnLocalPlayerSaveGameLoadedNative::CreateLambda(
		[Delegate](ULocalPlayerSaveGame* LoadedSave)
		{
			UGameSettingsShared* LoadedSettings = CastChecked<UGameSettingsShared>(LoadedSave);

			LoadedSettings->ApplySettings();

			Delegate.ExecuteIfBound(LoadedSettings);
		});

	return AsyncLoadOrCreateSaveGameForLocalPlayer(StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME, Lambda);
}

void UGameSettingsShared::SaveSettings()
{
	// Schedule an async save because it's okay if it fails
	AsyncSaveGameToSlotForLocalPlayer();

	// TODO_BH: Move this to the serialize function instead with a bumped version number
	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->AsyncSaveSettings();
		}
	}
}

void UGameSettingsShared::ApplySettings()
{
	ApplySubtitleOptions();
	ApplyBackgroundAudioSettings();
	ApplyCultureSettings();

	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->ApplySettings();
		}
	}
}

void UGameSettingsShared::SetColorBlindStrength(int32 InColorBlindStrength)
{
	InColorBlindStrength = FMath::Clamp(InColorBlindStrength, 0, 10);
	if (ColorBlindStrength != InColorBlindStrength)
	{
		ColorBlindStrength = InColorBlindStrength;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

void UGameSettingsShared::SetGamepadInputAPIOption(const EGameGamepadInputAPIOption NewValue)
{
	const bool bWasValueChanged = ChangeValueAndDirty(GamepadInputAPIOptions, NewValue);

	// We dont have any other additional work to do if the value wasn't changed.
	if (!bWasValueChanged)
	{
		return;
	}

	// A comma-separated list of preferred gamepad APIs
	FString GamepadAPIOptions = TEXT("");

	switch (NewValue)
	{
		case EGameGamepadInputAPIOption::Legacy:
			GamepadAPIOptions = TEXT("XInput,WinDualShock");
			break;
		case EGameGamepadInputAPIOption::Modern:
			GamepadAPIOptions = TEXT("GameInput");
			break;
		default:
			checkNoEntry();
			break;
	}

	FGenericPlatformMisc::SetPreferredInputDevices(*GamepadAPIOptions);
}

int32 UGameSettingsShared::GetColorBlindStrength() const
{
	return ColorBlindStrength;
}

void UGameSettingsShared::SetColorBlindMode(EColorBlindMode InMode)
{
	if (ColorBlindMode != InMode)
	{
		ColorBlindMode = InMode;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

EColorBlindMode UGameSettingsShared::GetColorBlindMode() const
{
	return ColorBlindMode;
}

void UGameSettingsShared::ApplySubtitleOptions()
{
	if (USubtitleDisplaySubsystem* SubtitleSystem = USubtitleDisplaySubsystem::Get(OwningPlayer))
	{
		FSubtitleFormat SubtitleFormat;
		SubtitleFormat.SubtitleTextSize = SubtitleTextSize;
		SubtitleFormat.SubtitleTextColor = SubtitleTextColor;
		SubtitleFormat.SubtitleTextBorder = SubtitleTextBorder;
		SubtitleFormat.SubtitleBackgroundOpacity = SubtitleBackgroundOpacity;

		SubtitleSystem->SetSubtitleDisplayOptions(SubtitleFormat);
	}
}

//////////////////////////////////////////////////////////////////////

void UGameSettingsShared::SetAllowAudioInBackgroundSetting(EGameAllowBackgroundAudioSetting NewValue)
{
	if (ChangeValueAndDirty(AllowAudioInBackground, NewValue))
	{
		ApplyBackgroundAudioSettings();
	}
}

void UGameSettingsShared::ApplyBackgroundAudioSettings()
{
	if (OwningPlayer && OwningPlayer->IsPrimaryPlayer())
	{
		FApp::SetUnfocusedVolumeMultiplier((AllowAudioInBackground != EGameAllowBackgroundAudioSetting::Off) ? 1.0f : 0.0f);
	}
}

//////////////////////////////////////////////////////////////////////

void UGameSettingsShared::ApplyCultureSettings()
{
	if (bResetToDefaultCulture)
	{
		const FCulturePtr SystemDefaultCulture = FInternationalization::Get().GetDefaultCulture();
		check(SystemDefaultCulture.IsValid());

		const FString CultureToApply = SystemDefaultCulture->GetName();
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Clear this string
			GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		bResetToDefaultCulture = false;
	}
	else if (!PendingCulture.IsEmpty())
	{
		// SetCurrentCulture may trigger PendingCulture to be cleared (if a culture change is broadcast) so we take a copy of it to work with
		const FString CultureToApply = PendingCulture;
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Note: This is intentionally saved to the users config
			// We need to localize text before the player logs in and very early in the loading screen
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *CultureToApply, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ClearPendingCulture();
	}
}

void UGameSettingsShared::ResetCultureToCurrentSettings()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

const FString& UGameSettingsShared::GetPendingCulture() const
{
	return PendingCulture;
}

void UGameSettingsShared::SetPendingCulture(const FString& NewCulture)
{
	PendingCulture = NewCulture;
	bResetToDefaultCulture = false;
	bIsDirty = true;
}

void UGameSettingsShared::OnCultureChanged()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

void UGameSettingsShared::ClearPendingCulture()
{
	PendingCulture.Reset();
}

bool UGameSettingsShared::IsUsingDefaultCulture() const
{
	FString Culture;
	GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Culture, GGameUserSettingsIni);

	return Culture.IsEmpty();
}

void UGameSettingsShared::ResetToDefaultCulture()
{
	ClearPendingCulture();
	bResetToDefaultCulture = true;
	bIsDirty = true;
}

//////////////////////////////////////////////////////////////////////

void UGameSettingsShared::ApplyInputSensitivity()
{
}
