#include "Abilities/SunriseUnitOrderExecutionAbility.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/SunriseUnitOrderAbility.h"
#include "Environment/Resources/SunriseResourceNode.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "Units/SunriseUnit.h"
#include "Units/SunriseUnitInterfaces.h"

USunriseUnitOrderExecutionAbility::USunriseUnitOrderExecutionAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	for (FGameplayTag Tag :
		{SunriseOrders::Move.GetTag(), SunriseOrders::Target.GetTag(), SunriseOrders::Hack.GetTag(), SunriseOrders::Stop.GetTag()})
	{
		FAbilityTriggerData Trigger;
		Trigger.TriggerTag = Tag;
		Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(Trigger);
	}
}

void USunriseUnitOrderExecutionAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const bool bSuccess = ActorInfo && ActorInfo->IsNetAuthority() && TriggerEventData && ExecuteOrder(*TriggerEventData, ActorInfo);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bSuccess);
}

bool USunriseUnitOrderExecutionAbility::ExecuteOrder(const FGameplayEventData& Event, const FGameplayAbilityActorInfo* ActorInfo)
{
	ASunriseUnit* Unit = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!IsValid(Unit) || !Unit->HasAuthority() || !Unit->IsAlive() ||
		(Event.EventTag != SunriseOrders::Move && Event.EventTag != SunriseOrders::Target && Event.EventTag != SunriseOrders::Hack &&
			Event.EventTag != SunriseOrders::Stop) ||
		Event.TargetData.Num() != 2 || !Event.TargetData.Get(0) || !Event.TargetData.Get(1) ||
		Event.TargetData.Get(0)->GetScriptStruct() != FGameplayAbilityTargetData_ActorArray::StaticStruct() ||
		Event.TargetData.Get(1)->GetScriptStruct() != FGameplayAbilityTargetData_SingleTargetHit::StaticStruct())
	{
		return false;
	}

	const TArray<TWeakObjectPtr<AActor>> Actors = Event.TargetData.Get(0)->GetActors();
	const FHitResult* Hit = Event.TargetData.Get(1)->GetHitResult();
	if (Actors.Num() != 1 || Actors[0].Get() != Unit || !Hit || Hit->ImpactPoint.ContainsNaN() ||
		Hit->ImpactPoint.GetAbsMax() > HALF_WORLD_MAX || !CommitAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo))
	{
		return false;
	}

	AActor* TargetActor = const_cast<AActor*>(Event.Target.Get());
	ASunriseUnit* TargetUnit = Cast<ASunriseUnit>(TargetActor);
	const ASunriseResourceNode* ResourceNode = Cast<ASunriseResourceNode>(TargetActor);
	const ASurvivalBuilding* ConstructionSite = Cast<ASurvivalBuilding>(TargetActor);
	const bool bValidUnitTarget =
		IsValid(TargetUnit) && TargetUnit != Unit && TargetUnit->GetWorld() == GetWorld() && TargetUnit->IsAlive();
	const bool bValidResourceTarget =
		IsValid(ResourceNode) && ResourceNode->GetWorld() == GetWorld() && ResourceNode->GetRemainingAmount() > 0;
	const bool bValidConstructionTarget = IsValid(ConstructionSite) && ConstructionSite->GetWorld() == GetWorld() &&
										  ConstructionSite->IsAlive() && !ConstructionSite->IsConstructionComplete();
	if (Event.EventTag == SunriseOrders::Target && !bValidUnitTarget && !bValidResourceTarget && !bValidConstructionTarget)
	{
		return false;
	}
	if (Event.EventTag == SunriseOrders::Hack &&
		(!IsValid(TargetActor) || TargetActor->GetWorld() != GetWorld() || !TargetActor->Implements<UOverloadHackable>()))
	{
		return false;
	}

	if (Event.EventTag == SunriseOrders::Move)
	{
		ISunriseOrderReceiver::Execute_IssueMoveOrder(Unit, Hit->ImpactPoint);
		return true;
	}
	if (Event.EventTag == SunriseOrders::Target)
	{
		if (ResourceNode || ConstructionSite || !Unit->HasPawnTag(SunrisePawnTags::Class_Healer) || Unit->CanTargetWithWeapon(TargetUnit))
		{
			ISunriseOrderReceiver::Execute_IssueTargetOrder(Unit, TargetActor);
		}
		else
		{
			ISunriseOrderReceiver::Execute_IssueMoveOrder(Unit, TargetUnit->GetActorLocation());
		}
		return true;
	}
	if (Event.EventTag == SunriseOrders::Hack)
	{
		UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>();
		return Interactor && Interactor->RequestHack(TargetActor, true);
	}
	if (UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>())
	{
		Interactor->CancelHack();
	}
	ISunriseOrderReceiver::Execute_StopOrder(Unit);
	return true;
}
