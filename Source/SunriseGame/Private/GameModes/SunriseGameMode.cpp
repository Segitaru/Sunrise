// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/SunriseGameMode.h"

#include "GameModes/SunriseGameState.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"
#include "Units/SunrisePawn.h"

ASunriseGameMode::ASunriseGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ASunriseGameState::StaticClass();
	DefaultPawnClass = ASunrisePawn::StaticClass();
	PlayerControllerClass = ASunrisePlayerController::StaticClass();
	HUDClass = ASunriseHUD::StaticClass();
}
