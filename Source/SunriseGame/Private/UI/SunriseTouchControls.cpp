// Copyright Epic Games, Inc. All Rights Reserved.


#include "UI/SunriseTouchControls.h"

#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>

#include "Abilities/SunriseSelectionAbility.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunrisePawn.h"

void USunriseTouchControls::SetPlayerController(ASunrisePlayerController* PC)
{
	PlayerController = PC;
}

void USunriseTouchControls::ResetZoom()
{
	if (ASunrisePawn* Pawn = PlayerController ? PlayerController->GetPawn<ASunrisePawn>() : nullptr)
	{
		Pawn->DoCameraResetZoomCommand();
		BP_SetZoomPercentage(Pawn->GetZoomPercentage());
	}
}

void USunriseTouchControls::ToggleSelectAllUnits()
{
	const auto* const ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerController->GetPawn());
	if (!ASC)
	{
		return;
	}
	FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromClass(USunriseSelectionAbility::StaticClass());
	if (!AbilitySpec)
	{
		return;
	}
	if (USunriseSelectionAbility* Selection = Cast<USunriseSelectionAbility>(AbilitySpec->Ability); IsValid(Selection))
	{
		Selection->DoToggleSelectAllUnitsCommand();
	}
}

void USunriseTouchControls::SetZoomPercentage(float Percentage)
{
	if (ASunrisePawn* Pawn = PlayerController ? PlayerController->GetPawn<ASunrisePawn>() : nullptr)
	{
		Pawn->DoCameraSetZoomPercentageCommand(Percentage);
		BP_SetZoomPercentage(Pawn->GetZoomPercentage());
	}
}
