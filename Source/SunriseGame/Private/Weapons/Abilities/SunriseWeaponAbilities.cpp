// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/Abilities/SunriseWeaponAbilities.h"

#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/SunriseUnit.h"
#include "Weapons/SunriseWeapon.h"

USunriseWeaponAbility::USunriseWeaponAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Unarmed/Attack/MM_Attack_01_Montage.MM_Attack_01_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}

void USunriseWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PlayWeaponAnimation(ActorInfo);
	ASunriseUnit* Unit = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (USunriseWeapon* Weapon = Unit ? Unit->GetWeapon() : nullptr; IsValid(Weapon))
	{
		if (WeaponAction == ESunriseWeaponAbilityAction::Area)
		{
			Weapon->ExecuteAreaAbility();
		}
		else
		{
			Weapon->ExecutePrimaryAbility();
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USunriseWeaponAbility::PlayWeaponAnimation(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!WeaponAnimationMontage || !ActorInfo)
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->PlayAnimMontage(WeaponAnimationMontage, WeaponAnimationPlayRate);
	}
}

USunriseSwordAttackAbility::USunriseSwordAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Unarmed/Attack/MM_Attack_01_Montage.MM_Attack_01_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseBowAttackAbility::USunriseBowAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Rifle/MM_Rifle_DryFire_Montage.MM_Rifle_DryFire_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseDrumsHealAbility::USunriseDrumsHealAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Pistol/MM_Pistol_Reload_Montage.MM_Pistol_Reload_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseStaffAttackAbility::USunriseStaffAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Unarmed/Jump/MM_Jump_Montage.MM_Jump_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseSpearShieldAttackAbility::USunriseSpearShieldAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack_Montage.MM_ChargedAttack_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}

USunriseStaffAreaAttackAbility::USunriseStaffAreaAttackAbility()
{
	WeaponAction = ESunriseWeaponAbilityAction::Area;
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(
		TEXT("/Game/Sunrise/Character/Mannequins/Anims/Death/MM_Death_Right_01_Montage.MM_Death_Right_01_Montage"));
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
