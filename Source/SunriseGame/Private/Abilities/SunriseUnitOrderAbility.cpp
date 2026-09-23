// Copyright Epic Games, Inc. All Rights Reserved.
#include "Abilities/SunriseUnitOrderAbility.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/SunriseHeroSquadAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Environment/Resources/SunriseResourceNode.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunriseUnit.h"

namespace SunriseOrders
{
	UE_DEFINE_GAMEPLAY_TAG(Move, "Event.Sunrise.Order.Move");
	UE_DEFINE_GAMEPLAY_TAG(Target, "Event.Sunrise.Order.Target");
	UE_DEFINE_GAMEPLAY_TAG(Hack, "Event.Sunrise.Order.Hack");
	UE_DEFINE_GAMEPLAY_TAG(Stop, "Event.Sunrise.Order.Stop");
} // namespace SunriseOrders

USunriseUnitOrderAbility::USunriseUnitOrderAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	for (FGameplayTag Tag :
		{SunriseOrders::Move.GetTag(), SunriseOrders::Target.GetTag(), SunriseOrders::Hack.GetTag(), SunriseOrders::Stop.GetTag()})
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
	const bool bSuccess = !bAuthority || (TriggerEventData && DispatchOrder(*TriggerEventData, ActorInfo));
	EndAbility(Handle, ActorInfo, ActivationInfo, bAuthority, !bSuccess);
}

bool USunriseUnitOrderAbility::SendOrderEvent(
	AActor* Pawn, FGameplayTag Tag, const TArray<ASunriseUnit*>& Units, ASunriseUnit* SingleUnit, const FVector& Location, AActor* Target)
{
	if (!IsValid(Pawn) || !Tag.IsValid() || Units.IsEmpty())
	{
		return false;
	}
	FGameplayEventData Event;
	Event.EventTag = Tag;
	Event.Target = Target;
	FGameplayAbilityTargetData_ActorArray* Actors = new FGameplayAbilityTargetData_ActorArray();
	if (SingleUnit)
	{
		Actors->TargetActorArray.Add(SingleUnit);
	}
	else
	{
		for (ASunriseUnit* Unit : Units)
		{
			if (IsValid(Unit))
			{
				Actors->TargetActorArray.Add(Unit);
			}
		}
	}
	if (Actors->TargetActorArray.IsEmpty())
	{
		return false;
	}
	Event.TargetData.Add(Actors);
	FHitResult Hit;
	Hit.ImpactPoint = Location;
	Hit.Location = Location;
	Event.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Tag, Event);
	return true;
}

bool USunriseUnitOrderAbility::DispatchOrder(const FGameplayEventData& Event, const FGameplayAbilityActorInfo* ActorInfo)
{
	ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(ActorInfo->PlayerController.Get());
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(PC);
	if (!PC || !PC->HasAuthority() || !PC->AreCommandsEnabled() || PC->IsPaused() || !Manager ||
		ActorInfo->AvatarActor.Get() != PC->GetPawn())
	{
		return false;
	}

	const FGameplayTag Tag = Event.EventTag;
	if (Tag != SunriseOrders::Move && Tag != SunriseOrders::Target && Tag != SunriseOrders::Hack && Tag != SunriseOrders::Stop)
	{
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
	const ASunriseResourceNode* ResourceNode = Cast<ASunriseResourceNode>(TargetActor);
	const ASurvivalBuilding* ConstructionSite = Cast<ASurvivalBuilding>(TargetActor);
	const bool bValidUnitTarget = IsValid(TargetUnit) && TargetUnit->GetWorld() == GetWorld() && TargetUnit->IsAlive();
	const bool bValidResourceTarget =
		IsValid(ResourceNode) && ResourceNode->GetWorld() == GetWorld() && ResourceNode->GetRemainingAmount() > 0;
	const bool bValidConstructionTarget = IsValid(ConstructionSite) && ConstructionSite->GetWorld() == GetWorld() &&
										  ConstructionSite->IsAlive() && !ConstructionSite->IsConstructionComplete();
	if (Tag == SunriseOrders::Target && !bValidUnitTarget && !bValidResourceTarget && !bValidConstructionTarget)
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

	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		ASunriseUnit* Unit = Units[Index];
		const FVector Offset = Tag == SunriseOrders::Move ? FVector((Index / Columns - (Rows - 1) * 0.5f) * FormationSpacing,
																(Index % Columns - (Columns - 1) * 0.5f) * FormationSpacing, 0.0f)
														  : FVector::ZeroVector;
		FGameplayEventData UnitEvent = Event;
		UnitEvent.TargetData = FGameplayAbilityTargetDataHandle();
		FGameplayAbilityTargetData_ActorArray* ActorData = new FGameplayAbilityTargetData_ActorArray();
		ActorData->TargetActorArray.Add(Unit);
		UnitEvent.TargetData.Add(ActorData);
		FHitResult UnitHit = *Event.TargetData.Get(1)->GetHitResult();
		UnitHit.ImpactPoint += Offset;
		UnitHit.Location = UnitHit.ImpactPoint;
		UnitEvent.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(UnitHit));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, Tag, UnitEvent);
	}
	return true;
}
