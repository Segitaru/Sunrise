#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

#include "SunriseBTTask_Attack.generated.h"

/** Attempts one weapon/GAS action; use a Wait node between retries. */
UCLASS()
class SUNRISEGAME_API USunriseBTTask_Attack : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USunriseBTTask_Attack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
