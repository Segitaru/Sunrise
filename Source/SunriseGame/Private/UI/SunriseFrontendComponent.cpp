// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseFrontendComponent.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameSettingsLocal.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SunriseWidgets.h"

void USunriseFrontendComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* const Controller = GetController<APlayerController>();
	UGameSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(true);
	MenuWidget = CreateWidget<USunriseMainMenuWidget>(Controller, USunriseMainMenuWidget::StaticClass());
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, MenuWidget, EMouseLockMode::DoNotLock);
		Controller->bShowMouseCursor = true;
	}
}
