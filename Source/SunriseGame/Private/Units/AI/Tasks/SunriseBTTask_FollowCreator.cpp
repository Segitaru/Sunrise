#include "Units/AI/Tasks/SunriseBTTask_FollowCreator.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Units/AI/SunriseUnitAIController.h"
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
	const ASunriseUnitAIController* SunriseAI = Cast<ASunriseUnitAIController>(AIController);
	ASunriseUnit* Unit = AIController ? Cast<ASunriseUnit>(AIController->GetPawn()) : nullptr;
	AActor* Creator = Unit ? Unit->GetOwner() : nullptr;
	if (!AIController || !Unit || (SunriseAI && SunriseAI->HasActivePlayerOrder()) || !IsValid(Creator) || Creator == Unit)
	{
		return EBTNodeResult::Failed;
	}

	if (FVector::DistSquared2D(Unit->GetActorLocation(), Creator->GetActorLocation()) <= FMath::Square(FollowRadius))
	{
		// Keep the task active. Succeeded would leave the selector and stop following
		// as soon as the creator moves away again.
		return EBTNodeResult::InProgress;
	}

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(Creator, FollowRadius, true, true, false, nullptr, true);
	return MoveResult == EPathFollowingRequestResult::Failed ? EBTNodeResult::Failed : EBTNodeResult::InProgress;
}

void USunriseBTTask_FollowCreator::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	(void)DeltaSeconds;
	AAIController* AIController = OwnerComp.GetAIOwner();
	const ASunriseUnitAIController* SunriseAI = Cast<ASunriseUnitAIController>(AIController);
	ASunriseUnit* Unit = AIController ? Cast<ASunriseUnit>(AIController->GetPawn()) : nullptr;
	AActor* Creator = Unit ? Unit->GetOwner() : nullptr;
	if (!AIController || !Unit || (SunriseAI && SunriseAI->HasActivePlayerOrder()) || !IsValid(Creator) || Creator == Unit)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (FVector::DistSquared2D(Unit->GetActorLocation(), Creator->GetActorLocation()) <= FMath::Square(FollowRadius))
	{
		if (AIController->GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			AIController->StopMovement();
		}
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
		const ASunriseUnitAIController* SunriseAI = Cast<ASunriseUnitAIController>(AIController);
		if (!SunriseAI || !SunriseAI->HasActivePlayerOrder())
		{
			AIController->StopMovement();
		}
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
