#pragma once

#include "Components/ActorComponent.h"
#include "GameModes/Survival/Types/SurvivalResourceTypes.h"

#include "SurvivalWorkerComponent.generated.h"

class ASurvivalBuilding;
class ASunriseResourceNode;
class FLifetimeProperty;

/** Server gather/carry/deposit mechanics called by a GameplayAbility or BT task. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalWorkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalWorkerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	int32 GatherFrom(ASunriseResourceNode* Node);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	bool DepositAt(ASurvivalBuilding* Building);

	UFUNCTION(BlueprintPure, Category = "Survival|Worker")
	int32 GetCargoAmount() const { return CargoAmount; }

	UFUNCTION(BlueprintPure, Category = "Survival|Worker")
	ESurvivalResourceType GetCargoType() const { return CargoType; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1"))
	int32 CarryCapacity = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1"))
	int32 GatherAmount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1.0", Units = "cm"))
	float InteractionRange = 250.0f;

private:
	UPROPERTY(Replicated)
	ESurvivalResourceType CargoType = ESurvivalResourceType::Food;

	UPROPERTY(Replicated)
	int32 CargoAmount = 0;
};
