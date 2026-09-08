// Copyright Epic Games, Inc. All Rights Reserved.
#include "Abilities/SunriseUnitOrderAbility.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/SunriseHeroSquadAbility.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunriseUnit.h"
#include "Units/SunriseUnitInterfaces.h"

namespace SunriseOrders
{
	UE_DEFINE_GAMEPLAY_TAG(Move, "Event.Sunrise.Order.Move");
	UE_DEFINE_GAMEPLAY_TAG(Target, "Event.Sunrise.Order.Target");
	UE_DEFINE_GAMEPLAY_TAG(Hack, "Event.Sunrise.Order.Hack");
	UE_DEFINE_GAMEPLAY_TAG(Stop, "Event.Sunrise.Order.Stop");
	UE_DEFINE_GAMEPLAY_TAG(Squad, "Event.Sunrise.Order.Squad");
} // namespace SunriseOrders

USunriseUnitOrderAbility::USunriseUnitOrderAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	for (FGameplayTag Tag : {SunriseOrders::Move.GetTag(), SunriseOrders::Target.GetTag(), SunriseOrders::Hack.GetTag(),
			 SunriseOrders::Stop.GetTag(), SunriseOrders::Squad.GetTag()})
	{
		FAbilityTriggerData Trigger;
		Trigger.TriggerTag = Tag;
		Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(Trigger);
	}
}

void USunriseUnitOrderAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// GAS carries the event/target data in ServerTryActivateAbilityWithEventData.
	// Local prediction submits intent only; no client writes to units.
	const bool bAuthority = ActorInfo && ActorInfo->IsNetAuthority();
	const bool bSuccess = !bAuthority || (TriggerEventData && ExecuteOrder(*TriggerEventData, ActorInfo));
	EndAbility(Handle, ActorInfo, ActivationInfo, bAuthority, !bSuccess);
}

bool USunriseUnitOrderAbility::ExecuteOrder(const FGameplayEventData& Event, const FGameplayAbilityActorInfo* ActorInfo)
{
	ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(ActorInfo->PlayerController.Get());
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(PC);
	if (!PC || !PC->HasAuthority() || !PC->AreCommandsEnabled() || PC->IsPaused() || !Manager ||
		ActorInfo->AvatarActor.Get() != PC->GetPawn())
	{
		return false;
	}
	const FGameplayTag Tag = Event.EventTag;
	if (Tag != SunriseOrders::Move && Tag != SunriseOrders::Target && Tag != SunriseOrders::Hack && Tag != SunriseOrders::Stop &&
		Tag != SunriseOrders::Squad)
	{
		return false;
	}
	if (Tag == SunriseOrders::Squad)
	{
		for (AActor* Entity : Manager->GetControlledEntities())
		{
			ASunriseUnit* Hero = Cast<ASunriseUnit>(Entity);
			if (IsValid(Hero) && Hero->IsHero() && Hero->IsAlive() && Manager->CanControlEntity(Hero))
			{
				return USunriseHeroSquadAbility::ActivateForHero(Hero, HeroSquadAbilityClass);
			}
		}
		return false;
	}
	if (Event.TargetData.Num() != 2 || !Event.TargetData.Get(0) || !Event.TargetData.Get(1) ||
		Event.TargetData.Get(0)->GetScriptStruct() != FGameplayAbilityTargetData_ActorArray::StaticStruct() ||
		Event.TargetData.Get(1)->GetScriptStruct() != FGameplayAbilityTargetData_SingleTargetHit::StaticStruct())
	{
		return false;
	}
	const TArray<TWeakObjectPtr<AActor>> Actors = Event.TargetData.Get(0)->GetActors();
	if (Actors.IsEmpty() || Actors.Num() > 128)
	{
		return false;
	}
	TArray<ASunriseUnit*> Units;
	for (const TWeakObjectPtr<AActor>& Actor : Actors)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Actor.Get());
		if (!IsValid(Unit) || Unit->GetWorld() != GetWorld() || !Unit->IsAlive() || !Manager->CanControlEntity(Unit) ||
			Units.Contains(Unit))
		{
			return false;
		}
		Units.Add(Unit);
	}
	const FVector Location = Event.TargetData.Get(1)->GetHitResult()->ImpactPoint;
	if (Location.ContainsNaN() || Location.GetAbsMax() > HALF_WORLD_MAX || !FMath::IsFinite(FormationSpacing) || FormationSpacing < 50.0f)
	{
		return false;
	}
	AActor* TargetActor = const_cast<AActor*>(Event.Target.Get());
	ASunriseUnit* TargetUnit = Cast<ASunriseUnit>(TargetActor);
	if (Tag == SunriseOrders::Target && (!IsValid(TargetUnit) || TargetUnit->GetWorld() != GetWorld() || !TargetUnit->IsAlive()))
	{
		return false;
	}
	if (Tag == SunriseOrders::Hack &&
		(!IsValid(TargetActor) || TargetActor->GetWorld() != GetWorld() || !TargetActor->Implements<UOverloadHackable>()))
	{
		return false;
	}
	if (!CommitAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo))
	{
		return false;
	}
	const int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Units.Num())));
	const int32 Rows = FMath::DivideAndRoundUp(Units.Num(), Columns);
	bool bIssuedOrder = false;
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		ASunriseUnit* Unit = Units[Index];
		if (Tag == SunriseOrders::Move)
		{
			const FVector Offset((Index / Columns - (Rows - 1) * 0.5f) * FormationSpacing,
				(Index % Columns - (Columns - 1) * 0.5f) * FormationSpacing, 0.0f);
			ISunriseOrderReceiver::Execute_IssueMoveOrder(Unit, Location + Offset);
		}
		else if (Tag == SunriseOrders::Target)
		{
			if (TargetUnit == Unit)
			{
				continue;
			}
			if (Unit->GetUnitRole() != ESunriseUnitRole::Healer || Unit->CanTargetWithWeapon(TargetUnit))
			{
				ISunriseOrderReceiver::Execute_IssueTargetOrder(Unit, TargetUnit);
			}
			else
			{
				ISunriseOrderReceiver::Execute_IssueMoveOrder(
					Unit, TargetUnit->GetActorLocation() + FVector(0.0f, (Index - Units.Num() * 0.5f) * FormationSpacing, 0.0f));
			}
		}
		else if (Tag == SunriseOrders::Hack)
		{
			UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>();
			bIssuedOrder |= Interactor && Interactor->RequestHack(TargetActor, true);
			continue;
		}
		else
		{
			if (UOverloadInteractorComponent* Interactor = Unit->FindComponentByClass<UOverloadInteractorComponent>())
			{
				Interactor->CancelHack();
			}
			ISunriseOrderReceiver::Execute_StopOrder(Unit);
		}
		bIssuedOrder = true;
	}
	return bIssuedOrder;
}

float USunriseUnitOrderAbility::GetHeroSquadCooldownRemaining(const AController* Controller)
{
	if (const UControllableEntitiesManager* const Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
	{
		for (AActor* Entity : Manager->GetControlledEntities())
		{
			const ASunriseUnit* Hero = Cast<ASunriseUnit>(Entity);
			if (IsValid(Hero) && Hero->IsHero() && Hero->IsAlive() && Manager->CanControlEntity(Hero))
			{
				return USunriseHeroSquadAbility::GetCooldownRemaining(Hero);
			}
		}
	}
	return 0.0f;
}
