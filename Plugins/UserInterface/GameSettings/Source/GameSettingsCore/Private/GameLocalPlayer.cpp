// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameLocalPlayer.h"

#include "GameSettingsLocal.h"
#include "GameSettingsShared.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameLocalPlayer)

UGameSettingsLocal* UGameLocalPlayer::GetLocalSettings()
{
	return UGameSettingsLocal::Get();
}

UGameSettingsShared* UGameLocalPlayer::GetSharedSettings()
{
	return UGameSettingsShared::GetSharedSettings(this);
}

void UGameLocalPlayer::LoadSharedSettingsFromDisk(bool bForceLoad)
{
	FUniqueNetIdRepl CurrentNetId = GetCachedUniqueNetId();
	if (!bForceLoad && SharedSettings && CurrentNetId == NetIdForSharedSettings)
	{
		// Already loaded once, don't reload
		return;
	}

	ensure(UGameSettingsShared::AsyncLoadOrCreateSettings(
		this, UGameSettingsShared::FOnSettingsLoadedEvent::CreateUObject(this, &UGameLocalPlayer::OnSharedSettingsLoaded)));
}

void UGameLocalPlayer::OnSharedSettingsLoaded(UGameSettingsShared* LoadedOrCreatedSettings)
{
	// The settings are applied before it gets here
	if (ensure(LoadedOrCreatedSettings))
	{
		// This will replace the temporary or previously loaded object which will GC out normally
		SharedSettings = LoadedOrCreatedSettings;

		NetIdForSharedSettings = GetCachedUniqueNetId();
	}
}
