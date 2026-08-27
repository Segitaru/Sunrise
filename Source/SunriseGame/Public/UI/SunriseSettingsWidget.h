// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GameSettingRegistry.h"

#include "SunriseSettingsWidget.generated.h"

class SWidget;
class UGameSettingValue;
class UGameSettingValueDiscrete;
class UGameSettingValueScalar;
class UGameSettingsLocal;
class UGameSettingsShared;
class USunriseMainMenuWidget;

/** Concrete registry used by the native Sunrise settings screen. */
UCLASS()
class SUNRISEGAME_API USunriseGameSettingRegistry final : public UGameSettingRegistry
{
	GENERATED_BODY()
};

/** Native fallback settings UI backed by GameSettingsLocal and GameSettingsShared. */
UCLASS()
class SUNRISEGAME_API USunriseSettingsWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwningMenu(USunriseMainMenuWidget* InOwningMenu);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Settings")
	UGameSettingsLocal* GetGameLocalSettings() const { return LocalSettings; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Settings")
	UGameSettingsShared* GetGameSharedSettings() const { return SharedSettings; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply AdjustDiscreteSetting(TWeakObjectPtr<UGameSettingValueDiscrete> Setting, int32 Direction);
	FReply AdjustScalarSetting(TWeakObjectPtr<UGameSettingValueScalar> Setting, int32 Direction);
	FReply ResetToDefaults();
	FReply ApplyAndClose();
	FReply CancelAndClose();
	void CloseAndRestoreMenu();

	TWeakObjectPtr<USunriseMainMenuWidget> OwningMenu;

	UPROPERTY(Transient)
	TObjectPtr<USunriseGameSettingRegistry> Registry;

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingsLocal> LocalSettings;

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingsShared> SharedSettings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingValue>> EditableValues;
};
