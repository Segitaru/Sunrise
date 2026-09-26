#pragma once

#include "Components/ActorComponent.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalProductionComponent.generated.h"

class ASunriseUnit;
class UModularPawnData;

USTRUCT(BlueprintType)
struct FSurvivalProductionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag UnitId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UModularPawnData> UnitDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSurvivalResourceAmounts Cost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 PopulationCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1", Units = "s"))
	float ProductionTime = 5.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSurvivalProductionChanged);

/** Server-authoritative production queue for a Survival production building. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalProductionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalProductionComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Survival|Production")
	void ServerQueueUnit(FGameplayTag UnitId);

	UFUNCTION(BlueprintPure, Category = "Survival|Production")
	const TArray<FGameplayTag>& GetQueue() const { return Queue; }

	UFUNCTION(BlueprintPure, Category = "Survival|Production")
	const TArray<FSurvivalProductionOption>& GetProductionOptions() const { return ProductionOptions; }

	UFUNCTION(BlueprintPure, Category = "Survival|Production")
	float GetCurrentCompletionServerTime() const { return CurrentCompletionServerTime; }

	UPROPERTY(BlueprintAssignable, Category = "Survival|Production")
	FOnSurvivalProductionChanged OnProductionChanged;

protected:
	const FSurvivalProductionOption* FindOption(FGameplayTag UnitId) const;
	void StartCurrentProduction();
	void FinishCurrentProduction();
	void HandleUnitDied(ASunriseUnit* Unit);
	void RefundQueuedUnits();

	UFUNCTION()
	void OnRep_Queue();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Production")
	TArray<FSurvivalProductionOption> ProductionOptions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Production", meta = (ClampMin = "1"))
	int32 MaxQueueSize = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Production", meta = (Units = "cm"))
	FVector SpawnOffset = FVector(300.0f, 0.0f, 50.0f);

private:
	UPROPERTY(ReplicatedUsing = OnRep_Queue)
	TArray<FGameplayTag> Queue;

	UPROPERTY(ReplicatedUsing = OnRep_Queue)
	float CurrentCompletionServerTime = -1.0f;

	UPROPERTY()
	TMap<TObjectPtr<ASunriseUnit>, int32> ProducedPopulation;

	FTimerHandle ProductionTimer;
};
