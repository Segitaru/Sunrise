#pragma once

#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"

#include "SunriseBTDecorator_CanAttack.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseBTDecorator_CanAttack : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	USunriseBTDecorator_CanAttack();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
