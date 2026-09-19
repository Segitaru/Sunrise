#include "Units/AI/Tasks/SunriseBTTask_MoveTo.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Units/AI/SunriseUnitAIController.h"

USunriseBTTask_MoveTo::USunriseBTTask_MoveTo()
{
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
	bObserveBlackboardValue = true;
	bAllowPartialPath = false;
	ObservedBlackboardValueTolerance = 1.0f;
}

EBTNodeResult::Type USunriseBTTask_MoveTo::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (const ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(OwnerComp.GetAIOwner()))
	{
		OrderRevision = AI->GetOrderRevision();
	}
	return Super::PerformMoveTask(OwnerComp, NodeMemory);
}

UAITask_MoveTo* USunriseBTTask_MoveTo::PrepareMoveTask(
	UBehaviorTreeComponent& OwnerComp, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest)
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	const UAbilitySystemComponent* ASC = AI ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AI->GetPawn()) : nullptr;
	if (ASC && AcceptableRadiusAttribute.IsValid() && ASC->HasAttributeSetForAttribute(AcceptableRadiusAttribute))
	{
		const float Radius = ASC->GetNumericAttribute(AcceptableRadiusAttribute);
		if (FMath::IsFinite(Radius))
		{
			// The request belongs to this pawn; never mutate the shared authored node's AcceptableRadius.
			MoveRequest.SetAcceptanceRadius(FMath::Max(0.0f, Radius) * 0.95f);
			MoveRequest.SetReachTestIncludesAgentRadius(false);
			MoveRequest.SetReachTestIncludesGoalRadius(false);
		}
	}
	return Super::PrepareMoveTask(OwnerComp, ExistingTask, MoveRequest);
}

void USunriseBTTask_MoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	if (TaskResult == EBTNodeResult::Succeeded || TaskResult == EBTNodeResult::Failed)
	{
		if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(OwnerComp.GetAIOwner()))
		{
			AI->CompleteMoveOrder(GetSelectedBlackboardKey(), OrderRevision, TaskResult == EBTNodeResult::Succeeded);
		}
	}
}
