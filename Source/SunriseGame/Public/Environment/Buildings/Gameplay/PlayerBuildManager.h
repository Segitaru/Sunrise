// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>

#include "AsyncMixin.h"
#include "Components/PlayerStateComponent.h"

#include "PlayerBuildManager.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnPlayerBuilderReady);

class AGameBuilding;
class UBuildingSaveGame;

//TODO: I think it's bad, but dont know how make better now
inline int32 AIBuildingUserSlot = 100;

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UPlayerBuildManager : public UPlayerStateComponent, public FAsyncMixin
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, BlueprintCallable)
	static UPlayerBuildManager* FindPlayerBuildManager(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<UPlayerBuildManager>() : nullptr);
	}

	virtual void BeginPlay() override;
	void TryLoadInfoForBot();
	void TryLoadInfo();

	UFUNCTION(BlueprintCallable)
	void AddBuilding(AGameBuilding* Building);

	void TrySetReadyState();

	UFUNCTION(BlueprintCallable)
	virtual bool SpawnBuilding(TSubclassOf<AGameBuilding> BuildingClass, const FTransform& SpawnTransform, AGameBuilding*& SpawnedBuilding);

	FString GetSaveSlotName(const int32& FromUserIndex) const;

	/// Attention, this clear all saved info for user
	UFUNCTION(BlueprintCallable)
	void DEV_ClearBuildings();

	UFUNCTION()
	void OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	// Ensures the delegate is called once the experience has been loaded
	// If the experience has already loaded, calls the delegate immediately
	void CallOrRegister_OnPlayerBuilderReady(FOnPlayerBuilderReady::FDelegate&& Delegate);

	void LoadUserInfo();
	void SaveBuilderInfo();

	UFUNCTION(BlueprintCallable)
	void SaveBuildingInfo(AGameBuilding* UpdatedBuilding);

	UFUNCTION(BlueprintCallable)
	void ClearSaveInfo();
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsInfoWasStored();
	void AsyncCreateBuilding();

	UFUNCTION()
	void OnSaveGameLoaded(const FString& SlotName, const int32 InUserIndex, USaveGame* SaveObject);

	UFUNCTION()
	void OnSaveGameSaved(const FString& SlotName, const int32 InUserIndex, bool bIsSuccess);

	FOnPlayerBuilderReady OnPlayerBuilderReady;

	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APawn> InstigatingPawn;

protected:
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AGameBuilding>> Buildings;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<AGameBuilding> MainBuilding = nullptr;

	UPROPERTY()
	TObjectPtr<UBuildingSaveGame> BuildingSaveInfo = nullptr;

private:
	int32 UserIndex = INDEX_NONE;
	int32 LoadedBuildingIndex = 0;
	int32 MustBeLoaded = 0;

	bool bHaveStoredInfo = false;
};
