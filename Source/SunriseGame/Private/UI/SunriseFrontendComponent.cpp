// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseFrontendComponent.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameSettingsLocal.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SunriseWidgets.h"

void USunriseFrontendComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}
	UGameSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(true);
	MenuWidget = CreateWidget<USunriseMainMenuWidget>(PlayerController, USunriseMainMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, MenuWidget, EMouseLockMode::DoNotLock);
		PlayerController->bShowMouseCursor = true;
	}
}
