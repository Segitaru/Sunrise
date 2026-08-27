// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/Overload/Actors/Base/OverloadObjectiveBase.h"
#include "GameModes/Overload/Types/OverloadTypes.h"

#include "OverloadEnergyCore.generated.h"

struct FOnAttributeChangeData;
class AOverloadEnergyCore;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverloadCoreStateChanged, AOverloadEnergyCore*, Core, EOverloadCoreState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverloadCoreExploded, AOverloadEnergyCore*, Core, int32, OverloadingTeamId);

UCLASS(Blueprintable)
class SUNRISEGAME_API AOverloadEnergyCore : public AOverloadObjectiveBase
{
	GENERATED_BODY()
public:
	AOverloadEnergyCore();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeCore(int32 TeamId, float InOverloadSeconds, float InCoolingPerSecond);
	void SetSupplyCompromised(bool bCompromised, int32 InOverloadingTeamId);

	UFUNCTION(BlueprintPure, Category = "Overload|Core")
	EOverloadCoreState GetCoreState() const { return CoreState; }
	UFUNCTION(BlueprintPure, Category = "Overload|Core")
	float GetOverloadPercent() const;
	UFUNCTION(BlueprintPure, Category = "Overload|Core")
	int32 GetOverloadingTeamId() const { return OverloadingTeamId; }

	UPROPERTY(BlueprintAssignable, Category = "Overload|Core")
	FOnOverloadCoreStateChanged OnCoreStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "Overload|Core")
	FOnOverloadCoreExploded OnCoreExploded;

protected:
	void SetCoreState(EOverloadCoreState NewState);
	void HandleEnergyChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "Overload|Presentation", meta = (DisplayName = "Core State Changed"))
	void BP_CoreStateChanged(EOverloadCoreState NewState);
	UFUNCTION(BlueprintImplementableEvent, Category = "Overload|Presentation", meta = (DisplayName = "Core Exploded"))
	void BP_CoreExploded();

	UPROPERTY(EditDefaultsOnly, Category = "Overload|Core")
	float MaxIntegrity = 2500.0f;
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Overload|Core")
	EOverloadCoreState CoreState = EOverloadCoreState::Stable;
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Overload|Core")
	int32 OverloadingTeamId = INDEX_NONE;

private:
	float OverloadSeconds = 45.0f;
	float CoolingPerSecond = 1.5f;
};
