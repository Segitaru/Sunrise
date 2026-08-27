// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/Overload/Actors/Base/OverloadObjectiveBase.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "GameModes/Overload/Types/OverloadTypes.h"
#include "GameplayAbilitySpec.h"

#include "OverloadGuardTower.generated.h"

class AOverloadLaneSpline;
class AOverloadGuardTower;
class ASunriseUnit;
class UOverloadCaptureComponent;
class UOverloadTowerDefenseComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnOverloadTowerCaptured, AOverloadGuardTower*, Tower, int32, PreviousTeamId, int32, NewTeamId);

UCLASS(Blueprintable)
class SUNRISEGAME_API AOverloadGuardTower : public AOverloadObjectiveBase, public IOverloadHackable
{
	GENERATED_BODY()
public:
	AOverloadGuardTower();
	virtual void BeginPlay() override;
	void InitializeTower(int32 TeamId, int32 InTierIndex, AOverloadLaneSpline* InLane);
	void ApplyBalanceMultipliers(float DefenderMultiplier, float LeaderWeakeningMultiplier);
	void TryTowerAttack(ASunriseUnit* Target);
	void ExecuteTowerAttack();

	UFUNCTION(BlueprintPure, Category = "Overload|Tower")
	int32 GetTierIndex() const { return TierIndex; }
	UFUNCTION(BlueprintPure, Category = "Overload|Tower")
	AOverloadLaneSpline* GetLane() const { return Lane; }
	UFUNCTION(BlueprintPure, Category = "Overload|Tower")
	UOverloadCaptureComponent* GetCaptureComponent() const { return CaptureComponent; }

	virtual bool CanBeHackedByTeam_Implementation(int32 TeamId) const override;
	virtual FVector GetHackLocation_Implementation() const override;
	virtual void RegisterHacker_Implementation(ASunriseUnit* Unit) override;
	virtual void UnregisterHacker_Implementation(ASunriseUnit* Unit) override;

	UPROPERTY(BlueprintAssignable, Category = "Overload|Tower")
	FOnOverloadTowerCaptured OnTowerCaptured;

protected:
	UFUNCTION()
	void HandleCaptured(int32 PreviousTeamId, int32 NewTeamId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<USceneComponent> TerminalPoint;
	/** Temporary terminal visualization that can be replaced in a Blueprint child. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UStaticMeshComponent> TerminalVisualMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadCaptureComponent> CaptureComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadTowerDefenseComponent> DefenseComponent;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Balance")
	FOverloadTowerTierStats TierOneStats;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Balance", meta = (ClampMin = "0.0"))
	float PerTierStatGrowth = 0.22f;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Overload|Tower")
	int32 TierIndex = 1;
	UPROPERTY()
	TObjectPtr<AOverloadLaneSpline> Lane;
	UPROPERTY()
	TObjectPtr<ASunriseUnit> PendingAttackTarget;
	FGameplayAbilitySpecHandle AttackAbilityHandle;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Overload|Presentation", meta = (DisplayName = "Tower Attack"))
	void BP_TowerAttack(ASunriseUnit* Target);
};
