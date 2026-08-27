// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Attributes/SunriseAttributeSet.h"

#include "SunriseCombatSet.generated.h"

/** Targeting and action tuning for combat-capable actors. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseCombatSet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseCombatSet, AttackPower)
	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseCombatSet, ActionRange)
	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseCombatSet, ActionInterval)
	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseCombatSet, AggroRadius)

private:
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ActionRange(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_ActionInterval(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_AggroRadius(const FGameplayAttributeData& OldValue);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Sunrise|Combat", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackPower;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActionRange, Category = "Sunrise|Combat", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ActionRange;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ActionInterval, Category = "Sunrise|Combat", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ActionInterval;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AggroRadius, Category = "Sunrise|Combat", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AggroRadius;
};
