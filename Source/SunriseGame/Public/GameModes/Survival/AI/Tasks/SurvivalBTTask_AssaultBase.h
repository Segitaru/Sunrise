#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "SurvivalBTTask_AssaultBase.generated.h"

class ASurvivalBuilding;

/** Keeps a Survival raider moving toward the nearest living main base. */
UCLASS()
class SUNRISEGAME_API USurvivalBTTask_AssaultBase : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USurvivalBTTask_AssaultBase();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Survival|Raid", meta = (ClampMin = "1.0", Units = "cm"))
	float AcceptanceRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Survival|Raid", meta = (ClampMin = "0.1", Units = "s"))
	float RefreshInterval = 1.0f;

private:
	bool RefreshTarget(UBehaviorTreeComponent& OwnerComp);

	TWeakObjectPtr<ASurvivalBuilding> TargetBase;
	float RefreshTimeRemaining = 0.0f;
};
