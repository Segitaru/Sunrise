// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/SunriseGameState.h"

#include "Units/Components/SunriseUnitManagerComponent.h"

ASunriseGameState::ASunriseGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnitManager = CreateDefaultSubobject<USunriseUnitManagerComponent>(TEXT("SunriseUnitManager"));
}
