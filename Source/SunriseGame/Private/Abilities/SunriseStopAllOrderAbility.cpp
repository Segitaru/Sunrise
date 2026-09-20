#include "Abilities/SunriseStopAllOrderAbility.h"

#include "Abilities/SunriseUnitOrderAbility.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"

USunriseStopAllOrderAbility::USunriseStopAllOrderAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USunriseStopAllOrderAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunrisePawn* Pawn = ActorInfo ? Cast<ASunrisePawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	ASunrisePlayerController* Controller = Pawn ? Pawn->GetController<ASunrisePlayerController>() : nullptr;
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller);
	TArray<ASunriseUnit*> Units;
	if (Manager)
	{
		for (AActor* Entity : Manager->GetControlledEntities())
		{
			if (ASunriseUnit* Unit = Cast<ASunriseUnit>(Entity); IsValid(Unit) && Unit->IsAlive() && Manager->CanControlEntity(Unit))
			{
				Units.Add(Unit);
			}
		}
	}
	if (Pawn && !Units.IsEmpty())
	{
		USunriseUnitOrderAbility::SendOrderEvent(Pawn, SunriseOrders::Stop, Units);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
