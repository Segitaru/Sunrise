// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/EFGameData.h"

#include "System/EFAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFGameData)

UEFGameData::UEFGameData()
{
}

const UEFGameData& UEFGameData::UEFGameData::Get()
{
	return UEFAssetManager::Get().GetGameData();
}
