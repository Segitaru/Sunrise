#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "SunriseBTTask_RequestLaneMove.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseBTTask_RequestLaneMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USunriseBTTask_RequestLaneMove();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};