#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"

#include "SunriseStopAllOrderAbility.generated.h"

/** Pawn-side command ability activated by the HeroComponent input tag. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseStopAllOrderAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	USunriseStopAllOrderAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
