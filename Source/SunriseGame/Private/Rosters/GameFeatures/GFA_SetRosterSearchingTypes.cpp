// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/GameFeatures/GFA_SetRosterSearchingTypes.h"
#include "GameFramework/GameStateBase.h"
#include "Rosters/Components/GamePawnRosterComponent.h"

void UGFA_SetRosterSearchingTypes::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			const AGameStateBase* GameState = WorldContext.World()->GetGameState();
			if (!GameState)
			{
				return;
			}

			if (UGamePawnRosterComponent* GamePawnRosterComponent = GameState->FindComponentByClass<UGamePawnRosterComponent>())
			{
				for (const FPrimaryAssetType& AssetsType : SearchingAssetsTypesToRemove)
				{
					GamePawnRosterComponent->SearchingAssetsTypes.Remove(AssetsType);
				}

				for (const FPrimaryAssetType& AssetsType : SearchingAssetsTypesToAdd)
				{
					GamePawnRosterComponent->SearchingAssetsTypes.AddUnique(AssetsType);
				}
			}
		}
	}
}
