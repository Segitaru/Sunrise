// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Abilities/OverloadHackAbility.h"

#include "GameModes/Overload/Components/OverloadInteractorComponent.h"

UOverloadHackAbility::UOverloadHackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UOverloadHackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (UOverloadInteractorComponent* Interactor = Avatar ? Avatar->FindComponentByClass<UOverloadInteractorComponent>() : nullptr)
	{
		Interactor->CommitHack();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
