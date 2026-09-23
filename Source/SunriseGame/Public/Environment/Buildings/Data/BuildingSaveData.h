// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AttributeSet.h"
#include "CoreMinimal.h"

#include "BuildingSaveData.generated.h"

struct FGameplayTagContainer;
class AGameBuilding;

USTRUCT(BlueprintType)
struct SUNRISEGAME_API FBuildingInfo
{
	GENERATED_BODY()

	FBuildingInfo() {}

	FBuildingInfo(const AGameBuilding* UpdatedBuilding);

	UPROPERTY(BlueprintReadOnly)
	FTransform Transform;

	UPROPERTY(BlueprintReadOnly)
	TSoftClassPtr<AGameBuilding> BuildingType = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float CurrentLevel = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float BuildingProgress = 0.f;

	UPROPERTY(BlueprintReadOnly)
	TMap<FGameplayAttribute, float> VitalityState;
};
