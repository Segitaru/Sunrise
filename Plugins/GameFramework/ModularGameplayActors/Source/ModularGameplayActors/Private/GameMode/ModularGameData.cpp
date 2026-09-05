// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModularGameData.h"
#include "System/ModularAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularGameData)

UModularGameData::UModularGameData()
{
}

const UModularGameData& UModularGameData::Get()
{
	return UModularAssetManager::Get().GetGameData();
}
