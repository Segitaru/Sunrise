// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"

#include "SunriseFrontendComponent.generated.h"

class USunriseMainMenuWidget;

/** Frontend Experience component. Add it to GameState through a Game Feature action. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseFrontendComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<USunriseMainMenuWidget> MenuWidget;
};
