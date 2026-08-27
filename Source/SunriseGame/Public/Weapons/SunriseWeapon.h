// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "UObject/Object.h"
#include "Weapons/Types/SunriseWeaponTypes.h"

#include "SunriseWeapon.generated.h"

class ASunriseUnit;
class UGameplayAbility;

/** Base equipment object: attributes, GAS grants, cooldown and attack execution live here. */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SUNRISEGAME_API USunriseWeapon : public UObject
{
	GENERATED_BODY()
public:
	virtual UWorld* GetWorld() const override;
	virtual void Initialize(ASunriseUnit* InOwner);
	virtual void Uninitialize();
	virtual bool CanTarget(const ASunriseUnit* Target) const;
	bool TryActivatePrimaryAttack(ASunriseUnit* Target);
	virtual void ExecutePrimaryAbility();
	virtual void ExecuteAreaAbility();

	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	ASunriseUnit* GetOwningUnit() const { return OwnerUnit; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	ASunriseUnit* GetPendingTarget() const { return PendingTarget.Get(); }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	ESunriseWeaponType GetWeaponType() const { return WeaponType; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	float GetAttackRange() const { return ActionRange; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	float GetAttackInterval() const { return ActionInterval; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	float GetPower() const { return Power; }

protected:
	void GrantAbilities();
	void ApplyDamage(ASunriseUnit* Target, float PowerScale = 1.0f, bool bArea = false);
	void ApplyHealing(ASunriseUnit* Target, float PowerScale = 1.0f, bool bArea = false);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Weapon", meta = (DisplayName = "Weapon Action"))
	void BP_WeaponAction(ASunriseUnit* Target, bool bHealing, bool bArea);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon")
	ESunriseWeaponType WeaponType = ESunriseWeaponType::Sword;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon")
	FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.0"))
	float Power = 20.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "50.0", Units = "cm"))
	float ActionRange = 200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float ActionInterval = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Abilities")
	TSubclassOf<UGameplayAbility> PrimaryAbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilityClasses;
	UPROPERTY(Transient)
	TObjectPtr<ASunriseUnit> OwnerUnit;
	TWeakObjectPtr<ASunriseUnit> PendingTarget;
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
	float NextActionTime = 0.0f;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseSwordWeapon : public USunriseWeapon
{
	GENERATED_BODY()
public:
	USunriseSwordWeapon();
};
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseBowWeapon : public USunriseWeapon
{
	GENERATED_BODY()
public:
	USunriseBowWeapon();
	virtual void ExecutePrimaryAbility() override;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseDrumsWeapon : public USunriseWeapon
{
	GENERATED_BODY()
public:
	USunriseDrumsWeapon();
	virtual bool CanTarget(const ASunriseUnit* Target) const override;
	virtual void ExecutePrimaryAbility() override;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseStaffWeapon : public USunriseWeapon
{
	GENERATED_BODY()
public:
	USunriseStaffWeapon();
	virtual void ExecutePrimaryAbility() override;
	virtual void ExecuteAreaAbility() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "1"))
	int32 AttacksPerAreaBurst = 3;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "50.0", Units = "cm"))
	float AreaRadius = 350.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.0"))
	float AreaPowerScale = 0.75f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Abilities")
	TSubclassOf<UGameplayAbility> AreaAbilityClass;
	int32 AttackCounter = 0;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseSpearShieldWeapon : public USunriseWeapon
{
	GENERATED_BODY()
public:
	USunriseSpearShieldWeapon();
	virtual void ExecutePrimaryAbility() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "50.0", Units = "cm"))
	float FocusRadius = 500.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.1", Units = "s"))
	float FocusDuration = 2.5f;
};
