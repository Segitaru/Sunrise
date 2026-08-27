// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Components/SunriseHUDComponent.h"

USunriseHUDComponent::USunriseHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USunriseHUDComponent::DrawHUD(ASunriseHUD* HUD)
{
	(void)HUD;
}
