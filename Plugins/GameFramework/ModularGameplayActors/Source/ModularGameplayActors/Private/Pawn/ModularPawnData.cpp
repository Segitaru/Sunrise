// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawn/ModularPawnData.h"

void FGrantedPawnComponents::AddComponent(UActorComponent* Component)
{
	SpawnedComponents.Add(Component);
}

void FPawnComponentList::GiveComponentsToActor(AActor* OwnerActor, FGrantedPawnComponents* IssuedComponents) const
{
	if (!OwnerActor)
	{
		return;
	}

	for (const auto& Component : Components)
	{
		if (const auto LoadedComponent = Component.LoadSynchronous())
		{
			if (UActorComponent* NewComponent = OwnerActor->AddComponentByClass(LoadedComponent, false, FTransform(), false))
			{
				IssuedComponents->AddComponent(NewComponent);
			}
		}
	}
}

void FGrantedPawnComponents::TakeFromActor(const AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return;
	}

	for (auto const Component : SpawnedComponents)
	{
		Component->DestroyComponent();
	}
	SpawnedComponents.Empty();
}

UModularPawnData::UModularPawnData(const FObjectInitializer& ObjectInitializer)
{
}

bool UModularPawnData::ShowInGame() const
{
#if WITH_EDITOR
	if (bShowInEditor)
	{
		return true;
	}
#endif

	return bShowInFrontend;
}

FPrimaryAssetId UModularPawnData::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

EDataValidationResult UModularPawnData::IsDataValid(FDataValidationContext& Context) const
{
	return Super::IsDataValid(Context);
}