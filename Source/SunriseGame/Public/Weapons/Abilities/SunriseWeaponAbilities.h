// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"
#include "Weapons/Types/SunriseWeaponTypes.h"

#include "SunriseWeaponAbilities.generated.h"

UCLASS(Abstract)
class SUNRISEGAME_API USunriseWeaponAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseWeaponAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void PlayWeaponAnimation(const FGameplayAbilityActorInfo* ActorInfo) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon|Animation")
	TObjectPtr<UAnimMontage> WeaponAnimationMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon|Animation", meta = (ClampMin = "0.1"))
	float WeaponAnimationPlayRate = 1.0f;

	ESunriseWeaponAbilityAction WeaponAction = ESunriseWeaponAbilityAction::Primary;
};

UCLASS()
class SUNRISEGAME_API USunriseSwordAttackAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
	USunriseSwordAttackAbility();
};

UCLASS()
class SUNRISEGAME_API USunriseBowAttackAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
	USunriseBowAttackAbility();
};

UCLASS()
class SUNRISEGAME_API USunriseDrumsHealAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
	USunriseDrumsHealAbility();
};

UCLASS()
class SUNRISEGAME_API USunriseStaffAttackAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
	USunriseStaffAttackAbility();
};

UCLASS()
class SUNRISEGAME_API USunriseSpearShieldAttackAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
	USunriseSpearShieldAttackAbility();
};

UCLASS()
class SUNRISEGAME_API USunriseStaffAreaAttackAbility : public USunriseWeaponAbility
{
	GENERATED_BODY()
public:
	USunriseStaffAreaAttackAbility();
};
