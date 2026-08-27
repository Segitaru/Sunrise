// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SunriseWeaponEffects.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	USunriseDamageEffect();
	static FName GetMagnitudeDataName() { return TEXT("Sunrise.Weapon.Damage"); }
};

UCLASS()
class SUNRISEGAME_API USunriseHealingEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	USunriseHealingEffect();
	static FName GetMagnitudeDataName() { return TEXT("Sunrise.Weapon.Healing"); }
};
