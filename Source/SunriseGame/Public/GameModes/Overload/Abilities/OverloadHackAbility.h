// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"

#include "OverloadHackAbility.generated.h"

UCLASS()
class SUNRISEGAME_API UOverloadHackAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UOverloadHackAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
