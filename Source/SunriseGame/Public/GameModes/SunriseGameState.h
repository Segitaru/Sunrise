// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameMode/EFGameState.h"

#include "SunriseGameState.generated.h"

class USunriseUnitManagerComponent;

/** Product GameState that owns services shared by independently composed match rules. */
UCLASS()
class SUNRISEGAME_API ASunriseGameState : public AEFGameState
{
	GENERATED_BODY()

public:
	ASunriseGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	USunriseUnitManagerComponent* GetUnitManager() const { return UnitManager; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Units", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USunriseUnitManagerComponent> UnitManager;
};
