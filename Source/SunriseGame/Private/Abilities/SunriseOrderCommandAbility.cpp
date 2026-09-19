#include "Abilities/SunriseOrderCommandAbility.h"

#include "Abilities/SunriseUnitOrderAbility.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"

USunriseOrderCommandAbility::USunriseOrderCommandAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USunriseOrderCommandAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunrisePawn* Pawn = ActorInfo ? Cast<ASunrisePawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	ASunrisePlayerController* PC = Pawn ? Cast<ASunrisePlayerController>(Pawn->GetController()) : nullptr;
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(PC);
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
	FHitResult Hit;
	AActor* Target = nullptr;
	FGameplayTag OrderTag = SunriseOrders::Move;
	if (PC && PC->GetHitResultUnderCursorByChannel(TraceTypeQuery1, true, Hit))
	{
		Target = Hit.GetActor();
		if (Cast<ASunriseUnit>(Target))
		{
			OrderTag = SunriseOrders::Target;
		}
		else if (IsValid(Target) && Target->Implements<UOverloadHackable>())
		{
			OrderTag = SunriseOrders::Hack;
		}
	}
	if (Pawn && !Units.IsEmpty())
	{
		USunriseUnitOrderAbility::SendOrderEvent(Pawn, OrderTag, Units, nullptr, Hit.ImpactPoint, Target);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
