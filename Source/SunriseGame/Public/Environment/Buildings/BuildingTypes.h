// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "BuildingTypes.generated.h"

SUNRISEGAME_API DECLARE_LOG_CATEGORY_EXTERN(LogGameBuilder, Log, All);

USTRUCT(BlueprintType)
struct SUNRISEGAME_API FGameplayAbilityTargetData_ActorPlacement : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	FGameplayAbilityTargetData_ActorPlacement() {}

	FGameplayAbilityTargetData_ActorPlacement(TSoftClassPtr<AActor> InPlacementActorClass)
		: PlacementActorClass(MoveTemp(InPlacementActorClass))
	{
	}

	/** Hit result that stores data */
	UPROPERTY()
	TSoftClassPtr<AActor> PlacementActorClass;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override { return FGameplayAbilityTargetData_ActorPlacement::StaticStruct(); }
};

template <>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_ActorPlacement>
	: TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_ActorPlacement>
{
	enum
	{
		WithNetSerializer = true // For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};

USTRUCT(BlueprintType)
struct FInstigatorOnlyPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	AActor* Instigator;
};

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UBuildingTypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 GetLocalUserId(AActor* FromActor);

	static FGameplayAbilityTargetData_ActorPlacement* ExtractActorPlacementData(FGameplayAbilityTargetDataHandle InTargetData);

	UFUNCTION(BlueprintCallable)
	static TSoftClassPtr<AActor> GetPlacementActorFromTargetData(FGameplayAbilityTargetDataHandle InTargetData);
};
