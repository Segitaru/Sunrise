#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"
#include "NativeGameplayTags.h"

#include "SunriseAbilityCommandAbilities.generated.h"

/** Pawn-side bridge that forwards an ability input event to controlled units. */
UCLASS(Abstract, Blueprintable)
class SUNRISEGAME_API USunriseAbilityCommandAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	USunriseAbilityCommandAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual FGameplayTag GetCommandInputTag() const { return FGameplayTag(); }
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunrisePrimaryAbilityCommand : public USunriseAbilityCommandAbility
{
	GENERATED_BODY()
protected:
	virtual FGameplayTag GetCommandInputTag() const override;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseSecondaryAbilityCommand : public USunriseAbilityCommandAbility
{
	GENERATED_BODY()
protected:
	virtual FGameplayTag GetCommandInputTag() const override;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseOptionalAbilityCommand : public USunriseAbilityCommandAbility
{
	GENERATED_BODY()
protected:
	virtual FGameplayTag GetCommandInputTag() const override;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseUltimateAbilityCommand : public USunriseAbilityCommandAbility
{
	GENERATED_BODY()
protected:
	virtual FGameplayTag GetCommandInputTag() const override;
};
