// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "GameRosterLoadMessage.generated.h"

class UModularPawnData;

// Represents a generic message of the form Instigator Verb Target (in Context, with Magnitude)
USTRUCT(BlueprintType)
struct SUNRISEGAME_API FGameRosterLoadMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TArray<TObjectPtr<UModularPawnData>> CurrentRoster;

	// Returns a debug string representation of this message
	FString ToString() const;
};
