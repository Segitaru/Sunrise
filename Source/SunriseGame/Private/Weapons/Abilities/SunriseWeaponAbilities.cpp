// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/Abilities/SunriseWeaponAbilities.h"

#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/SunriseUnit.h"
#include "Weapons/SunriseWeapon.h"

const FString BaseAnimation = TEXT("/Game/Sunrise/Character/UE4Defult/Animations/DustOff/A_Dust02_Montage.A_Dust02_Montage");

USunriseWeaponAbility::USunriseWeaponAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
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
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseBowAttackAbility::USunriseBowAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseDrumsHealAbility::USunriseDrumsHealAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseStaffAttackAbility::USunriseStaffAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
USunriseSpearShieldAttackAbility::USunriseSpearShieldAttackAbility()
{
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}

USunriseStaffAreaAttackAbility::USunriseStaffAreaAttackAbility()
{
	WeaponAction = ESunriseWeaponAbilityAction::Area;
	// Example direct asset path: replace this with the montage you want for a specific weapon ability.
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DefaultWeaponAnimation(*BaseAnimation);
	if (DefaultWeaponAnimation.Succeeded())
	{
		WeaponAnimationMontage = DefaultWeaponAnimation.Object;
	}
}
