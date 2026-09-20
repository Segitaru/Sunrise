#include "Units/AI/Tasks/SunriseBTTask_Attack.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Units/AI/SunriseUnitAIController.h"
#include "Units/SunriseUnit.h"
#include "Weapons/SunriseWeapon.h"

USunriseBTTask_Attack::USunriseBTTask_Attack()
{
	NodeName = TEXT("Sunrise Attack");
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USunriseBTTask_Attack, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type USunriseBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(OwnerComp.GetAIOwner());
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASunriseUnit* Target = BB ? Cast<ASunriseUnit>(BB->GetValueAsObject(GetSelectedBlackboardKey())) : nullptr;
	if (!AI || !AI->CanAttackTarget(Target))
	{
		return EBTNodeResult::Failed;
	}
	ASunriseUnit* Unit = CastChecked<ASunriseUnit>(AI->GetPawn());
	const FVector Direction = Target->GetActorLocation() - Unit->GetActorLocation();
	if (!Direction.IsNearlyZero())
	{
		Unit->SetActorRotation(Direction.Rotation());
	}
	// Cooldown rejection is a normal retry, not a reason to discard a valid target.
	Unit->GetWeapon()->TryActivatePrimaryAttack(Target);
	return EBTNodeResult::Succeeded;
}
