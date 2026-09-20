// Copyright Epic Games, Inc. All Rights Reserved.

#include "Attribute/ModularAttributeSet.h"

#include "AbilitySystem/ModularAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAttributeSet)

class UWorld;


UModularAttributeSet::UModularAttributeSet()
{
}

UWorld* UModularAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UModularAbilitySystemComponent* UModularAttributeSet::GetModularAbilitySystemComponent() const
{
	return Cast<UModularAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

