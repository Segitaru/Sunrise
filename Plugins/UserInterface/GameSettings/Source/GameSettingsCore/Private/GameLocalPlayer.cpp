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
