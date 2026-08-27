// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ControllableEntitiesTypes.generated.h"

class IIControllableEntity;
/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UControllableEntitiesTypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "PlayerStateClass"))
	static APlayerState* GetControllingPlayerState(TSubclassOf<APlayerState> PlayerStateClass, UObject* Source);

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "PlayerStateClass"))
	static APlayerState* GetPlayerStateFromObject(TSubclassOf<APlayerState> PlayerStateClass, UObject* Source);
};
