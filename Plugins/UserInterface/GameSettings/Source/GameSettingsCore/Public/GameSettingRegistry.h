// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "DataSource/GameSettingDataSourceDynamic.h" // IWYU pragma: keep
#include "GameLocalPlayer.h"						 // IWYU pragma: keep
#include "GameSetting.h"
#include "GameSettingsLocal.h" // IWYU pragma: keep
#include "Templates/Casts.h"

#include "GameSettingRegistry.generated.h"

#define UE_API GAMESETTINGSCORE_API

struct FGameplayTag;

//--------------------------------------
// UGameSettingRegistry
//--------------------------------------

class ULocalPlayer;
struct FGameSettingFilterState;

enum class EGameSettingChangeReason : uint8;

//--------------------------------------
// UGameGameSettingRegistry
//--------------------------------------

class UGameSettingCollection;
class ULocalPlayer;

DECLARE_LOG_CATEGORY_EXTERN(LogGameGameSettingRegistry, Log, Log);

#define GET_SHARED_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)                                                                     \
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({GET_FUNCTION_NAME_STRING_CHECKED(UGameLocalPlayer, GetSharedSettings), \
		GET_FUNCTION_NAME_STRING_CHECKED(UGameSettingsShared, FunctionOrPropertyName)}))

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)                                                                     \
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({GET_FUNCTION_NAME_STRING_CHECKED(UGameLocalPlayer, GetLocalSettings), \
		GET_FUNCTION_NAME_STRING_CHECKED(UGameSettingsLocal, FunctionOrPropertyName)}))

/**
 * 
 */
UCLASS(MinimalAPI, Abstract, BlueprintType)
class UGameSettingRegistry : public UObject
{
	GENERATED_BODY()

public:
	UE_API static UGameSettingRegistry* Get(ULocalPlayer* InLocalPlayer);

	UGameSettingCollection* InitializeVideoSettings(ULocalPlayer* InLocalPlayer);
	void InitializeVideoSettings_FrameRates(UGameSettingCollection* Screen, ULocalPlayer* InLocalPlayer);
	void AddPerformanceStatPage(UGameSettingCollection* Screen, ULocalPlayer* InLocalPlayer);

	UGameSettingCollection* InitializeAudioSettings(ULocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeGameplaySettings(ULocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeMouseAndKeyboardSettings(ULocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeGamepadSettings(ULocalPlayer* InLocalPlayer);

	void AddDLCPage(UGameSettingCollection* Screen, ULocalPlayer* InLocalPlayer);

	DECLARE_EVENT_TwoParams(UGameSettingRegistry, FOnSettingChanged, UGameSetting*, EGameSettingChangeReason);
	DECLARE_EVENT_OneParam(UGameSettingRegistry, FOnSettingEditConditionChanged, UGameSetting*);

	FOnSettingChanged OnSettingChangedEvent;
	FOnSettingEditConditionChanged OnSettingEditConditionChangedEvent;

	DECLARE_EVENT_TwoParams(
		UGameSettingRegistry, FOnSettingNamedActionEvent, UGameSetting* /*Setting*/, FGameplayTag /*GameSettings_Action_Tag*/);
	FOnSettingNamedActionEvent OnSettingNamedActionEvent;

	/** Navigate to the child settings of the provided setting. */
	DECLARE_EVENT_OneParam(UGameSettingRegistry, FOnExecuteNavigation, UGameSetting* /*Setting*/);
	FOnExecuteNavigation OnExecuteNavigationEvent;

public:
	UE_API UGameSettingRegistry();

	UE_API void Initialize(ULocalPlayer* InLocalPlayer);

	UE_API virtual void Regenerate();

	UE_API virtual bool IsFinishedInitializing() const;

	UE_API virtual void SaveChanges();

	UE_API void GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings);

	UE_API UGameSetting* FindSettingByDevName(const FName& SettingDevName);

	template <typename T = UGameSetting>
	T* FindSettingByDevNameChecked(const FName& SettingDevName)
	{
		T* Setting = Cast<T>(FindSettingByDevName(SettingDevName));
		check(Setting);
		return Setting;
	}

protected:
	UE_API virtual void OnInitialize(ULocalPlayer* InLocalPlayer);

	virtual void OnSettingApplied(UGameSetting* Setting) {}

	UE_API void RegisterSetting(UGameSetting* InSetting);
	UE_API void RegisterInnerSettings(UGameSetting* InSetting);

	// Internal event handlers.
	UE_API void HandleSettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason);
	UE_API void HandleSettingApplied(UGameSetting* Setting);
	UE_API void HandleSettingEditConditionsChanged(UGameSetting* Setting);
	UE_API void HandleSettingNamedAction(UGameSetting* Setting, FGameplayTag GameSettings_Action_Tag);
	UE_API void HandleSettingNavigation(UGameSetting* Setting);

	UPROPERTY(Transient)
	TObjectPtr<ULocalPlayer> OwningLocalPlayer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSetting>> TopLevelSettings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSetting>> RegisteredSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> VideoSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> AudioSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GameplaySettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> MouseAndKeyboardSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GamepadSettings;

	FTSTicker::FDelegateHandle DLCTickHandle;
};

#undef UE_API
