// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include "GamePawnSelectionMessage.generated.h"

class UModularPawnData;

// Represents a generic message of the form Instigator Verb Target (in Context, with Magnitude)
USTRUCT(BlueprintType)
struct SUNRISEGAME_API FGamePawnSelectionMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TObjectPtr<const AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TObjectPtr<const UModularPawnData> SelectedPawn = nullptr;

	// Returns a debug string representation of this message
	FString ToString() const;
};
