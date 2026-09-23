#pragma once

#include "CoreMinimal.h"

#include "SurvivalResourceTypes.generated.h"

UENUM(BlueprintType)
enum class ESurvivalResourceType : uint8
{
	Food,
	Wood,
	Stone,
	Metal
};
