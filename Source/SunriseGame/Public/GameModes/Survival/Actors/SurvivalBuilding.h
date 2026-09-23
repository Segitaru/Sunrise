#pragma once

#include "Environment/Buildings/Gameplay/GameBuilding.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalBuilding.generated.h"

class FLifetimeProperty;
class UStaticMeshComponent;
class UWidgetComponent;
class USurvivalProductionComponent;

/** Native basic-shape building used by the first Survival vertical slice. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASurvivalBuilding : public AGameBuilding
{
	GENERATED_BODY()

public:
	ASurvivalBuilding();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	ESurvivalBuildingRole GetBuildingRole() const { return BuildingRole; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	bool IsMainBase() const { return BuildingRole == ESurvivalBuildingRole::MainBase; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	const FSurvivalResourceAmounts& GetConstructionCost() const { return ConstructionCost; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	int32 GetPopulationCapacity() const { return PopulationCapacity; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	bool IsConstructionComplete() const { return !bUnderConstruction; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	float GetSurvivalConstructionProgress() const { return SurvivalConstructionProgress; }

	UStaticMeshComponent* GetBuildingMeshComponent() const { return BuildingMesh; }
	void SetLocallySelected(bool bSelected);

	/** Must be called on a deferred-spawned building before FinishSpawning. */
	void InitializeConstructionSite();
	/** Applies authoritative worker time and returns true once construction completes. */
	bool ApplyConstructionWork(float WorkSeconds);

protected:
	void ConfigureBasicShape(const FVector& RelativeScale, ESurvivalBuildingRole InRole, float MaxHealth, int32 InPopulationCapacity);
	void ApplyOperationalState();
	void UpdateConstructionPresentation();

	UFUNCTION()
	void HandleBuildingVitalityStateChanged(AActor* OwningActor, EVitalityState OldState, EVitalityState NewState);

	UFUNCTION()
	void OnRep_ConstructionState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival|Building")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival|Production")
	TObjectPtr<UWidgetComponent> ProductionQueueWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building")
	ESurvivalBuildingRole BuildingRole = ESurvivalBuildingRole::Generic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building")
	FSurvivalResourceAmounts ConstructionCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building", meta = (ClampMin = "0"))
	int32 PopulationCapacity = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building", meta = (ClampMin = "0.1", Units = "s"))
	float ConstructionDuration = 8.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_ConstructionState)
	bool bUnderConstruction = false;

	UPROPERTY(ReplicatedUsing = OnRep_ConstructionState)
	float SurvivalConstructionProgress = 1.0f;

	FVector FullBuildingMeshScale = FVector::OneVector;
	bool bEconomyCapacityApplied = false;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API ASurvivalMainBase : public ASurvivalBuilding
{
	GENERATED_BODY()
public:
	ASurvivalMainBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival|Production")
	TObjectPtr<USurvivalProductionComponent> ProductionComponent;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API ASurvivalBarracks : public ASurvivalBuilding
{
	GENERATED_BODY()
public:
	ASurvivalBarracks();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival|Production")
	TObjectPtr<USurvivalProductionComponent> ProductionComponent;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API ASurvivalHouse : public ASurvivalBuilding
{
	GENERATED_BODY()
public:
	ASurvivalHouse();
};

UCLASS(Blueprintable)
class SUNRISEGAME_API ASurvivalDefenseBuilding : public ASurvivalBuilding
{
	GENERATED_BODY()
public:
	ASurvivalDefenseBuilding();
};
