#include "GameModes/Survival/AI/Tasks/SurvivalBTTask_AssaultBase.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalBTTask_AssaultBase)

USurvivalBTTask_AssaultBase::USurvivalBTTask_AssaultBase()
{
	NodeName = TEXT("Survival: Assault Main Base");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

EBTNodeResult::Type USurvivalBTTask_AssaultBase::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8*)
{
	RefreshTimeRemaining = 0.0f;
	return RefreshTarget(OwnerComp) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

void USurvivalBTTask_AssaultBase::TickTask(UBehaviorTreeComponent& OwnerComp, uint8*, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	ASunriseUnit* Unit = AI ? Cast<ASunriseUnit>(AI->GetPawn()) : nullptr;
	USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(Unit);
	if (!Unit || !Unit->IsAlive() || !Match || Match->GetSurvivalMatchState() != ESurvivalMatchState::InProgress)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	RefreshTimeRemaining -= DeltaSeconds;
	if (!TargetBase.IsValid() || !TargetBase->IsAlive() || RefreshTimeRemaining <= 0.0f)
	{
		if (!RefreshTarget(OwnerComp))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

EBTNodeResult::Type USurvivalBTTask_AssaultBase::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8*)
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	TargetBase.Reset();
	return EBTNodeResult::Aborted;
}

bool USurvivalBTTask_AssaultBase::RefreshTarget(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	ASunriseUnit* Unit = AI ? Cast<ASunriseUnit>(AI->GetPawn()) : nullptr;
	USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(Unit);
	ASurvivalBuilding* Base = Match && Unit ? Match->GetClosestLivingMainBase(Unit->GetActorLocation()) : nullptr;
	if (!AI || !Unit || !Base)
	{
		TargetBase.Reset();
		return false;
	}

	TargetBase = Base;
	RefreshTimeRemaining = FMath::Max(0.1f, RefreshInterval);
	if (FVector::DistSquared(Unit->GetActorLocation(), Base->GetActorLocation()) > FMath::Square(AcceptanceRadius))
	{
		AI->MoveToActor(Base, AcceptanceRadius, true, true, true, nullptr, true);
	}
	else
	{
		AI->StopMovement();
	}
	return true;
}
