// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"

#include "OverloadEnvQueryContext_HackTarget.generated.h"

/**
 * EQS context that exposes the current Overload hack target location for the owning unit.
 */
UCLASS()
class SUNRISEGAME_API UOverloadEnvQueryContext_HackTarget : public UEnvQueryContext
{
	GENERATED_BODY()

protected:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
