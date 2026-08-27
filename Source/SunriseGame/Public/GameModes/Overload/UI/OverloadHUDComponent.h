// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Components/SunriseHUDComponent.h"

#include "OverloadHUDComponent.generated.h"

class AOverloadEnergyCore;
class AOverloadGuardTower;
class ASunriseHUD;
class UOverloadGameMatchComponent;

/** Native, asset-independent Overload presentation composed onto ASunriseHUD. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UOverloadHUDComponent : public USunriseHUDComponent
{
	GENERATED_BODY()

public:
	virtual void DrawHUD(ASunriseHUD* HUD) override;

private:
	void DrawCoreOverview(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode);
	void DrawLaneOverview(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode);
	void DrawObjectiveWorldOverlays(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode);
	void DrawCoreWorldOverlay(ASunriseHUD* HUD, const AOverloadEnergyCore* Core);
	void DrawTowerWorldOverlay(ASunriseHUD* HUD, const AOverloadGuardTower* Tower);
	static void DrawProgressBar(
		ASunriseHUD* HUD, float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor);

	static FLinearColor GetTeamColor(int32 TeamId);
	static FString GetTeamLabel(int32 TeamId);
};
