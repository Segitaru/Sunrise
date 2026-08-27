// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Units/SunriseUnitTypes.h"

#include "SunriseGameInstance.generated.h"

UCLASS()
class SUNRISEGAME_API USunriseProgressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	ESunriseDifficulty SelectedDifficulty = ESunriseDifficulty::Normal;

	UPROPERTY(SaveGame)
	TArray<FSunriseMatchRecord> MatchHistory;
};

/** Persistent settings and match history shared by menu and gameplay maps. */
UCLASS()
class SUNRISEGAME_API USunriseGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Difficulty")
	ESunriseDifficulty GetSelectedDifficulty() const { return SelectedDifficulty; }

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Difficulty")
	void SetSelectedDifficulty(ESunriseDifficulty NewDifficulty);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Difficulty")
	FSunriseDifficultyTuning GetDifficultyTuning() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|History")
	const TArray<FSunriseMatchRecord>& GetMatchHistory() const { return MatchHistory; }

	void RecordMatch(const FSunriseMatchRecord& Record);
	FText BuildHistoryText(int32 MaxEntries = 5) const;
	static FText GetDifficultyDisplayName(ESunriseDifficulty Difficulty);

private:
	void SaveProgress();

	UPROPERTY(VisibleInstanceOnly, Category = "Sunrise|Difficulty")
	ESunriseDifficulty SelectedDifficulty = ESunriseDifficulty::Normal;

	UPROPERTY(VisibleInstanceOnly, Category = "Sunrise|History")
	TArray<FSunriseMatchRecord> MatchHistory;

	static const FString SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;
	static constexpr int32 MaxStoredMatches = 20;
};
