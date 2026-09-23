// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Components/SunriseHUDComponent.h"

#include "SunriseHeroAbilityHUDComponent.generated.h"

class ASunriseHUD;

/** Shared selected-hero ability presentation for every Sunrise game mode. */
UCLASS()
class SUNRISEGAME_API USunriseHeroAbilityHUDComponent : public USunriseHUDComponent
{
	GENERATED_BODY()

public:
	virtual void DrawHUD(ASunriseHUD* HUD) override;
};
