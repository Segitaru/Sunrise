#include "Units/AI/Decorators/SunriseBTDecorator_CanAttack.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Units/AI/SunriseUnitAIController.h"
#include "Units/SunriseUnit.h"

USunriseBTDecorator_CanAttack::USunriseBTDecorator_CanAttack()
{
	NodeName = TEXT("Sunrise Can Attack");
	FlowAbortMode = EBTFlowAbortMode::Both;
	bNotifyTick = true;
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USunriseBTDecorator_CanAttack, BlackboardKey), AActor::StaticClass());
}

bool USunriseBTDecorator_CanAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(OwnerComp.GetAIOwner());
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const ASunriseUnit* Target = BB ? Cast<ASunriseUnit>(BB->GetValueAsObject(GetSelectedBlackboardKey())) : nullptr;
	return AI && AI->CanAttackTarget(Target);
}

void USunriseBTDecorator_CanAttack::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}
