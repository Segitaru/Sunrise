// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "ResourceCostFragment.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Blueprintable, CollapseCategories)
class SUNRISEGAME_API UResourceCostFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void CheckCost() {};
	virtual void ApplyCost() {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	FGameplayTag Type;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	float Cost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText Title = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FLinearColor TabColor = FLinearColor::Blue;
};
