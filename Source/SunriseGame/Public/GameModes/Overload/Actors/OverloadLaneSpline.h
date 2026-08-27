// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "OverloadLaneSpline.generated.h"

class AOverloadGuardTower;
class UOverloadWaveSpawnerComponent;
class USplineComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class SUNRISEGAME_API AOverloadLaneSpline : public AActor
{
	GENERATED_BODY()
public:
	AOverloadLaneSpline();
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Overload|Lane")
	USplineComponent* GetLaneSpline() const { return LaneSpline; }
	UFUNCTION(BlueprintPure, Category = "Overload|Lane")
	int32 GetSourceTeamId() const { return SourceTeamId; }
	UFUNCTION(BlueprintPure, Category = "Overload|Lane")
	int32 GetTargetTeamId() const { return TargetTeamId; }
	UFUNCTION(BlueprintPure, Category = "Overload|Lane")
	int32 GetCheckpointCount() const { return CheckpointCount; }
	UFUNCTION(BlueprintPure, Category = "Overload|Lane")
	float GetCheckpointDistance(int32 ZeroBasedIndex) const;

	/** 
	 * Repairs legacy Blueprint instances that predate the native wave-spawner subobject.
	 */
	UOverloadWaveSpawnerComponent* GetOrCreateWaveSpawner();
	UOverloadWaveSpawnerComponent* GetWaveSpawner() const { return WaveSpawner; }
	const TArray<TObjectPtr<AOverloadGuardTower>>& GetSpawnedTowers() const { return SpawnedTowerStorage; }

	void SetSpawnedTowers(const TArray<AOverloadGuardTower*>& Towers);
	AOverloadGuardTower* FindNextHostileTower(int32 UnitTeamId, float FromDistance, float TravelDirection) const;
	float FindTowerDistance(const AOverloadGuardTower* Tower) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category = "Overload|Lane")
	TObjectPtr<UOverloadWaveSpawnerComponent> WaveSpawner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Lane")
	TObjectPtr<USplineComponent> LaneSpline;
	/**
	 * Dummy markers make lane direction readable before final level art is available.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Lane")
	TObjectPtr<UStaticMeshComponent> StartMarkerMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Lane")
	TObjectPtr<UStaticMeshComponent> EndMarkerMesh;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Overload|Lane", meta = (ClampMin = "0"))
	int32 SourceTeamId = 0;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Overload|Lane", meta = (ClampMin = "0"))
	int32 TargetTeamId = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Lane", meta = (ClampMin = "1", ClampMax = "12"))
	int32 CheckpointCount = 5;

private:
	UPROPERTY()
	TArray<TObjectPtr<AOverloadGuardTower>> SpawnedTowerStorage;
};
