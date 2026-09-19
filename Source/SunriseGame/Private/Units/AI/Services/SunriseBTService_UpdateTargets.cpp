#include "Units/AI/Services/SunriseBTService_UpdateTargets.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Units/AI/SunriseUnitAIController.h"

USunriseBTService_UpdateTargets::USunriseBTService_UpdateTargets()
{
	NodeName = TEXT("Sunrise Update Targets");
	bNotifyTick = true;
	bCallTickOnSearchStart = true;
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void USunriseBTService_UpdateTargets::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(OwnerComp.GetAIOwner()))
	{
		AI->RefreshTargets();
	}
}
