#pragma once

#include "Engine/DataAsset.h"

#include "ControllableEntityDefinition.generated.h"

class ASunriseUnit;

/** Authored presentation/spawn definition for a controllable hero or summoned unit. */
UCLASS(BlueprintType)
class SUNRISEGAME_API UControllableEntityDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Control")
	TSoftClassPtr<ASunriseUnit> UnitClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Control")
	FText Name;
};
