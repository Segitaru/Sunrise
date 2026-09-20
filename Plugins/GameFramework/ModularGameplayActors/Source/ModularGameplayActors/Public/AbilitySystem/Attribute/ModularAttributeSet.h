// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include <AbilitySystemComponent.h>
#include "ModularAttributeSet.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class AActor;
class UModularAbilitySystemComponent;
class UObject;
class UWorld;
struct FGameplayEffectSpec;


/**
 * This macro defines a set of helper functions for accessing and initializing attributes.
 *
 * The following example of the macro:
 *		ATTRIBUTE_ACCESSORS(UModularHealthSet, Health)
 * will create the following functions:
 *		static FGameplayAttribute GetHealthAttribute();
 *		float GetHealth() const;
 *		void SetHealth(float NewVal);
 *		void InitHealth(float NewVal);
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** 
 * Delegate used to broadcast attribute events, some of these parameters may be null on clients: 
 * @param EffectInstigator	The original instigating actor for this event
 * @param EffectCauser		The physical actor that caused the change
 * @param EffectSpec		The full effect spec for this change
 * @param EffectMagnitude	The raw magnitude, this is before clamping
 * @param OldValue			The value of the attribute before it was changed
 * @param NewValue			The value after it was changed
*/
DECLARE_MULTICAST_DELEGATE_SixParams(FModularAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

/**
 * UModularAttributeSet
 *
 *	Base attribute set class for the project.
 */
UCLASS(MinimalAPI)
class UModularAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UE_API UModularAttributeSet();

	UE_API UWorld* GetWorld() const override;

	UE_API UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const;
};

#undef UE_API
