// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "PickModeRule.generated.h"

class UGamePawnRosterComponent;
class UModularPawnData;

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UPickModeRule : public UObject
{
	GENERATED_BODY()

public:
	UPickModeRule(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UGamePawnRosterComponent* GetPawnManagerComponent() const;

	bool GetTeamIdFromTarget(const UObject* TargetObject, int32& CurrentTeamId) const;

	virtual bool TryTakePawnFromPool(const UObject* Instigator, const TObjectPtr<UModularPawnData> TakingPawn,
		int32& PoolId, TObjectPtr<UModularPawnData>& ReleasedPawn);

	virtual bool TryTakeRandomPawnFromPool(const UObject* Instigator, int32& PoolId,
		TObjectPtr<UModularPawnData>& TakingPawn, TObjectPtr<UModularPawnData>& ReleasedPawn);

	virtual bool TryTakePawnFromPoolByClass(const UObject* Instigator, int32& PoolId,
		TSubclassOf<UObject> ClassToSearch, TObjectPtr<UModularPawnData>& TakingPawn,
		TObjectPtr<UModularPawnData>& ReleasedPawn);

	virtual bool OnPawnConfirmed(const UObject* Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn,
		int32& PoolId, TArray<TObjectPtr<UModularPawnData>>& BlockedPawn);

	virtual bool OnPawnReleased(const UObject* Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn,
		int32& PoolId, TArray<TObjectPtr<UModularPawnData>>& BlockedPawn);
};
