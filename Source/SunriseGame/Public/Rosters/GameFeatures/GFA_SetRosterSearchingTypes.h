// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Engine/AssetManagerTypes.h"
#include "GFA_SetRosterSearchingTypes.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Set Roster Searching Types"))
class SUNRISEGAME_API UGFA_SetRosterSearchingTypes : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FPrimaryAssetType> SearchingAssetsTypesToAdd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FPrimaryAssetType> SearchingAssetsTypesToRemove;
};
