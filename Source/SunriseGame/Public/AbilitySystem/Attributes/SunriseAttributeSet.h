// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

#include "SunriseAttributeSet.generated.h"

class AActor;
struct FGameplayEffectSpec;

#define SUNRISE_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)   \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE_SixParams(FSunriseAttributeEvent, AActor*, AActor*, const FGameplayEffectSpec*, float, float, float);

/** Common base for every Sunrise AttributeSet. Concrete sets stay responsibility-focused. */
UCLASS(Abstract, BlueprintType)
class SUNRISEGAME_API USunriseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UAbilitySystemComponent* GetSunriseAbilitySystemComponent() const;
};
