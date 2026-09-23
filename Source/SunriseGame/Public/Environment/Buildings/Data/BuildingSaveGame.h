// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/SaveGame.h>

#include "Data/BuildingSaveData.h"

#include "BuildingSaveGame.generated.h"


/**
 * 
 */
UCLASS(Blueprintable)
class SUNRISEGAME_API UBuildingSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, FBuildingInfo> BuildingInfo;
};
