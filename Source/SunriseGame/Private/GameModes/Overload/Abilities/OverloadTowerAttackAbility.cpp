// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Abilities/OverloadTowerAttackAbility.h"

#include "GameModes/Overload/Actors/OverloadGuardTower.h"

UOverloadTowerAttackAbility::UOverloadTowerAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UOverloadTowerAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (AOverloadGuardTower* Tower = ActorInfo ? Cast<AOverloadGuardTower>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Tower->ExecuteTowerAttack();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
