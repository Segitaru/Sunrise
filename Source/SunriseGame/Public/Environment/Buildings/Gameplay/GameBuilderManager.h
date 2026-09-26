// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/GameStateComponent.h>
#include <CoreMinimal.h>

#include "GameBuilderManager.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UGameBuilderManager : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, BlueprintCallable)
	static UGameBuilderManager* FindGameBuilderManager(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<UGameBuilderManager>() : nullptr);
	}
};
