// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/AI/OverloadEnvQueryContext_HackTarget.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "Units/SunriseUnit.h"

void UOverloadEnvQueryContext_HackTarget::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const ASunriseUnit* QuerierUnit = Cast<ASunriseUnit>(QueryInstance.Owner.Get());
	if (!QuerierUnit)
	{
		return;
	}
	const UOverloadInteractorComponent* Interactor = QuerierUnit->FindComponentByClass<UOverloadInteractorComponent>();
	const AActor* Target = Interactor ? Interactor->GetHackTarget() : nullptr;
	if (!IsValid(Target) || !Target->Implements<UOverloadHackable>())
	{
		return;
	}
	UEnvQueryItemType_Point::SetContextHelper(ContextData, IOverloadHackable::Execute_GetHackLocation(Target));
}
