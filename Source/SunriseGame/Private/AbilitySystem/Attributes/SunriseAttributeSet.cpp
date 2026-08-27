// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/Attributes/SunriseAttributeSet.h"

#include "AbilitySystemComponent.h"

UAbilitySystemComponent* USunriseAttributeSet::GetSunriseAbilitySystemComponent() const
{
	return GetOwningAbilitySystemComponent();
}
