// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include "GamePoolChangesMessage.generated.h"

class UModularPawnData;

// Represents a generic message of the form Instigator Verb Target (in Context, with Magnitude)
USTRUCT(BlueprintType)
struct SUNRISEGAME_API FGamePoolChangesMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	int32 PoolID = -1;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	bool bIsLock = false;

	UPROPERTY(BlueprintReadWrite, Category = Gameplay)
	TObjectPtr<const UModularPawnData> ChangedPawn = nullptr;

	// Returns a debug string representation of this message
	FString ToString() const;
};
