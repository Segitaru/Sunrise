#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"

#include "SunriseUnitOrderExecutionAbility.generated.h"

/** Executes one already validated order on the Unit that owns this ASC. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseUnitOrderExecutionAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	USunriseUnitOrderExecutionAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	bool ExecuteOrder(const FGameplayEventData& Event, const FGameplayAbilityActorInfo* ActorInfo);
};
