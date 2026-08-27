#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UOverloadIntegritySet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxIntegrityAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
}
void UOverloadIntegritySet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIntegrityAttribute())
	{
		SetIntegrity(FMath::Clamp(GetIntegrity(), 0.0f, GetMaxIntegrity()));
	}
}
void UOverloadIntegritySet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadIntegritySet, Integrity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadIntegritySet, MaxIntegrity, COND_None, REPNOTIFY_Always);
}
void UOverloadIntegritySet::OnRep_Integrity(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadIntegritySet, Integrity, Old);
}
void UOverloadIntegritySet::OnRep_MaxIntegrity(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadIntegritySet, MaxIntegrity, Old);
}

void UOverloadDefenseSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetAttackPowerAttribute() || Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}
void UOverloadDefenseSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadDefenseSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadDefenseSet, Armor, COND_None, REPNOTIFY_Always);
}
void UOverloadDefenseSet::OnRep_AttackPower(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadDefenseSet, AttackPower, Old);
}
void UOverloadDefenseSet::OnRep_Armor(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadDefenseSet, Armor, Old);
}

void UOverloadHackSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetHackResistanceAttribute())
	{
		NewValue = FMath::Max(0.1f, NewValue);
	}
}
void UOverloadHackSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadHackSet, HackResistance, COND_None, REPNOTIFY_Always);
}
void UOverloadHackSet::OnRep_HackResistance(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadHackSet, HackResistance, Old);
}

void UOverloadEnergySet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxOverloadEnergyAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
}
void UOverloadEnergySet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetOverloadEnergyAttribute())
	{
		SetOverloadEnergy(FMath::Clamp(GetOverloadEnergy(), 0.0f, GetMaxOverloadEnergy()));
	}
}
void UOverloadEnergySet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadEnergySet, OverloadEnergy, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOverloadEnergySet, MaxOverloadEnergy, COND_None, REPNOTIFY_Always);
}
void UOverloadEnergySet::OnRep_OverloadEnergy(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadEnergySet, OverloadEnergy, Old);
}
void UOverloadEnergySet::OnRep_MaxOverloadEnergy(const FGameplayAttributeData& Old)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOverloadEnergySet, MaxOverloadEnergy, Old);
}
