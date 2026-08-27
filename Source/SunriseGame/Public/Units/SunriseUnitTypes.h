// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "SunriseUnitTypes.generated.h"

/** The two combat sides used by the RTS prototype. */
UENUM(BlueprintType)
enum class ESunriseTeam : uint8
{
	Friendly UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy"),
	Neutral UMETA(DisplayName = "Neutral")
};

/** A role changes the native default attributes and target-selection rules. */
UENUM(BlueprintType)
enum class ESunriseUnitRole : uint8
{
	Melee UMETA(DisplayName = "Swordsman"),
	Ranged UMETA(DisplayName = "Archer"),
	Healer UMETA(DisplayName = "Healer (Drums)"),
	Mage UMETA(DisplayName = "Mage (Staff)"),
	Vanguard UMETA(DisplayName = "Vanguard (Spear and Shield)")
};

/** Gameplay ownership class; independent from combat role and presentation loadout. */
UENUM(BlueprintType)
enum class ESunriseUnitKind : uint8
{
	Creep,
	Hero,
	Summoned
};

/** Broad combat responsibility; UnitRole below it is the concrete class/loadout. */
UENUM(BlueprintType)
enum class ESunriseCombatRole : uint8
{
	DamageDealer UMETA(DisplayName = "Damage Dealer"),
	Support UMETA(DisplayName = "Support"),
	Tank UMETA(DisplayName = "Tank")
};

UENUM(BlueprintType)
enum class ESunriseOrderState : uint8
{
	Idle,
	Moving,
	Attacking,
	Healing,
	Dead
};

UENUM(BlueprintType)
enum class ESunriseMatchResult : uint8
{
	InProgress,
	Victory,
	Defeat
};

UENUM(BlueprintType)
enum class ESunriseDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard UMETA(DisplayName = "Hard")
};

USTRUCT(BlueprintType)
struct FSunriseDifficultyTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float EnemyCountMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float EnemyHealthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float EnemyPowerMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FSunriseMatchRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	ESunriseMatchResult Result = ESunriseMatchResult::InProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	ESunriseDifficulty Difficulty = ESunriseDifficulty::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	float DurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	int32 FriendlySurvivors = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	int32 EnemiesDefeated = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Match")
	FDateTime CompletedAt = FDateTime::MinValue();
};

/** Initial values copied into GAS attributes; runtime code reads the AttributeSet, never this struct. */
USTRUCT(BlueprintType)
struct FSunriseUnitStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 150.0f;

	/** Damage for fighters and healing per action for healers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0"))
	float Power = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "50.0", Units = "cm"))
	float ActionRange = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.05", Units = "s"))
	float ActionInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", Units = "cm"))
	float AggroRadius = 1200.0f;
};
