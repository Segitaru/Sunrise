#include "Units/AI/Tasks/SunriseBTTask_FollowCreator.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Units/SunriseUnit.h"

USunriseBTTask_FollowCreator::USunriseBTTask_FollowCreator()
{
	NodeName = TEXT("Sunrise Follow Creator");
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type USunriseBTTask_FollowCreator::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ASunriseUnit* Unit = AIController ? Cast<ASunriseUnit>(AIController->GetPawn()) : nullptr;
	AActor* Creator = Unit ? Unit->GetOwner() : nullptr;
	if (!AIController || !Unit || !IsValid(Creator) || Creator == Unit)
	{
		return EBTNodeResult::Failed;
	}

	if (FVector::DistSquared2D(Unit->GetActorLocation(), Creator->GetActorLocation()) <= FMath::Square(FollowRadius))
	{
		return EBTNodeResult::Succeeded;
	}

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(Creator, FollowRadius, true, true, false, nullptr, true);
	return MoveResult == EPathFollowingRequestResult::Failed ? EBTNodeResult::Failed : EBTNodeResult::InProgress;
}

void USunriseBTTask_FollowCreator::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ASunriseUnit* Unit = AIController ? Cast<ASunriseUnit>(AIController->GetPawn()) : nullptr;
	AActor* Creator = Unit ? Unit->GetOwner() : nullptr;
	if (!AIController || !Unit || !IsValid(Creator) || Creator == Unit)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (FVector::DistSquared2D(Unit->GetActorLocation(), Creator->GetActorLocation()) <= FMath::Square(FollowRadius))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		const EPathFollowingRequestResult::Type MoveResult =
			AIController->MoveToActor(Creator, FollowRadius, true, true, false, nullptr, true);
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}

void USunriseBTTask_FollowCreator::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}