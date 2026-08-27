// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "SunriseHUDComponent.generated.h"

class ASunriseHUD;

/** Composable presentation hook for mode-specific drawing on the single Sunrise HUD. */
UCLASS(Abstract, Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USunriseHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USunriseHUDComponent();
	virtual void DrawHUD(ASunriseHUD* HUD);
};
