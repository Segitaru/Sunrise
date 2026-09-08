// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"

#include "GameLocalPlayer.generated.h"

#define UE_API GAMESETTINGSCORE_API
class UGameSettingsLocal;
class UGameSettingsShared;

/** Local-player bridge used by reflection-backed game-setting data sources. */
UCLASS(MinimalAPI, config = Engine, transient)
class UGameLocalPlayer : public UCommonLocalPlayer
{
	GENERATED_BODY()

public:
	UFUNCTION()
	UGameSettingsLocal* GetLocalSettings();

	UFUNCTION()
	UGameSettingsShared* GetSharedSettings();

	/** Starts an async request to load the shared settings, this will call OnSharedSettingsLoaded after loading or creating new ones */
	UE_API void LoadSharedSettingsFromDisk(bool bForceLoad = false);

protected:
	UE_API void OnSharedSettingsLoaded(UGameSettingsShared* LoadedOrCreatedSettings);

	UPROPERTY(Transient)
	mutable TObjectPtr<UGameSettingsShared> SharedSettings;
	FUniqueNetIdRepl NetIdForSharedSettings;
};
#undef UE_API