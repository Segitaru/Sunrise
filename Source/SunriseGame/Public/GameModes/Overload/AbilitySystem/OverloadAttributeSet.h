#pragma once

#include "AbilitySystem/Attributes/SunriseAttributeSet.h"

#include "OverloadAttributeSet.generated.h"

UCLASS(meta = (DeprecatedNode, DeprecationMessage = "Use focused Overload attribute sets"))
class SUNRISEGAME_API UOverloadAttributeSet : public USunriseAttributeSet
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class SUNRISEGAME_API UOverloadIntegritySet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadIntegritySet, Integrity)
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadIntegritySet, MaxIntegrity)
private:
	UFUNCTION()
	void OnRep_Integrity(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxIntegrity(const FGameplayAttributeData& OldValue);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Integrity, Category = "Overload|Integrity", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Integrity;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxIntegrity, Category = "Overload|Integrity", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxIntegrity;
};

UCLASS(BlueprintType)
class SUNRISEGAME_API UOverloadDefenseSet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadDefenseSet, AttackPower)
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadDefenseSet, Armor)
private:
	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "Overload|Defense", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData AttackPower;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Overload|Defense", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Armor;
};

UCLASS(BlueprintType)
class SUNRISEGAME_API UOverloadHackSet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadHackSet, HackResistance)
private:
	UFUNCTION()
	void OnRep_HackResistance(const FGameplayAttributeData& OldValue);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HackResistance, Category = "Overload|Hack", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData HackResistance;
};

UCLASS(BlueprintType)
class SUNRISEGAME_API UOverloadEnergySet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadEnergySet, OverloadEnergy)
	SUNRISE_ATTRIBUTE_ACCESSORS(UOverloadEnergySet, MaxOverloadEnergy)
private:
	UFUNCTION()
	void OnRep_OverloadEnergy(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxOverloadEnergy(const FGameplayAttributeData& OldValue);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OverloadEnergy, Category = "Overload|Energy", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData OverloadEnergy;
	UPROPERTY(
		BlueprintReadOnly, ReplicatedUsing = OnRep_MaxOverloadEnergy, Category = "Overload|Energy", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxOverloadEnergy;
};
