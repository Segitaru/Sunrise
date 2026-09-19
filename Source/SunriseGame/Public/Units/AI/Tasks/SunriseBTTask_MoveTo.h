#pragma once

#include "AttributeSet.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"

#include "SunriseBTTask_MoveTo.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseBTTask_MoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	USunriseBTTask_MoveTo();

protected:
	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual UAITask_MoveTo* PrepareMoveTask(
		UBehaviorTreeComponent& OwnerComp, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	UPROPERTY(config, Category = Node, EditAnywhere)
	FGameplayAttribute AcceptableRadiusAttribute;

	uint32 OrderRevision = 0;
};
