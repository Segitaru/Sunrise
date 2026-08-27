// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomSettings/GameSettingValueDiscrete_MobileFPSType.h"

#include "GameSettingsLocal.h"
#include "Performance/GamePerformanceSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueDiscrete_MobileFPSType)

#define LOCTEXT_NAMESPACE "GameSettings"

UGameSettingValueDiscrete_MobileFPSType::UGameSettingValueDiscrete_MobileFPSType()
{
}

void UGameSettingValueDiscrete_MobileFPSType::OnInitialized()
{
	Super::OnInitialized();

	const UGamePlatformSpecificRenderingSettings* PlatformSettings = UGamePlatformSpecificRenderingSettings::Get();
	const UGameSettingsLocal* UserSettings = UGameSettingsLocal::Get();

	for (int32 TestLimit : PlatformSettings->MobileFrameRateLimits)
	{
		if (UGameSettingsLocal::IsSupportedMobileFramePace(TestLimit))
		{
			FPSOptions.Add(TestLimit, MakeLimitString(TestLimit));
		}
	}

	const int32 FirstFrameRateWithQualityLimit = UserSettings->GetFirstFrameRateWithQualityLimit();
	if (FirstFrameRateWithQualityLimit > 0)
	{
		SetWarningRichText(
			FText::Format(LOCTEXT("MobileFPSType_Note",
							  "<strong>Note: Changing the framerate setting to {0} or higher might lower your Quality Presets.</>"),
				MakeLimitString(FirstFrameRateWithQualityLimit)));
	}
}

int32 UGameSettingValueDiscrete_MobileFPSType::GetDefaultFPS() const
{
	return UGameSettingsLocal::GetDefaultMobileFrameRate();
}

FText UGameSettingValueDiscrete_MobileFPSType::MakeLimitString(int32 Number)
{
	return FText::Format(LOCTEXT("MobileFrameRateOption", "{0} FPS"), FText::AsNumber(Number));
}

void UGameSettingValueDiscrete_MobileFPSType::StoreInitial()
{
	InitialValue = GetValue();
}

void UGameSettingValueDiscrete_MobileFPSType::ResetToDefault()
{
	SetValue(GetDefaultFPS(), EGameSettingChangeReason::ResetToDefault);
}

void UGameSettingValueDiscrete_MobileFPSType::RestoreToInitial()
{
	SetValue(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UGameSettingValueDiscrete_MobileFPSType::SetDiscreteOptionByIndex(int32 Index)
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);

	int32 NewMode = FPSOptionsModes.IsValidIndex(Index) ? FPSOptionsModes[Index] : GetDefaultFPS();

	SetValue(NewMode, EGameSettingChangeReason::Change);
}

int32 UGameSettingValueDiscrete_MobileFPSType::GetDiscreteOptionIndex() const
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);
	return FPSOptionsModes.IndexOfByKey(GetValue());
}

TArray<FText> UGameSettingValueDiscrete_MobileFPSType::GetDiscreteOptions() const
{
	TArray<FText> Options;
	FPSOptions.GenerateValueArray(Options);

	return Options;
}

int32 UGameSettingValueDiscrete_MobileFPSType::GetValue() const
{
	return UGameSettingsLocal::Get()->GetDesiredMobileFrameRateLimit();
}

void UGameSettingValueDiscrete_MobileFPSType::SetValue(int32 NewLimitFPS, EGameSettingChangeReason InReason)
{
	UGameSettingsLocal::Get()->SetDesiredMobileFrameRateLimit(NewLimitFPS);

	NotifySettingChanged(InReason);
}

#undef LOCTEXT_NAMESPACE
