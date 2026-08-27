// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseWidgets.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Widget.h"
#include "GameSettingsLocal.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "System/SunriseGameInstance.h"
#include "UI/SunriseSettingsWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace SunriseWidgets
{
	static TSharedRef<SButton> MakeButton(const FText& Label, const FOnClicked& Handler)
	{
		return SNew(SButton)
			.OnClicked(Handler)
			.ContentPadding(FMargin(36.0f, 12.0f))
			.HAlign(HAlign_Center)[SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))];
	}

	static void OpenMenu(UObject* Context)
	{
		UGameplayStatics::SetGamePaused(Context, false);
		UGameplayStatics::OpenLevel(Context, FName(TEXT("/Game/Sunrise/Maps/Test/L_MainMenu")), true);
	}
} // namespace SunriseWidgets

TSharedRef<SWidget> USunriseMainMenuWidget::RebuildWidget()
{
	USunriseGameInstance* SunriseGI = GetGameInstance<USunriseGameInstance>();
	const FText History = SunriseGI ? SunriseGI->BuildHistoryText() : FText::FromString(TEXT("No completed matches yet"));
	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.015f, 0.025f, 0.04f, 0.98f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
			[SNew(SVerticalBox) +
				SVerticalBox::Slot().AutoHeight().Padding(20.0f)[SNew(STextBlock)
																	 .Text(FText::FromString(TEXT("SUNRISE COMMAND")))
																	 .ColorAndOpacity(FLinearColor(0.9f, 0.72f, 0.2f))
																	 .Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
																	 .Justification(ETextJustify::Center)] +
				SVerticalBox::Slot().AutoHeight().Padding(
					8.0f, 0.0f, 8.0f, 32.0f)[SNew(STextBlock)
												 .Text(FText::FromString(TEXT("Top Down real-time strategy prototype")))
												 .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
												 .Justification(ETextJustify::Center)] +
				SVerticalBox::Slot().AutoHeight().Padding(
					8.0f)[SAssignNew(DifficultyLabel, STextBlock)
							  .Text(SunriseGI ? FText::Format(FText::FromString(TEXT("Difficulty: {0}")),
													USunriseGameInstance::GetDifficultyDisplayName(SunriseGI->GetSelectedDifficulty()))
											  : FText::FromString(TEXT("Difficulty: Normal")))
							  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							  .Justification(ETextJustify::Center)] +
				SVerticalBox::Slot().AutoHeight().Padding(
					8.0f)[SNew(SHorizontalBox) +
						  SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SunriseWidgets::MakeButton(
							  FText::FromString(TEXT("EASY")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::SelectEasy))] +
						  SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SunriseWidgets::MakeButton(
							  FText::FromString(TEXT("NORMAL")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::SelectNormal))] +
						  SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SunriseWidgets::MakeButton(
							  FText::FromString(TEXT("HARD")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::SelectHard))]] +
				SVerticalBox::Slot().AutoHeight().Padding(8.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("START GAME")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::StartGame))] +
				SVerticalBox::Slot().AutoHeight().Padding(8.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("SETTINGS")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::OpenSettings))] +
				SVerticalBox::Slot().AutoHeight().Padding(8.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("EXIT")), FOnClicked::CreateUObject(this, &USunriseMainMenuWidget::QuitGame))] +
				SVerticalBox::Slot().AutoHeight().Padding(
					12.0f, 28.0f, 12.0f, 4.0f)[SNew(STextBlock)
												   .Text(FText::FromString(TEXT("MATCH HISTORY")))
												   .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
												   .Justification(ETextJustify::Center)] +
				SVerticalBox::Slot().AutoHeight().Padding(12.0f, 4.0f)[SNew(STextBlock)
																		   .Text(History)
																		   .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
																		   .Justification(ETextJustify::Left)]];
}

FReply USunriseMainMenuWidget::StartGame()
{
	if (bStartingGame)
	{
		return FReply::Handled();
	}
	bStartingGame = true;
	UGameSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(false);
	RemoveFromParent();
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Sunrise/Maps/Test/L_Overload")), true, TEXT("Experience=Experience_Overload"));
	return FReply::Handled();
}

FReply USunriseMainMenuWidget::OpenSettings()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return FReply::Handled();
	}

	SettingsWidget = CreateWidget<USunriseSettingsWidget>(PlayerController, USunriseSettingsWidget::StaticClass());
	if (SettingsWidget)
	{
		SettingsWidget->SetOwningMenu(this);
		SetVisibility(ESlateVisibility::Collapsed);
		SettingsWidget->AddToViewport(101);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, SettingsWidget, EMouseLockMode::DoNotLock);
	}
	return FReply::Handled();
}

