#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"
#include "GameplayTagContainer.h"

#include "SunriseOrderCommandAbility.generated.h"

/** Pawn-side bridge that turns the HeroComponent order input into an order event. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseOrderCommandAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	USunriseOrderCommandAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void PlayOrderFeedback(const FVector& Location, FGameplayTag OrderTag);
};
