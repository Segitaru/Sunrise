// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "GamePawnConfirmationMessage.generated.h"

class UModularPawnData;

// Represents a generic message of the form Instigator Verb Target (in Context, with Magnitude)
USTRUCT(BlueprintType)
struct SUNRISEGAME_API FGamePawnConfirmationMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TObjectPtr<const UObject> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TObjectPtr<const UModularPawnData> SelectedPawn = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bCharacterConfirmed = false;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bIsAutoSelect = false;

	// Returns a debug string representation of this message
	FString ToString() const;
};
