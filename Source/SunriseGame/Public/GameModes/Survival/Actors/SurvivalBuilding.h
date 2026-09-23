#pragma once

#include "Environment/Buildings/Gameplay/GameBuilding.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalBuilding.generated.h"

class UStaticMeshComponent;
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

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	ESurvivalBuildingRole GetBuildingRole() const { return BuildingRole; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	bool IsMainBase() const { return BuildingRole == ESurvivalBuildingRole::MainBase; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	const FSurvivalResourceAmounts& GetConstructionCost() const { return ConstructionCost; }

	UFUNCTION(BlueprintPure, Category = "Survival|Building")
	int32 GetPopulationCapacity() const { return PopulationCapacity; }

	UStaticMeshComponent* GetBuildingMeshComponent() const { return BuildingMesh; }
	void SetLocallySelected(bool bSelected);

protected:
	void ConfigureBasicShape(const FVector& RelativeScale, ESurvivalBuildingRole InRole, float MaxHealth, int32 InPopulationCapacity);
	UFUNCTION()
	void HandleBuildingVitalityStateChanged(AActor* OwningActor, EVitalityState OldState, EVitalityState NewState);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival|Building")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building")
	ESurvivalBuildingRole BuildingRole = ESurvivalBuildingRole::Generic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building")
	FSurvivalResourceAmounts ConstructionCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Building", meta = (ClampMin = "0"))
	int32 PopulationCapacity = 0;

private:
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