FReply USunriseMainMenuWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	return FReply::Handled();
}
FReply USunriseMainMenuWidget::SelectEasy()
{
	return SetDifficulty(ESunriseDifficulty::Easy);
}
FReply USunriseMainMenuWidget::SelectNormal()
{
	return SetDifficulty(ESunriseDifficulty::Normal);
}

FReply USunriseMainMenuWidget::SelectHard()
{
	return SetDifficulty(ESunriseDifficulty::Hard);
}

FReply USunriseMainMenuWidget::SetDifficulty(ESunriseDifficulty Difficulty)
{
	if (USunriseGameInstance* SunriseGI = GetGameInstance<USunriseGameInstance>())
	{
		SunriseGI->SetSelectedDifficulty(Difficulty);
		RefreshDifficultyLabel();
	}
	return FReply::Handled();
}

void USunriseMainMenuWidget::RefreshDifficultyLabel()
{
	if (DifficultyLabel)
	{
		const USunriseGameInstance* SunriseGI = GetGameInstance<USunriseGameInstance>();
		DifficultyLabel->SetText(SunriseGI ? FText::Format(FText::FromString(TEXT("Difficulty: {0}")),
												 USunriseGameInstance::GetDifficultyDisplayName(SunriseGI->GetSelectedDifficulty()))
										   : FText::FromString(TEXT("Difficulty: Normal")));
	}
}

TSharedRef<SWidget> USunrisePauseMenuWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.015f, 0.9f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
			[SNew(SVerticalBox) +
				SVerticalBox::Slot().AutoHeight().Padding(
					12.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("PAUSED"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 36))] +
				SVerticalBox::Slot().AutoHeight().Padding(6.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("RESUME")), FOnClicked::CreateUObject(this, &USunrisePauseMenuWidget::ResumeGame))] +
				SVerticalBox::Slot().AutoHeight().Padding(6.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("MAIN MENU")), FOnClicked::CreateUObject(this, &USunrisePauseMenuWidget::ReturnToMenu))] +
				SVerticalBox::Slot().AutoHeight().Padding(6.0f)[SunriseWidgets::MakeButton(
					FText::FromString(TEXT("EXIT")), FOnClicked::CreateUObject(this, &USunrisePauseMenuWidget::QuitGame))]];
}

FReply USunrisePauseMenuWidget::ResumeGame()
{
	UGameplayStatics::SetGamePaused(this, false);
	RemoveFromParent();
	if (APlayerController* PC = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(PC, nullptr, EMouseLockMode::DoNotLock, false);
	}
	return FReply::Handled();
}

FReply USunrisePauseMenuWidget::ReturnToMenu()
{
	SunriseWidgets::OpenMenu(this);
	return FReply::Handled();
}

FReply USunrisePauseMenuWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	return FReply::Handled();
}

TSharedRef<SWidget> USunriseEndScreenWidget::RebuildWidget()
{
	const bool bVictory = Result == ESunriseMatchResult::Victory;
	const FLinearColor Accent = bVictory ? FLinearColor(0.1f, 0.85f, 0.3f) : FLinearColor(0.95f, 0.12f, 0.08f);
	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.015f, 0.92f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)[SNew(SVerticalBox) +
							   SVerticalBox::Slot().AutoHeight().Padding(
								   18.0f)[SNew(STextBlock)
											  .Text(FText::FromString(bVictory ? TEXT("VICTORY") : TEXT("DEFEAT")))
											  .ColorAndOpacity(Accent)
											  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 52))] +
							   SVerticalBox::Slot().AutoHeight().Padding(
								   8.0f)[SunriseWidgets::MakeButton(FText::FromString(TEXT("RETURN TO MAIN MENU")),
								   FOnClicked::CreateUObject(this, &USunriseEndScreenWidget::ReturnToMenu))]];
}

FReply USunriseEndScreenWidget::ReturnToMenu()
{
	SunriseWidgets::OpenMenu(this);
	return FReply::Handled();
}

TSharedRef<SWidget> USunriseOverloadInfoWidget::RebuildWidget()
{
	return SNew(SBox).WidthOverride(430.0f)
		[SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.9f))
				.Padding(FMargin(16.0f))
					[SNew(SVerticalBox) +
						SVerticalBox::Slot().AutoHeight()
							[SNew(STextBlock).Text(FText::FromString(TEXT("OVERLOAD"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))] +
						SVerticalBox::Slot().AutoHeight().Padding(
							0.0f, 6.0f)[SNew(STextBlock)
											.Text(FText::FromString(
												TEXT("Захватите терминалы на пути к вражескому ядру и перегрузите его энергией")))
											.AutoWrapText(true)
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))] +
						SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[SNew(STextBlock)
									.Text(FText::FromString(TEXT(
										"WSAD: перемещение камеры\nQ/E: поворот камеры\nMouse Wheel: Zoom\nT: призыв юнитов\nLMB: выбор юнитов\nRMB: отправка приказов")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))]]];
}