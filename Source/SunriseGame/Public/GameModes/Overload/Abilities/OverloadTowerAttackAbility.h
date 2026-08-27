// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"

#include "OverloadTowerAttackAbility.generated.h"

UCLASS()
class SUNRISEGAME_API UOverloadTowerAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UOverloadTowerAttackAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
