// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Components/ControllerComponent.h>

#include "SunriseFrontendComponent.generated.h"

class USunriseMainMenuWidget;

/** Frontend Experience component. Add it to GameState through a Game Feature action. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseFrontendComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USunriseMainMenuWidget> MenuWidget;
};
