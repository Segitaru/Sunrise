// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "SunriseTouchControls.generated.h"

class ASunrisePlayerController;

/**
 *  Base class for additional touchscreen controls for a strategy game.
 *  Exposes some game commands to UI
 */
UCLASS(abstract)
class SUNRISEGAME_API USunriseTouchControls : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Pointer to the owning Sunrise PC */
	TObjectPtr<ASunrisePlayerController> PlayerController;

public:
	/** Sets the owning Sunrise PC pointer */
	void SetPlayerController(ASunrisePlayerController* PC);

	/** Syncs the camera zoom percentage with the UI. Called by the owning PC */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Set Zoom Percentage"))
	void BP_SetZoomPercentage(float Percentage);

protected:
	/** Resets the camera zoom level */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetZoom();

	/** Toggles between select all units and deselect all units. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleSelectAllUnits();

	/** Sets the camera zoom percentage level */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetZoomPercentage(float Percentage);
};
