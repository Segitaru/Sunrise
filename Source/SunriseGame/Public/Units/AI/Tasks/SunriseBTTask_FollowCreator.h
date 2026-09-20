#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "SunriseBTTask_FollowCreator.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseBTTask_FollowCreator : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USunriseBTTask_FollowCreator();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "0.0", Units = "cm"))
	float FollowRadius = 220.0f;
};