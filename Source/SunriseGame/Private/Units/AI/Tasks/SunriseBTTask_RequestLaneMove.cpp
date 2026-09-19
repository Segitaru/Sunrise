#include "Units/AI/Tasks/SunriseBTTask_RequestLaneMove.h"

#include <AIController.h>

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "Units/SunriseUnit.h"

USunriseBTTask_RequestLaneMove::USunriseBTTask_RequestLaneMove()
{
	NodeName = TEXT("Sunrise Request Lane Move");
}

EBTNodeResult::Type USunriseBTTask_RequestLaneMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASunriseUnit* Unit = Cast<ASunriseUnit>(OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr);
	UOverloadLaneFollowerComponent* Follower = Unit ? Unit->FindComponentByClass<UOverloadLaneFollowerComponent>() : nullptr;
	return Follower && Follower->RequestNextMove() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}