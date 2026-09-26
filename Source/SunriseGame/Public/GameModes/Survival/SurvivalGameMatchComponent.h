#pragma once

#include "GameFeatures/Components/ModularGameMatchComponent.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalGameMatchComponent.generated.h"

class APlayerState;
class ASunriseUnit;
class ASurvivalBuilding;
class UExperienceDefinition;
class UModularPawnData;
class USunriseUnitManagerComponent;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivalMatchStateChanged, ESurvivalMatchState, NewState);

/** Server-authoritative Survival setup, waves, base recovery, victory and defeat. */
UCLASS(Blueprintable, BlueprintType)
class SUNRISEGAME_API USurvivalGameMatchComponent : public UModularGameMatchComponent
{
	GENERATED_BODY()

public:
	USurvivalGameMatchComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static USurvivalGameMatchComponent* Find(const UObject* WorldContextObject);

	void RegisterBuilding(ASurvivalBuilding* Building);
	void UnregisterBuilding(ASurvivalBuilding* Building);

	UFUNCTION(BlueprintPure, Category = "Survival")
	ESurvivalMatchState GetSurvivalMatchState() const { return MatchState; }

	UFUNCTION(BlueprintPure, Category = "Survival")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category = "Survival")
	int32 GetTotalWaves() const { return Waves.Num(); }

	UFUNCTION(BlueprintPure, Category = "Survival")
	float GetSecondsUntilNextWave() const;

	UFUNCTION(BlueprintPure, Category = "Survival")
	int32 GetAliveMainBaseCount() const;

	UFUNCTION(BlueprintPure, Category = "Survival")
	int32 GetAliveWorkerCount() const;

	UFUNCTION(BlueprintPure, Category = "Survival")
	int32 GetAliveWaveEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Survival")
	ASurvivalBuilding* GetClosestLivingMainBase(const FVector& Location) const;

	TSoftObjectPtr<UModularPawnData> GetStartingWorkerDefinition() const { return StartingWorkerDefinition; }
	void RegisterProducedWorker(ASunriseUnit* Worker);

	UPROPERTY(BlueprintAssignable, Category = "Survival")
	FOnSurvivalMatchStateChanged OnMatchStateChanged;

protected:
	void HandleExperienceLoaded(const UExperienceDefinition* CurrentExperience);
	void InitializeMode();
	void InitializePlayer(APlayerState* PlayerState, int32 PlayerIndex);
	void StartNextWave();
	void UpdateMatch();
	void SetMatchState(ESurvivalMatchState NewState);
	void EvaluateDefeatCondition();
	void HandleUnitDied(ASunriseUnit* Unit);
	ASurvivalBuilding* FindClosestLivingBase(const FVector& Location) const;
	bool HasRecoveryPath() const;
	int32 ResolveEnemyTeamId() const;
	bool HasLivingWorkerForTeam(int32 TeamId) const;
	USunriseUnitManagerComponent* GetUnitManager() const;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Setup")
	TSubclassOf<ASurvivalBuilding> MainBaseClass;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Setup")
	TSoftObjectPtr<UModularPawnData> StartingWorkerDefinition;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Setup", meta = (ClampMin = "1"))
	int32 StartingWorkerCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Setup", meta = (Units = "cm"))
	FVector MainBaseOffset = FVector(700.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Waves")
	TArray<FSurvivalWaveDefinition> Waves;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Waves")
	FName WaveSpawnActorTag = TEXT("SurvivalWaveSpawn");

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Waves")
	int32 EnemyTeamId = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Waves", meta = (ClampMin = "0.0", Units = "cm"))
	float FallbackWaveSpawnDistance = 6000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Raid", meta = (ClampMin = "1.0", Units = "cm"))
	float RaidDamageRange = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Raid", meta = (ClampMin = "0.0"))
	float RaidDamagePerSecond = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Setup", meta = (ClampMin = "0.1", Units = "s"))
	float UpdateInterval = 0.5f;

private:
	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	ESurvivalMatchState MatchState = ESurvivalMatchState::Initializing;

	UPROPERTY(Replicated)
	int32 CurrentWave = 0;

	UPROPERTY(Replicated)
	float NextWaveServerTime = -1.0f;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<ASurvivalBuilding>> MainBases;

	UPROPERTY()
	TArray<TObjectPtr<ASunriseUnit>> Workers;

	TSet<TWeakObjectPtr<ASunriseUnit>> StartingPopulationWorkers;

	UPROPERTY()
	TArray<TObjectPtr<ASunriseUnit>> ActiveWaveUnits;

	FTimerHandle InitializationTimer;
	FTimerHandle UpdateTimer;
	bool bPlayersInitialized = false;
	int32 ResolvedEnemyTeamId = INDEX_NONE;
};
