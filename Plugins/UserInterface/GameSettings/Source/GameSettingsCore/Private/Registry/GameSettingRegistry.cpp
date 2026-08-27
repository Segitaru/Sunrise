// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSettingRegistry.h"

#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "GameSettingsLocal.h"
#include "GameSettingsShared.h"
#include "UObject/WeakObjectPtr.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingRegistry)

DEFINE_LOG_CATEGORY(LogGameGameSettingRegistry);

#define LOCTEXT_NAMESPACE "GameSetting"

//--------------------------------------
// UGameSettingRegistry
//--------------------------------------

UGameSettingRegistry::UGameSettingRegistry()
{
}

UGameSettingRegistry* UGameSettingRegistry::Get(ULocalPlayer* InLocalPlayer)
{
	UGameSettingRegistry* Registry =
		FindObject<UGameSettingRegistry>(InLocalPlayer, TEXT("GameGameSettingRegistry"), EFindObjectFlags::ExactClass);
	if (Registry == nullptr)
	{
		Registry = NewObject<UGameSettingRegistry>(InLocalPlayer, TEXT("GameGameSettingRegistry"));
		Registry->Initialize(InLocalPlayer);
	}

	return Registry;
}

bool UGameSettingRegistry::IsFinishedInitializing() const
{
	for (UGameSetting* Setting : RegisteredSettings)
	{
		if (!Setting->IsReady())
		{
			return false;
		}
	}

	return IsValid(UGameSettingsShared::GetSharedSettings(OwningLocalPlayer));
}

void UGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(InLocalPlayer);

	VideoSettings = InitializeVideoSettings(LocalPlayer);
	InitializeVideoSettings_FrameRates(VideoSettings, LocalPlayer);
	RegisterSetting(VideoSettings);

	AudioSettings = InitializeAudioSettings(LocalPlayer);
	RegisterSetting(AudioSettings);

	GameplaySettings = InitializeGameplaySettings(LocalPlayer);
	RegisterSetting(GameplaySettings);

	MouseAndKeyboardSettings = InitializeMouseAndKeyboardSettings(LocalPlayer);
	RegisterSetting(MouseAndKeyboardSettings);

	GamepadSettings = InitializeGamepadSettings(LocalPlayer);
	RegisterSetting(GamepadSettings);
}

void UGameSettingRegistry::SaveChanges()
{
	// Game user settings need to be applied to handle things like resolution, this saves indirectly
	UGameSettingsLocal::Get()->ApplySettings(false);

	if (UGameSettingsShared* const SharedSettings = UGameSettingsShared::GetSharedSettings(OwningLocalPlayer); IsValid(SharedSettings))
	{
		SharedSettings->ApplySettings();
		SharedSettings->SaveSettings();
	}
}

void UGameSettingRegistry::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;
	OnInitialize(InLocalPlayer);

	//UGameFeaturesSubsystem
}

void UGameSettingRegistry::Regenerate()
{
	for (UGameSetting* Setting : RegisteredSettings)
	{
		Setting->MarkAsGarbage();
	}
	RegisteredSettings.Reset();
	TopLevelSettings.Reset();

	OnInitialize(OwningLocalPlayer);
}

void UGameSettingRegistry::GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings)
{
	TArray<UGameSetting*> RootSettings;
	if (FilterState.GetSettingRootList().Num() > 0)
	{
		RootSettings.Append(FilterState.GetSettingRootList());
	}
	else
	{
		RootSettings.Append(TopLevelSettings);
	}

	for (UGameSetting* TopLevelSetting : RootSettings)
	{
		if (const UGameSettingCollection* TopLevelCollection = Cast<UGameSettingCollection>(TopLevelSetting))
		{
			TopLevelCollection->GetSettingsForFilter(FilterState, InOutSettings);
		}
		else
		{
			if (FilterState.DoesSettingPassFilter(*TopLevelSetting))
			{
				InOutSettings.Add(TopLevelSetting);
			}
		}
	}
}

UGameSetting* UGameSettingRegistry::FindSettingByDevName(const FName& SettingDevName)
{
	for (UGameSetting* Setting : RegisteredSettings)
	{
		if (Setting->GetDevName() == SettingDevName)
		{
			return Setting;
		}
	}

	return nullptr;
}

void UGameSettingRegistry::RegisterSetting(UGameSetting* InSetting)
{
	if (InSetting)
	{
		TopLevelSettings.Add(InSetting);
		InSetting->SetRegistry(this);
		RegisterInnerSettings(InSetting);
	}
}

void UGameSettingRegistry::RegisterInnerSettings(UGameSetting* InSetting)
{
	InSetting->OnSettingChangedEvent.AddUObject(this, &ThisClass::HandleSettingChanged);
	InSetting->OnSettingAppliedEvent.AddUObject(this, &ThisClass::HandleSettingApplied);
	InSetting->OnSettingEditConditionChangedEvent.AddUObject(this, &ThisClass::HandleSettingEditConditionsChanged);

	// Not a fan of this, but it makes sense to aggregate action events for simplicity.
	if (UGameSettingAction* ActionSetting = Cast<UGameSettingAction>(InSetting))
	{
		ActionSetting->OnExecuteNamedActionEvent.AddUObject(this, &ThisClass::HandleSettingNamedAction);
	}
	// Not a fan of this, but it makes sense to aggregate navigation events for simplicity.
	else if (UGameSettingCollectionPage* NewPageCollection = Cast<UGameSettingCollectionPage>(InSetting))
	{
		NewPageCollection->OnExecuteNavigationEvent.AddUObject(this, &ThisClass::HandleSettingNavigation);
	}

#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(!RegisteredSettings.Contains(InSetting), TEXT("This setting has already been registered!"));
	ensureAlwaysMsgf(nullptr == RegisteredSettings.FindByPredicate(
									[&InSetting](UGameSetting* ExistingSetting)
									{
										return InSetting->GetDevName() == ExistingSetting->GetDevName();
									}),
		TEXT("A setting with this DevName has already been registered!  DevNames must be unique within a registry."));
#endif

	RegisteredSettings.Add(InSetting);

	for (UGameSetting* ChildSetting : InSetting->GetChildSettings())
	{
		RegisterInnerSettings(ChildSetting);
	}
}

void UGameSettingRegistry::HandleSettingApplied(UGameSetting* Setting)
{
	OnSettingApplied(Setting);
}

void UGameSettingRegistry::HandleSettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason)
{
	OnSettingChangedEvent.Broadcast(Setting, Reason);
}

void UGameSettingRegistry::HandleSettingEditConditionsChanged(UGameSetting* Setting)
{
	OnSettingEditConditionChangedEvent.Broadcast(Setting);
}

void UGameSettingRegistry::HandleSettingNamedAction(UGameSetting* Setting, FGameplayTag GameSettings_Action_Tag)
{
	OnSettingNamedActionEvent.Broadcast(Setting, GameSettings_Action_Tag);
}

void UGameSettingRegistry::HandleSettingNavigation(UGameSetting* Setting)
{
	OnExecuteNavigationEvent.Broadcast(Setting);
}

#undef LOCTEXT_NAMESPACE
