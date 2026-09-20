#pragma once

#include "CoreMinimal.h"

#include "PawnFormationTypes.generated.h"

class UModularPawnData;

USTRUCT(BlueprintType)
struct FPawnFormation
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UModularPawnData> Definition;

	UPROPERTY(EditDefaultsOnly)
	int32 Count = 1;

	UPROPERTY(EditDefaultsOnly)
	float OffsetInLine = 150.f;
};
