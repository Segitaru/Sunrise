// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/SunriseWeapon.h"

#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Units/SunriseUnit.h"
#include "Weapons/Abilities/SunriseWeaponAbilities.h"
#include "Weapons/Actors/SunriseAreaIndicator.h"
#include "Weapons/Actors/SunriseProjectile.h"

UWorld* USunriseWeapon::GetWorld() const
{
	return OwnerUnit ? OwnerUnit->GetWorld() : nullptr;
}

void USunriseWeapon::Initialize(ASunriseUnit* InOwner)
{
	OwnerUnit = InOwner;
	NextActionTime = 0.0f;
	GrantAbilities();
	if (OwnerUnit)
	{
		OwnerUnit->ApplyWeaponAttributes(Power, ActionRange, ActionInterval);
	}
}

void USunriseWeapon::Uninitialize()
{
	if (OwnerUnit && OwnerUnit->GetAbilitySystemComponent())
	{
		for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
		{
			OwnerUnit->GetAbilitySystemComponent()->ClearAbility(Handle);
		}
	}
	GrantedAbilityHandles.Reset();
	PendingTarget.Reset();
	OwnerUnit = nullptr;
}

bool USunriseWeapon::CanTarget(const ASunriseUnit* Target) const
{
	return OwnerUnit && IsValid(Target) && Target->IsAlive() && Target != OwnerUnit && Target->GetTeamId() >= 0 &&
		   Target->GetTeamId() != OwnerUnit->GetTeamId();
}

bool USunriseWeapon::TryActivatePrimaryAttack(ASunriseUnit* Target)
{
	if (!CanTarget(Target) || !PrimaryAbilityClass || !GetWorld())
	{
		return false;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now + KINDA_SMALL_NUMBER < NextActionTime)
	{
		return false;
	}
	PendingTarget = Target;
	UAbilitySystemComponent* ASC = OwnerUnit->GetAbilitySystemComponent();
	if (!ASC->FindAbilitySpecFromClass(PrimaryAbilityClass) && OwnerUnit->HasAuthority())
	{
		ASC->GiveAbility(FGameplayAbilitySpec(PrimaryAbilityClass, 1));
	}
	if (ASC->TryActivateAbilityByClass(PrimaryAbilityClass))
	{
		NextActionTime = Now + ActionInterval;
		return true;
	}
	return false;
}

void USunriseWeapon::ExecutePrimaryAbility()
{
	ApplyDamage(PendingTarget.Get());
}

void USunriseWeapon::ExecuteAreaAbility()
{
}

void USunriseWeapon::GrantAbilities()
{
	if (!OwnerUnit || !OwnerUnit->HasAuthority() || !OwnerUnit->GetAbilitySystemComponent())
	{
		return;
	}
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilityClasses)
	{
		if (AbilityClass)
		{
			GrantedAbilityHandles.Add(OwnerUnit->GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1)));
		}
	}
}

void USunriseWeapon::ApplyDamage(ASunriseUnit* Target, float PowerScale, bool bArea)
{
	if (!CanTarget(Target))
	{
		return;
	}
	OwnerUnit->DealWeaponDamage(Target, OwnerUnit->GetAttackPowerAttribute() * PowerScale);
	OwnerUnit->NotifyWeaponAction(Target, false, bArea);
	BP_WeaponAction(Target, false, bArea);
}

void USunriseWeapon::ApplyHealing(ASunriseUnit* Target, float PowerScale, bool bArea)
{
	if (!OwnerUnit || !Target || !Target->IsAlive() || Target->GetTeamId() != OwnerUnit->GetTeamId())
	{
		return;
	}
	OwnerUnit->DealWeaponHealing(Target, OwnerUnit->GetAttackPowerAttribute() * PowerScale);
	OwnerUnit->NotifyWeaponAction(Target, true, bArea);
	BP_WeaponAction(Target, true, bArea);
}

USunriseSwordWeapon::USunriseSwordWeapon()
{
	WeaponType = ESunriseWeaponType::Sword;
	DisplayName = FText::FromString(TEXT("Sword"));
	Power = 26.0f;
	ActionRange = 250.0f;
	ActionInterval = 0.65f;
	PrimaryAbilityClass = USunriseSwordAttackAbility::StaticClass();
	GrantedAbilityClasses = {PrimaryAbilityClass};
}

USunriseBowWeapon::USunriseBowWeapon()
{
	WeaponType = ESunriseWeaponType::Bow;
	DisplayName = FText::FromString(TEXT("Bow"));
	Power = 44.0f;
	ActionRange = 1050.0f;
	ActionInterval = 1.8f;
	PrimaryAbilityClass = USunriseBowAttackAbility::StaticClass();
	GrantedAbilityClasses = {PrimaryAbilityClass};
}

