// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "BuildingDefinition.generated.h"


class UDependentBuildingFragment;
class AGameBuilding;
class UResourceCostFragment;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SUNRISEGAME_API UBuildingDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	TSoftClassPtr<AGameBuilding> BuildingDefinitionClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General", Instanced)
	TArray<TObjectPtr<UResourceCostFragment>> ResourceCosts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General", Instanced)
	TArray<TObjectPtr<UDependentBuildingFragment>> DependentBuilds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FText Title = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FLinearColor TabColor = FLinearColor::Blue;
};
