// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/SunriseGameInstance.h"

#include "Kismet/GameplayStatics.h"

void USunriseGameInstance::Init()
{
	Super::Init();
	if (USunriseProgressSaveGame* Save = Cast<USunriseProgressSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex)))
	{
		SelectedDifficulty = Save->SelectedDifficulty;
		MatchHistory = Save->MatchHistory;
	}
}

void USunriseGameInstance::SetSelectedDifficulty(ESunriseDifficulty NewDifficulty)
{
	SelectedDifficulty = NewDifficulty;
	SaveProgress();
}

FSunriseDifficultyTuning USunriseGameInstance::GetDifficultyTuning() const
{
	FSunriseDifficultyTuning Tuning;
	switch (SelectedDifficulty)
	{
		case ESunriseDifficulty::Easy:
			Tuning.EnemyCountMultiplier = 0.65f;
			Tuning.EnemyHealthMultiplier = 0.85f;
			Tuning.EnemyPowerMultiplier = 0.8f;
			break;
		case ESunriseDifficulty::Hard:
			Tuning.EnemyCountMultiplier = 2.0f;
			Tuning.EnemyHealthMultiplier = 1.35f;
			Tuning.EnemyPowerMultiplier = 1.25f;
			break;
		default:
			break;
	}
	return Tuning;
}

void USunriseGameInstance::RecordMatch(const FSunriseMatchRecord& Record)
{
	MatchHistory.Insert(Record, 0);
	if (MatchHistory.Num() > MaxStoredMatches)
	{
		MatchHistory.SetNum(MaxStoredMatches);
	}
	SaveProgress();
}

FText USunriseGameInstance::BuildHistoryText(int32 MaxEntries) const
{
	if (MatchHistory.IsEmpty())
	{
		return FText::FromString(TEXT("No completed matches yet"));
	}
	FString Result;
	const int32 Count = FMath::Min(MaxEntries, MatchHistory.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FSunriseMatchRecord& Record = MatchHistory[Index];
		const TCHAR* Outcome = Record.Result == ESunriseMatchResult::Victory ? TEXT("Victory") : TEXT("Defeat");
		Result += FString::Printf(TEXT("%d. %s | %s | %02d:%02d | allies %d | enemies %d"), Index + 1, Outcome,
			*GetDifficultyDisplayName(Record.Difficulty).ToString(), FMath::FloorToInt(Record.DurationSeconds / 60.0f),
			FMath::FloorToInt(FMath::Fmod(Record.DurationSeconds, 60.0f)), Record.FriendlySurvivors, Record.EnemiesDefeated);
		if (Index + 1 < Count)
		{
			Result += TEXT("\n");
		}
	}
	return FText::FromString(Result);
}

FText USunriseGameInstance::GetDifficultyDisplayName(ESunriseDifficulty Difficulty)
{
	switch (Difficulty)
	{
		case ESunriseDifficulty::Easy:
			return FText::FromString(TEXT("Easy"));
		case ESunriseDifficulty::Hard:
			return FText::FromString(TEXT("Hard"));
		default:
			return FText::FromString(TEXT("Normal"));
	}
}

void USunriseGameInstance::SaveProgress()
{
	USunriseProgressSaveGame* Save =
		Cast<USunriseProgressSaveGame>(UGameplayStatics::CreateSaveGameObject(USunriseProgressSaveGame::StaticClass()));
	if (Save)
	{
		Save->SelectedDifficulty = SelectedDifficulty;
		Save->MatchHistory = MatchHistory;
		UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, SaveUserIndex);
	}
}

const FString USunriseGameInstance::SaveSlotName = TEXT("SunriseSunriseProgress");
