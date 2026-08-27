// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"

#include "GameLocalPlayer.generated.h"

class UGameSettingsLocal;
class UGameSettingsShared;

/** Local-player bridge used by reflection-backed game-setting data sources. */
UCLASS(config = Engine, transient)
class GAMESETTINGSCORE_API UGameLocalPlayer : public UCommonLocalPlayer
{
	GENERATED_BODY()

public:
	UFUNCTION()
	UGameSettingsLocal* GetLocalSettings();

	UFUNCTION()
	UGameSettingsShared* GetSharedSettings();
};
