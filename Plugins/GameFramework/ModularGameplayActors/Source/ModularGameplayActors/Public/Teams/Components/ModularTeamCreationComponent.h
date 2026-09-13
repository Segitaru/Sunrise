// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/GameStateComponent.h"

#include "ModularTeamCreationComponent.generated.h"

class UExperienceDefinition;
class AModularTeamPublicInfo;
class AModularTeamPrivateInfo;
class AModularPlayerState;
class AGameModeBase;
class APlayerController;
class UModularTeamDisplayAsset;

UCLASS(Blueprintable)
class UModularTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UModularTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	void OnExperienceLoaded(const UExperienceDefinition* Experience);

protected:
	// List of teams to create (id to display asset mapping, the display asset can be left unset if desired)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TMap<uint8, TObjectPtr<UModularTeamDisplayAsset>> TeamsToCreate;

	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TSubclassOf<AModularTeamPublicInfo> PublicTeamInfoClass;

	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TSubclassOf<AModularTeamPrivateInfo> PrivateTeamInfoClass;

#if WITH_SERVER_CODE
protected:
	virtual void ServerCreateTeams();
	virtual void ServerAssignPlayersToTeams();

	/** Sets the team ID for the given player state. Spectator-only player states will be stripped of any team association. */
	virtual void ServerChooseTeamForPlayer(AModularPlayerState* PS);

private:
	void OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer);
	void ServerCreateTeam(int32 TeamId, UModularTeamDisplayAsset* DisplayAsset);

	/** returns the Team ID with the fewest active players, or INDEX_NONE if there are no valid teams */
	int32 GetLeastPopulatedTeamID() const;
#endif
};
