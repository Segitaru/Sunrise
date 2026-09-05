// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "GamePawnRosterComponent.generated.h"

class UExperienceDefinition;
class UModularPawnData;

namespace Rosters::Gameplay::Tags
{
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(RosterLoad);
}

DECLARE_MULTICAST_DELEGATE(FOnRosterLoaded);
DECLARE_MULTICAST_DELEGATE(FOnRosterReady);

USTRUCT(BlueprintType)
struct FRosterPool
{
	GENERATED_BODY();

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UModularPawnData>> AvailablePawnsFromRoster;
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UModularPawnData>> LockedPawnsInRoster;
};

UCLASS()
class SUNRISEGAME_API UGamePawnRosterComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UGamePawnRosterComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void CallOrRegister_OnRosterLoaded(FOnRosterLoaded::FDelegate&& Delegate);

	void CallOrRegister_OnRosterReady(FOnRosterReady::FDelegate&& Delegate);

	void CreatePools(TArray<int32> PoolIDs);

	virtual bool CheckContainPawnInPool(int32 PoolId, const UModularPawnData* PawnToCheck);

	virtual bool RemovePawnFromPool(int32 PoolId, const TObjectPtr<UModularPawnData> PawnToRemove);

	virtual bool ReleasePawnInPool(int32 PoolId, const TObjectPtr<UModularPawnData> ReturningPawn);

	virtual TObjectPtr<UModularPawnData> GetRandomPawnFromPool(int32 PoolId);
	virtual TObjectPtr<UModularPawnData> GetPawnFromPoolByClass(int32 PoolId, TSubclassOf<UObject> SearchClass);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentRoster)
	TArray<TObjectPtr<UModularPawnData>> CurrentRoster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|System")
	TArray<FPrimaryAssetType> SearchingAssetsTypes;

protected:
	UPROPERTY()
	TMap<int32, FRosterPool> RosterPoolsById;

	virtual void LoadRoster();

	virtual void BeginPlay() override;

	virtual void OnExperienceLoaded(const UExperienceDefinition* CurrentExperience);

	virtual bool GetPoolById(int32 PoolId, FRosterPool& ReceivedPool);

	UFUNCTION()
	void OnRep_CurrentRoster();

	FOnRosterLoaded OnRosterLoaded;
	FOnRosterReady OnRosterReady;
};
