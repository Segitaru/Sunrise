// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickModeRule.h"
#include "PickModeRule_FFA.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UPickModeRule_FFA : public UPickModeRule
{
	GENERATED_BODY()

	virtual bool TryTakePawnFromPool(const UObject* Instigator, const TObjectPtr<UModularPawnData> TakingPawn,
		int32& PoolId, TObjectPtr<UModularPawnData>& ReleasedPawn) override;

	virtual bool TryTakeRandomPawnFromPool(const UObject* Instigator, int32& PoolId,
		TObjectPtr<UModularPawnData>& TakingPawn, TObjectPtr<UModularPawnData>& ReleasedPawn) override;
};
