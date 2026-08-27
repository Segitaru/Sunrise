// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "EFPawnData.generated.h"

#define UE_API EXPERIENCEFRAMEWORKCORE_API


class UEFPawnDataFragment;
class APawn;

class UEFCameraMode;
class UEFInputConfig;
class UObject;


/**
 * UEFPawnData
 *
 * Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "EF Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class UEFPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_API UEFPawnData(const FObjectInitializer& ObjectInitializer);

public:
	// Class to instantiate for this pawn (should usually derive from AEFPawn or AEFCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EF|Pawn")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced)
	TArray<TObjectPtr<UEFPawnDataFragment>> DataFragments;
};


#undef UE_API
