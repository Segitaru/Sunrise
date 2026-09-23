// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>

#include "DependentBuildingFragment.generated.h"

/**
* 
*/
UCLASS(DefaultToInstanced, EditInlineNew, Blueprintable, CollapseCategories)
class SUNRISEGAME_API UDependentBuildingFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void CheckDependency() {};
	virtual void ConfirmDependency() {};

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TSoftObjectPtr<UTexture2D> GetBuildingIcon() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	FGameplayTag BuildingType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	float Level = 0;
};
