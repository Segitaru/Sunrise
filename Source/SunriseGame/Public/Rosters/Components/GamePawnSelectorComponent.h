// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <NativeGameplayTags.h>

#include "Components/GameStateComponent.h"
#include "CoreMinimal.h"

#include "GamePawnSelectorComponent.generated.h"

namespace Rosters::Gameplay::Tags
{
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(PoolChange);
}

class UPickModeRule;
class UModularPawnData;
class UGamePawnRosterComponent;
class UExperienceDefinition;

UENUM(BlueprintType)
enum ECharacterPickMode : uint8
{
	WithoutMode,
	AllPick,
	FreeForAll
};

USTRUCT(BlueprintType)
struct FCharacterPickModeMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TEnumAsByte<ECharacterPickMode> PickMode = WithoutMode;
};

UCLASS()
class SUNRISEGAME_API UGamePawnSelectorComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UGamePawnSelectorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TryTakePawnFromPool(const TObjectPtr<UObject> Instigator, const FGameplayTag& InPawnDeclaration);

	UFUNCTION(BlueprintCallable)
	virtual void TryTakeRandomPawnFromPool(const UObject* Instigator);

	UFUNCTION(BlueprintCallable)
	virtual void TryTakePawnFromPoolByClass(const UObject* Instigator, TSubclassOf<UObject> ClassToSearch);

	virtual void TryLockPawnInPool(const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> LockingPawn);

	virtual void ReleasePawnIntoPool(const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ReleasePawn);

	virtual void OnPawnConfirmed(const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn);
	virtual void OnPawnReset(const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ReleasedPawn);

protected:
	virtual void BeginPlay() override;

	virtual void OnRosterLoaded();
	void CreatePoolsAfterExperienceLoaded(const UExperienceDefinition* Experience);
	void CreatePoolsForCurrentTeams();

	UFUNCTION(NetMulticast, Reliable)
	void SendPoolChange_Multicast(int32 PawnPoolId, bool bIsLocked, const UModularPawnData* ChangedPawn);

	UGamePawnRosterComponent* GetPawnManagerComponent() const;

private:
	UPROPERTY()
	TMap<TEnumAsByte<ECharacterPickMode>, TSubclassOf<UPickModeRule>> RuleForPickMode;

	UPROPERTY()
	TObjectPtr<UPickModeRule> CurrentPickRule;
};
