// Copyright Epic Games, Inc. All Rights Reserved.

#include "Vitality/Attributes/SunriseHealthSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

USunriseHealthSet::USunriseHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
{
}

void USunriseHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void USunriseHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void USunriseHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

bool USunriseHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}
	HealthBeforeEffect = GetHealth();
	MaxHealthBeforeEffect = GetMaxHealth();
	return true;
}

void USunriseHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = Context.GetOriginalInstigator();
	AActor* Causer = Context.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float OldValue = HealthBeforeEffect;
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldValue, GetHealth());
		if (!bOutOfHealth && GetHealth() <= 0.0f)
		{
			OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, OldValue, GetHealth());
		}
		bOutOfHealth = GetHealth() <= 0.0f;
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		if (GetHealth() > GetMaxHealth())
		{
			SetHealth(GetMaxHealth());
		}
		OnMaxHealthChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeEffect, GetMaxHealth());
	}
}

void USunriseHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void USunriseHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USunriseHealthSet, Health, OldValue);
	const float Current = GetHealth();
	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, Current - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), Current);
	if (!bOutOfHealth && Current <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, Current - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), Current);
	}
	bOutOfHealth = Current <= 0.0f;
}

void USunriseHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USunriseHealthSet, MaxHealth, OldValue);
	OnMaxHealthChanged.Broadcast(
		nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}
