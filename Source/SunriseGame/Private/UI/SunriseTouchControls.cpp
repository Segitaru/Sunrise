// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/SunriseTouchControls.h"

#include "Player/SunrisePlayerController.h"

void USunriseTouchControls::SetPlayerController(ASunrisePlayerController* PC)
{
	PlayerController = PC;
}

void USunriseTouchControls::ResetZoom()
{
	if (PlayerController)
	{
		PlayerController->DoCameraResetZoomCommand();

		BP_SetZoomPercentage(PlayerController->GetDefaultZoomPercentage());
	}
}

void USunriseTouchControls::ToggleSelectAllUnits()
{
	if (PlayerController)
	{
		PlayerController->DoToggleSelectAllUnitsCommand();
	}
}

void USunriseTouchControls::SetZoomPercentage(float Percentage)
{
	if (PlayerController)
	{
		PlayerController->DoCameraSetZoomPercentageCommand(Percentage);
	}
}