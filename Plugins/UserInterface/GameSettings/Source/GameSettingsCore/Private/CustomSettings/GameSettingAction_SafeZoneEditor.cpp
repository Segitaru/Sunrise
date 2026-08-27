// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomSettings/GameSettingAction_SafeZoneEditor.h"

#include "GameSettingRegistry.h"
#include "GameSettingsLocal.h"
#include "Widgets/Layout/SSafeZone.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingAction_SafeZoneEditor)

class UGameSetting;

#define LOCTEXT_NAMESPACE "GameSettings"

UGameSettingAction_SafeZoneEditor::UGameSettingAction_SafeZoneEditor()
{
	SafeZoneValueSetting = NewObject<UGameSettingValueScalarDynamic_SafeZoneValue>();
	SafeZoneValueSetting->SetDevName(TEXT("SafeZoneValue"));
	SafeZoneValueSetting->SetDisplayName(LOCTEXT("SafeZoneValue_Name", "Safe Zone Value"));
	SafeZoneValueSetting->SetDescriptionRichText(LOCTEXT("SafeZoneValue_Description", "The safezone area percentage."));
	SafeZoneValueSetting->SetDefaultValue(0.0f);
	SafeZoneValueSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSafeZone));
	SafeZoneValueSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSafeZone));
	SafeZoneValueSetting->SetDisplayFormat(
		[](double SourceValue, double NormalizedValue)
		{
			return FText::AsNumber(SourceValue);
		});
	SafeZoneValueSetting->SetSettingParent(this);
}

TArray<UGameSetting*> UGameSettingAction_SafeZoneEditor::GetChildSettings()
{
	return {SafeZoneValueSetting};
}

void UGameSettingValueScalarDynamic_SafeZoneValue::ResetToDefault()
{
	Super::ResetToDefault();
	SSafeZone::SetGlobalSafeZoneScale(TOptional<float>(DefaultValue.Get(0.0f)));
}

void UGameSettingValueScalarDynamic_SafeZoneValue::RestoreToInitial()
{
	Super::RestoreToInitial();
	SSafeZone::SetGlobalSafeZoneScale(TOptional<float>(InitialValue));
}

#undef LOCTEXT_NAMESPACE
