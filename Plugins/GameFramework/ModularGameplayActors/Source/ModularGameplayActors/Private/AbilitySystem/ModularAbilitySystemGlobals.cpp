// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ModularAbilitySystemGlobals.h"

#include "ModularGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilitySystemGlobals)

struct FGameplayEffectContext;

UModularAbilitySystemGlobals::UModularAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UModularAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FModularGameplayEffectContext();
}

