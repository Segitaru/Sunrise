// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "OverloadTypes.generated.h"

UENUM(BlueprintType)
enum class EOverloadCoreState : uint8
{
	Stable,
	Overloading,
	Cooling,
	Destroyed
};

USTRUCT(BlueprintType)
struct SUNRISEGAME_API FOverloadTowerTierStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload")
	float MaxIntegrity = 350.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload")
	float AttackPower = 24.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload")
	float Armor = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload")
	float HackResistance = 1.0f;
};

USTRUCT(BlueprintType)
struct SUNRISEGAME_API FOverloadBalanceTuning
{
	GENERATED_BODY()

	/** Remaining original towers gain this fraction per lost supply point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload", meta = (ClampMin = "0.0"))
	float DefenderBoostPerLostPoint = 0.18f;

	/** All towers controlled by a leading team lose this fraction per foreign point held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload", meta = (ClampMin = "0.0"))
	float LeaderWeakeningPerCapturedPoint = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload", meta = (ClampMin = "0.1"))
	float CoreOverloadSeconds = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overload", meta = (ClampMin = "0.0"))
	float CoreCoolingPerSecond = 1.5f;
};
