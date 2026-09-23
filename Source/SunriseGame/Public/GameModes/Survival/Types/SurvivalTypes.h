#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "SurvivalTypes.generated.h"

UENUM(BlueprintType)
enum class ESurvivalMatchState : uint8
{
	Initializing,
	InProgress,
	Victory,
	Defeat
};

UENUM(BlueprintType)
enum class ESurvivalBuildingRole : uint8
{
	Generic,
	MainBase,
	Barracks,
	Housing,
	Defense,
	Storehouse
};

UENUM(BlueprintType)
enum class ESurvivalBuildFailure : uint8
{
	None,
	NotAuthority,
	MatchEnded,
	UnknownDefinition,
	InvalidBuilder,
	InvalidTransform,
	OutOfRange,
	Blocked,
	MissingDependency,
	LimitReached,
	InsufficientResources,
	SpawnFailed
};

USTRUCT(BlueprintType)
struct FSurvivalResourceAmounts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Food = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Wood = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Stone = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Metal = 0.0f;

	bool IsNonNegative() const { return Food >= 0.0f && Wood >= 0.0f && Stone >= 0.0f && Metal >= 0.0f; }
};

class UModularPawnData;

USTRUCT(BlueprintType)
struct FSurvivalWaveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UModularPawnData> UnitDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnRadius = 250.0f;
};

USTRUCT(BlueprintType)
struct FSurvivalWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", Units = "s"))
	float Delay = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSurvivalWaveEntry> Entries;
};
