#pragma once

#include "Components/PlayerStateComponent.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalEconomyComponent.generated.h"

class UAbilitySystemComponent;
class UBuildingResourceSet;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSurvivalEconomyChanged);

/** Server-owned resources and population for one Survival player. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalEconomyComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	USurvivalEconomyComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static USurvivalEconomyComponent* Find(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Survival|Economy")
	FSurvivalResourceAmounts GetResources() const;

	UFUNCTION(BlueprintPure, Category = "Survival|Economy")
	bool CanAfford(const FSurvivalResourceAmounts& Cost) const;

	bool TrySpend(const FSurvivalResourceAmounts& Cost);
	void AddResources(const FSurvivalResourceAmounts& Amounts);

	UFUNCTION(BlueprintPure, Category = "Survival|Economy")
	int32 GetPopulation() const { return Population; }

	UFUNCTION(BlueprintPure, Category = "Survival|Economy")
	int32 GetPopulationCap() const { return PopulationCap; }

	bool TryReservePopulation(int32 Amount);
	void ReleasePopulation(int32 Amount);
	void AddPopulationCapacity(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Survival|Economy")
	FOnSurvivalEconomyChanged OnEconomyChanged;

protected:
	UFUNCTION()
	void OnRep_Population();

	void HandleResourceChanged(const struct FOnAttributeChangeData& ChangeData);
	void ApplyResourceDelta(const FSurvivalResourceAmounts& Delta);

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Economy")
	FSurvivalResourceAmounts StartingResources = {200.0f, 300.0f, 150.0f, 50.0f};

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Economy", meta = (ClampMin = "0"))
	int32 StartingPopulationCap = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBuildingResourceSet> ResourceSet;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(ReplicatedUsing = OnRep_Population)
	int32 Population = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Population)
	int32 PopulationCap = 0;
};
