// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameModes/Spawning/PawnFormationTypes.h"

#include "OverloadWaveSpawnerComponent.generated.h"

class AOverloadLaneSpline;
class ASunriseUnit;
class UModularPawnData;

UCLASS(ClassGroup = (Overload), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UOverloadWaveSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOverloadWaveSpawnerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Initialize(AOverloadLaneSpline* InLane, const TArray<FPawnFormation>& InFormations);

	void ApplyEnemyDifficulty(float CountMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Overload|Wave")
	void SpawnWave();

	UFUNCTION(BlueprintPure, Category = "Overload|Wave")
	float GetSecondsUntilNextWave() const;

	UFUNCTION(BlueprintPure, Category = "Overload|Wave")
	float GetWaveInterval() const { return WaveInterval; }

	UFUNCTION(BlueprintPure, Category = "Overload|Wave")
	int32 GetAliveUnitCount(int32 TeamId) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Wave", meta = (ClampMin = "1.0", Units = "s"))
	float InitialDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Wave", meta = (ClampMin = "5.0", Units = "s"))
	float WaveInterval = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Wave", meta = (ClampMin = "100.0", Units = "cm"))
	float UnitSpacing = 180.0f;

	/** Keeps the wave clear of the core and other endpoint geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Wave", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnInsetDistance = 500.0f;

	/** RVO still separates units, while Pawn overlap prevents hard deadlocks in crowds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Wave")
	bool bUseNonBlockingPawnCollision = true;

private:
	void SpawnWaveForTeam(int32 TeamId, bool bSpawnAtSplineStart);
	void PruneTrackedUnits();

	TWeakObjectPtr<AOverloadLaneSpline> Lane;

	UPROPERTY()
	TArray<FPawnFormation> CurrentFormations;

	TArray<TWeakObjectPtr<ASunriseUnit>> SpawnedUnits;
	FTimerHandle WaveTimer;

	UPROPERTY(Replicated)
	float EnemyWaveMultiplier = 1.0f;
};
