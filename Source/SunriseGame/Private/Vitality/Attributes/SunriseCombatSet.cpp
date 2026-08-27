#include "Vitality/Attributes/SunriseCombatSet.h"

#include "Net/UnrealNetwork.h"

void USunriseCombatSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetAttackPowerAttribute() || Attribute == GetAggroRadiusAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetActionRangeAttribute())
	{
		NewValue = FMath::Max(50.0f, NewValue);
	}
	else if (Attribute == GetActionIntervalAttribute())
	{
		NewValue = FMath::Max(0.05f, NewValue);
	}
}

void USunriseCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseCombatSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseCombatSet, ActionRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseCombatSet, ActionInterval, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseCombatSet, AggroRadius, COND_None, REPNOTIFY_Always);
}
#define SUNRISE_COMBAT_REP(Property)                                                 \
	void USunriseCombatSet::OnRep_##Property(const FGameplayAttributeData& OldValue) \
	{                                                                                \
		GAMEPLAYATTRIBUTE_REPNOTIFY(USunriseCombatSet, Property, OldValue);          \
	}
SUNRISE_COMBAT_REP(AttackPower)
SUNRISE_COMBAT_REP(ActionRange)
SUNRISE_COMBAT_REP(ActionInterval)
SUNRISE_COMBAT_REP(AggroRadius)
#undef SUNRISE_COMBAT_REP
