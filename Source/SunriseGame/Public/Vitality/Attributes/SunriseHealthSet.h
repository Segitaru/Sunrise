// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once



#include "AbilitySystem/Attributes/SunriseAttributeSet.h"

#include "SunriseHealthSet.generated.h"

struct FGameplayEffectModCallbackData;

/** Health-only state shared by units, heroes and destructible environment actors. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseHealthSet : public USunriseAttributeSet
{
	GENERATED_BODY()

public:
	USunriseHealthSet();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseHealthSet, Health)
	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseHealthSet, MaxHealth)

	mutable FSunriseAttributeEvent OnHealthChanged;
	mutable FSunriseAttributeEvent OnMaxHealthChanged;
	mutable FSunriseAttributeEvent OnOutOfHealth;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Sunrise|Vitality", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Sunrise|Vitality", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	bool bOutOfHealth = false;
	float HealthBeforeEffect = 0.0f;
	float MaxHealthBeforeEffect = 0.0f;
};