void USunriseBowWeapon::ExecutePrimaryAbility()
{
	if (!OwnerUnit || !PendingTarget.IsValid() || !GetWorld())
	{
		return;
	}
	const FVector Start = OwnerUnit->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const FVector Direction = (PendingTarget->GetActorLocation() - Start).GetSafeNormal();
	FActorSpawnParameters Parameters;
	Parameters.Owner = OwnerUnit;
	Parameters.Instigator = OwnerUnit;
	if (ASunriseProjectile* Projectile =
			GetWorld()->SpawnActor<ASunriseProjectile>(ASunriseProjectile::StaticClass(), Start, Direction.Rotation(), Parameters))
	{
		Projectile->InitializeProjectile(OwnerUnit, PendingTarget.Get(), OwnerUnit->GetAttackPowerAttribute());
	}
	OwnerUnit->NotifyWeaponAction(PendingTarget.Get(), false, false);
	BP_WeaponAction(PendingTarget.Get(), false, false);
}
USunriseDrumsWeapon::USunriseDrumsWeapon()
{
	WeaponType = ESunriseWeaponType::Drums;
	DisplayName = FText::FromString(TEXT("Healing Drums"));
	Power = 12.0f;
	ActionRange = 525.0f;
	ActionInterval = 1.6f;
	PrimaryAbilityClass = USunriseDrumsHealAbility::StaticClass();
	GrantedAbilityClasses = {PrimaryAbilityClass};
}

bool USunriseDrumsWeapon::CanTarget(const ASunriseUnit* Target) const
{
	return OwnerUnit && IsValid(Target) && Target->IsAlive() && Target != OwnerUnit && Target->GetTeamId() == OwnerUnit->GetTeamId() &&
		   Target->GetHealthPercent() < 0.999f;
}

void USunriseDrumsWeapon::ExecutePrimaryAbility()
{
	ApplyHealing(PendingTarget.Get());
}

USunriseStaffWeapon::USunriseStaffWeapon()
{
	WeaponType = ESunriseWeaponType::Staff;
	DisplayName = FText::FromString(TEXT("Staff"));
	Power = 24.0f;
	ActionRange = 600.0f;
	ActionInterval = 1.1f;
	PrimaryAbilityClass = USunriseStaffAttackAbility::StaticClass();
	AreaAbilityClass = USunriseStaffAreaAttackAbility::StaticClass();
	GrantedAbilityClasses = {PrimaryAbilityClass, AreaAbilityClass};
}

void USunriseStaffWeapon::ExecutePrimaryAbility()
{
	Super::ExecutePrimaryAbility();
	if (++AttackCounter >= AttacksPerAreaBurst && AreaAbilityClass && OwnerUnit)
	{
		AttackCounter = 0;
		OwnerUnit->GetAbilitySystemComponent()->TryActivateAbilityByClass(AreaAbilityClass);
	}
}

void USunriseStaffWeapon::ExecuteAreaAbility()
{
	if (!OwnerUnit || !PendingTarget.IsValid())
	{
		return;
	}
	const FVector Direction = (PendingTarget->GetActorLocation() - OwnerUnit->GetActorLocation()).GetSafeNormal();
	FActorSpawnParameters Parameters;
	Parameters.Owner = OwnerUnit;
	if (ASunriseAreaIndicator* Indicator = GetWorld()->SpawnActor<ASunriseAreaIndicator>(
			ASunriseAreaIndicator::StaticClass(), PendingTarget->GetActorLocation(), Direction.Rotation(), Parameters))
	{
		Indicator->InitializeIndicator(AreaRadius);
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SunriseStaffArea), false, OwnerUnit);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps, PendingTarget->GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(AreaRadius), Params);
	TSet<ASunriseUnit*> Applied;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASunriseUnit* const Unit = Cast<ASunriseUnit>(Overlap.GetActor());
		if (Unit && !Applied.Contains(Unit) && CanTarget(Unit))
		{
			Applied.Add(Unit);
			ApplyDamage(Unit, AreaPowerScale, true);
		}
	}
}

void USunriseSpearShieldWeapon::ExecutePrimaryAbility()
{
	Super::ExecutePrimaryAbility();
	if (!OwnerUnit || !GetWorld())
	{
		return;
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SunriseVanguardFocus), false, OwnerUnit);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps, OwnerUnit->GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(FocusRadius), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (ASunriseUnit* Unit = Cast<ASunriseUnit>(Overlap.GetActor()))
		{
			Unit->ApplyFocusTarget(OwnerUnit, FocusDuration);
		}
	}
}
USunriseSpearShieldWeapon::USunriseSpearShieldWeapon()
{
	WeaponType = ESunriseWeaponType::SpearShield;
	DisplayName = FText::FromString(TEXT("Spear and Shield"));
	Power = 22.0f;
	ActionRange = 285.0f;
	ActionInterval = 0.95f;
	PrimaryAbilityClass = USunriseSpearShieldAttackAbility::StaticClass();
	GrantedAbilityClasses = {PrimaryAbilityClass};
}
