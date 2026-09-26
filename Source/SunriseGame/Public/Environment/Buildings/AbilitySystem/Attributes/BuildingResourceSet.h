// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <AbilitySystemComponent.h>

#include "AbilitySystem/Attribute/ModularAttributeSet.h"
#include "CoreMinimal.h"

#include "BuildingResourceSet.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class SUNRISEGAME_API UBuildingResourceSet : public UModularAttributeSet
{
	GENERATED_BODY()

public:
	UBuildingResourceSet();

	ATTRIBUTE_ACCESSORS(UBuildingResourceSet, Food);
	ATTRIBUTE_ACCESSORS(UBuildingResourceSet, Wood);
	ATTRIBUTE_ACCESSORS(UBuildingResourceSet, Stone);
	ATTRIBUTE_ACCESSORS(UBuildingResourceSet, Metal);

protected:
	UFUNCTION()
	void OnRep_Food(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Wood(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stone(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Metal(const FGameplayAttributeData& OldValue);

private:
	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Food, Category = "Resource|Building", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Food;
	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Wood, Category = "Resource|Building", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Wood;
	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stone, Category = "Resource|Building", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stone;
	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Metal, Category = "Resource|Building", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Metal;
};
