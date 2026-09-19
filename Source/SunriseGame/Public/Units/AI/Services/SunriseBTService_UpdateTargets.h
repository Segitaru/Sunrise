#pragma once

#include "BehaviorTree/BTService.h"

#include "SunriseBTService_UpdateTargets.generated.h"

/** Attach to the root selector so validation also runs during player orders and MoveTo. */
UCLASS()
class SUNRISEGAME_API USunriseBTService_UpdateTargets : public UBTService
{
	GENERATED_BODY()

public:
	USunriseBTService_UpdateTargets();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
