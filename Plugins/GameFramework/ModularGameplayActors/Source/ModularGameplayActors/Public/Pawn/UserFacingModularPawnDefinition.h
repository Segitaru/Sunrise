// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "UserFacingModularPawnDefinition.generated.h"

/**
 *
 */
UCLASS(MinimalAPI, BlueprintType, Const,
	Meta = (DisplayName = "UI Data", ShortTooltip = "Data asset used to define cosmetic info about a Pawn."), CollapseCategories)
class UUserFacingModularPawnDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UUserFacingModularPawnDefinition(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText PawnDisplayedName = FText::FromString("None");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> PawnMiniIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> PawnFullBody;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Info;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Difficulty;
};
