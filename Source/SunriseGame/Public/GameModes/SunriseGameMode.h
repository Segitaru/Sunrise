// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameMode/EFGameMode.h"

#include "SunriseGameMode.generated.h"

/** Single product GameMode provider. Gameplay rules are composed as GameState components by Experiences. */
UCLASS(Config = Game)
class SUNRISEGAME_API ASunriseGameMode : public AEFGameMode
{
	GENERATED_BODY()

public:
	ASunriseGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
