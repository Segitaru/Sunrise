// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/Effects/SunriseWeaponEffects.h"

#include "Vitality/Attributes/SunriseHealthSet.h"

namespace SunriseWeaponEffects
{
	FGameplayEffectModifierMagnitude SetByCaller(FName Name)
	{
		FSetByCallerFloat Value;
		Value.DataName = Name;
		return FGameplayEffectModifierMagnitude(Value);
	}
} // namespace SunriseWeaponEffects

USunriseDamageEffect::USunriseDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USunriseHealthSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = SunriseWeaponEffects::SetByCaller(GetMagnitudeDataName());
	Modifiers.Add(Modifier);
}

USunriseHealingEffect::USunriseHealingEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = USunriseHealthSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = SunriseWeaponEffects::SetByCaller(GetMagnitudeDataName());
	Modifiers.Add(Modifier);
}
