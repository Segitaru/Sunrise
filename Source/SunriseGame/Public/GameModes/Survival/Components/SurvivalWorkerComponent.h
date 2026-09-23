#pragma once

#include "Components/ActorComponent.h"
#include "GameModes/Survival/Types/SurvivalResourceTypes.h"

#include "SurvivalWorkerComponent.generated.h"

class ASurvivalBuilding;
class ASunriseResourceNode;
class FLifetimeProperty;

/** Server-owned gather, carry, return, deposit and construction loop for a Survival worker. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalWorkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivalWorkerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	int32 GatherFrom(ASunriseResourceNode* Node);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	bool DepositAt(ASurvivalBuilding* Building);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	bool StartGatherOrder(ASunriseResourceNode* Node);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	bool StartBuildOrder(ASurvivalBuilding* Building);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Survival|Worker")
	void CancelWorkerOrder();

	UFUNCTION(BlueprintPure, Category = "Survival|Worker")
	int32 GetCargoAmount() const { return CargoAmount; }

	UFUNCTION(BlueprintPure, Category = "Survival|Worker")
	ESurvivalResourceType GetCargoType() const { return CargoType; }

protected:
	ASurvivalBuilding* FindClosestDepositBuilding() const;
	void MoveOwnerTo(AActor* Target);

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1"))
	int32 CarryCapacity = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1"))
	int32 GatherAmount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Survival|Worker", meta = (ClampMin = "1.0", Units = "cm"))
	float InteractionRange = 250.0f;

private:
	enum class EWorkerOrderState : uint8
	{
		None,
		Gathering,
		Returning,
		Constructing
	};

	UPROPERTY(Replicated)
	ESurvivalResourceType CargoType = ESurvivalResourceType::Food;

	UPROPERTY(Replicated)
	int32 CargoAmount = 0;

	TWeakObjectPtr<ASunriseResourceNode> TargetNode;
	TWeakObjectPtr<ASurvivalBuilding> TargetDeposit;
	TWeakObjectPtr<ASurvivalBuilding> TargetBuilding;
	EWorkerOrderState WorkerOrderState = EWorkerOrderState::None;
	float MoveRefreshRemaining = 0.0f;
};
