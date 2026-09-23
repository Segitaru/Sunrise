// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <AttributeSet.h>
#include <CoreMinimal.h>

#include "Environment/PlaceableActor.h"

#include "GameBuilding.generated.h"

USTRUCT(Blueprintable)
struct FConstructionParameters
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bRequireBuild = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAutoConstruction = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ConstructionTime = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSoftObjectPtr<UStaticMesh>> VisualStages;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateBuildingInfo, AGameBuilding*, FromBuilding);

UCLASS()
class SUNRISEGAME_API AGameBuilding : public APlaceableActor
{
	GENERATED_BODY()

public:
	AGameBuilding();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void SetPlacementMode();

	UFUNCTION(BlueprintCallable)
	void SetBuildingMode();

	UFUNCTION(BlueprintCallable)
	void UpdateBuildProgress();

	UFUNCTION(BlueprintCallable)
	void UpdateVisualState();

	UFUNCTION(BlueprintCallable)
	void FinishBuilding();

	UPROPERTY(BlueprintAssignable)
	FOnUpdateBuildingInfo OnUpdateBuildingInfo;

	TMap<FGameplayAttribute, float> GetVitalityValues() const;

	void GetCurrentState(float& Level, float& Progress) const;
	void SetCurrentState(const float& Level, const float& Progress);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<float, FConstructionParameters> ConstructionStages;

	UPROPERTY(BlueprintReadWrite)
	float ConstructionProgress = 0.f;

	UFUNCTION(BlueprintImplementableEvent)
	void K2_SetPlacementMode();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_UpdateBuildProgress();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_UpdateVisualState();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_SetBuildingMode();

	UFUNCTION(BlueprintImplementableEvent)
	void OnFinishBuilding();

	bool bWasLoaded = false;

private:
	float LastInteractionTime = -1.f;
	FTimerHandle ConstructionHandle;
};
