// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Effects/OverloadEffects.h"

#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"

namespace OverloadEffects
{
	FGameplayEffectModifierMagnitude Caller(FName Name)
	{
		FSetByCallerFloat Value;
		Value.DataName = Name;
		return FGameplayEffectModifierMagnitude(Value);
	}

	FGameplayModifierInfo Modifier(const FGameplayAttribute& Attribute, EGameplayModOp::Type Operation, FName Name)
	{
		FGameplayModifierInfo Result;
		Result.Attribute = Attribute;
		Result.ModifierOp = Operation;
		Result.ModifierMagnitude = Caller(Name);
		return Result;
	}
} // namespace OverloadEffects

UOverloadIntegrityEffect::UOverloadIntegrityEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(OverloadEffects::Modifier(UOverloadIntegritySet::GetIntegrityAttribute(), EGameplayModOp::Additive, MagnitudeName()));
}

UOverloadEnergyEffect::UOverloadEnergyEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(OverloadEffects::Modifier(UOverloadEnergySet::GetOverloadEnergyAttribute(), EGameplayModOp::Additive, MagnitudeName()));
}

UOverloadObjectiveScalingEffect::UOverloadObjectiveScalingEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Modifiers.Add(OverloadEffects::Modifier(UOverloadDefenseSet::GetAttackPowerAttribute(), EGameplayModOp::Multiplicitive, AttackName()));
	Modifiers.Add(OverloadEffects::Modifier(UOverloadDefenseSet::GetArmorAttribute(), EGameplayModOp::Multiplicitive, ArmorName()));
	Modifiers.Add(
		OverloadEffects::Modifier(UOverloadHackSet::GetHackResistanceAttribute(), EGameplayModOp::Multiplicitive, ResistanceName()));
}
