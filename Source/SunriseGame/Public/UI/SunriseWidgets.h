// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Units/SunriseUnitTypes.h"

#include "SunriseWidgets.generated.h"

class SWidget;
class STextBlock;
class USunriseSettingsWidget;

/** Fully native main menu. A Blueprint wrapper is optional, not required. */
UCLASS()
class SUNRISEGAME_API USunriseMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply StartGame();
	FReply OpenSettings();
	FReply QuitGame();
	FReply SelectEasy();
	FReply SelectNormal();
	FReply SelectHard();
	FReply SetDifficulty(ESunriseDifficulty Difficulty);
	void RefreshDifficultyLabel();

	TSharedPtr<STextBlock> DifficultyLabel;

	UPROPERTY(Transient)
	TObjectPtr<USunriseSettingsWidget> SettingsWidget;

	bool bStartingGame = false;
};

/** Native pause menu used from Escape during a match. */
UCLASS()
class SUNRISEGAME_API USunrisePauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply ResumeGame();
	FReply ReturnToMenu();
	FReply QuitGame();
};

/** Clickable victory/defeat screen created by the elimination match component. */
UCLASS()
class SUNRISEGAME_API USunriseEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResult(ESunriseMatchResult InResult) { Result = InResult; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply ReturnToMenu();
	ESunriseMatchResult Result = ESunriseMatchResult::InProgress;
};

UCLASS()
class SUNRISEGAME_API USunriseOverloadInfoWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
