// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingAction.h"
#include "GameSettingValueScalarDynamic.h"

#include "GameSettingAction_HDRCalibrationEditor.generated.h"

class UGameSetting;

UCLASS()
class UGameSettingAction_HDRCalibrationEditor : public UGameSettingAction
{
	GENERATED_BODY()

public:
	UGameSettingAction_HDRCalibrationEditor();
	virtual TArray<UGameSetting*> GetChildSettings() override;

private:
	UPROPERTY()
	TObjectPtr<UGameSettingValueScalarDynamic> HDRCalibrationValueSetting;
};
