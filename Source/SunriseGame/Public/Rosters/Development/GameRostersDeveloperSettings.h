// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "Rosters/Components/GamePawnSelectorComponent.h"
#include "GameRostersDeveloperSettings.generated.h"

USTRUCT(BlueprintType)
struct FPawnsDefinitions
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowedTypes = "GamePawnDefinition"))
	TArray<FPrimaryAssetId> PawnDefinitionsForTeam;
};

#if WITH_EDITORONLY_DATA

UCLASS(config = EditorPerProjectUserSettings, MinimalAPI)
class UGameRostersDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UGameRostersDeveloperSettings();
	
	virtual FName GetCategoryName() const override;

	/// Only PIE! If you don't want to get a random pawn to control, check this
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn")
	bool bOverridePawnDefinition = false;

	/// Only PIE! If you want to create the required set of pawns for teams, check this
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn",
		meta = (EditCondition = "bOverridePawnDefinition == true", EditConditionHides))
	bool bSeparatePawnsBetweenTeam = false;

	/// Only PIE! This is a specific option in order not to receive a pawn when starting a game mode,
	/// for example, in order to start with a clean lobby, which occurs before the match
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn", AdvancedDisplay)
	bool bTakePawnOnGameStart = true;

	/// Only PIE! If you don’t need to share pawn between teams, then you can use this to set a pawn for all active windows, regardless of the team
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn",
		meta = (AllowedTypes = "GamePawnDefinition",
			EditCondition = "bSeparatePawnsBetweenTeam == false && bOverridePawnDefinition == true",
			EditConditionHides))
	FPrimaryAssetId DefaultOverridePawn;

	/// Only PIE! A set of pawns that will be issued to each active window that you launched in the editor for an individual team member
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn",
		meta = (AllowedTypes = "GamePawnDefinition",
			EditCondition = "bSeparatePawnsBetweenTeam == true && bOverridePawnDefinition == true", EditConditionHides))
	TMap<int32, FPawnsDefinitions> PawnsForTeam;

	/// Only PIE! If necessary, you can specify that all bots on the map be of the class you selected
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn")
	bool bOverridePawnDefinitionForBots = false;

	/// Only PIE! Class of bots that will be used on the map
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pawn",
		meta = (AllowedTypes = "GamePawnDefinition",
			EditCondition = "bOverridePawnDefinitionForBots == true", EditConditionHides))
	FPrimaryAssetId BotDefaultOverridePawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pick")
	bool bOverrideCharacterPickMode = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category = "Pick",
		meta = (EditCondition = "bOverrideCharacterPickMode == true", EditConditionHides))
	TEnumAsByte<ECharacterPickMode> PickMode = FreeForAll;
};

#endif