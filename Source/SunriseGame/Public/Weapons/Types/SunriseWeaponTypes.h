// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SunriseWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ESunriseWeaponType : uint8
{
	Sword UMETA(DisplayName = "Sword"),
	Bow UMETA(DisplayName = "Bow"),
	Drums UMETA(DisplayName = "Healing Drums"),
	Staff UMETA(DisplayName = "Staff"),
	SpearShield UMETA(DisplayName = "Spear and Shield")
};

UENUM()
enum class ESunriseWeaponAbilityAction : uint8
{
	Primary,
	Area
};
