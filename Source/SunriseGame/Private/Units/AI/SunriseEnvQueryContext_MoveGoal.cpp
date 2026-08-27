// Copyright Epic Games, Inc. All Rights Reserved.


#include "Units/AI/SunriseEnvQueryContext_MoveGoal.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Units/SunriseUnit.h"

void UEnvQueryContext_MoveGoal::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// get the querying unit
	if (ASunriseUnit* QuerierActor = Cast<ASunriseUnit>(QueryInstance.Owner.Get()))
	{
		// add the last recorded danger location to the context
		UEnvQueryItemType_Point::SetContextHelper(ContextData, QuerierActor->GetMovementGoal());
	}
}
