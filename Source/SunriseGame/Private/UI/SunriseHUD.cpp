// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseHUD.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameModes/SunriseGameMatchComponent.h"
#include "Player/SunrisePlayerController.h"
#include "UI/Components/SunriseHUDComponent.h"
#include "UI/SunriseUI.h"
#include "Units/SunriseUnit.h"

void ASunriseHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ASunriseHUD::BeginPlay()
{
	Super::BeginPlay();
	if (UIWidgetClass)
	{
		UIWidget = CreateWidget<USunriseUI>(GetOwningPlayerController(), UIWidgetClass);
		if (UIWidget)
		{
			UIWidget->AddToViewport(0);
		}
	}
}

void ASunriseHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void ASunriseHUD::DrawHUD()
{
	Super::DrawHUD();
	ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(GetOwningPlayerController());
	if (!PC)
	{
		return;
	}

	if (bDrawBox)
	{
		const FVector2D Min(FMath::Min(BoxStart.X, BoxCurrentPosition.X), FMath::Min(BoxStart.Y, BoxCurrentPosition.Y));
		const FVector2D Max(FMath::Max(BoxStart.X, BoxCurrentPosition.X), FMath::Max(BoxStart.Y, BoxCurrentPosition.Y));
		DrawRect(SelectionBoxColor, Min.X, Min.Y, Max.X - Min.X, Max.Y - Min.Y);
		TArray<ASunriseUnit*> BoxedUnits;
		for (TActorIterator<ASunriseUnit> It(GetWorld()); It; ++It)
		{
			ASunriseUnit* Unit = *It;
			if (!Unit->IsAlive() || Unit->GetTeamId() != PC->GetControlledTeamId())
			{
				continue;
			}
			FVector2D ScreenPosition;
			if (PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPosition, true) && ScreenPosition.X >= Min.X &&
				ScreenPosition.X <= Max.X && ScreenPosition.Y >= Min.Y && ScreenPosition.Y <= Max.Y)
			{
				BoxedUnits.Add(Unit);
			}
		}
		PC->DragSelectUnits(BoxedUnits);
	}
	if (bDrawCommandDrag && CommandDragUnit.IsValid())
	{
		FVector2D UnitScreen;
		if (PC->ProjectWorldLocationToScreen(CommandDragUnit->GetActorLocation(), UnitScreen, true))
		{
			DrawLine(UnitScreen.X, UnitScreen.Y, CommandDragCursor.X, CommandDragCursor.Y, FLinearColor(1.0f, 0.75f, 0.1f, 1.0f), 3.0f);
			DrawText(TEXT("ORDER"), FLinearColor(1.0f, 0.75f, 0.1f, 1.0f), CommandDragCursor.X + 10.0f, CommandDragCursor.Y + 8.0f,
				GEngine->GetSmallFont(), 1.15f);
		}
	}

	const TArray<ASunriseUnit*>& Selected = PC->GetSelectedUnits();
	if (UIWidget)
	{
		UIWidget->SetSelectedUnitsCount(Selected.Num());
	}
	DrawUnitOverlays();
	DrawMatchPanel();

	TInlineComponentArray<USunriseHUDComponent*> HUDComponents(this);
	for (USunriseHUDComponent* HUDComponent : HUDComponents)
	{
		if (IsValid(HUDComponent))
		{
			HUDComponent->DrawHUD(this);
		}
	}
}

void ASunriseHUD::DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw)
{
	bDrawBox = bDraw;
	BoxStart = Start;
	BoxSize = WidthAndHeight;
	BoxCurrentPosition = CurrentPosition;
}

void ASunriseHUD::CommandDragUpdate(ASunriseUnit* SourceUnit, FVector2D CursorPosition, bool bDraw)
{
	CommandDragUnit = SourceUnit;
	CommandDragCursor = CursorPosition;
	bDrawCommandDrag = bDraw;
}

void ASunriseHUD::DrawUnitOverlays()
{
	ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(GetOwningPlayerController());
	if (!PC)
	{
		return;
	}

	for (TActorIterator<ASunriseUnit> It(GetWorld()); It; ++It)
	{
		ASunriseUnit* Unit = *It;
		if (!Unit->IsAlive() || !Unit->WasRecentlyRendered(0.25f))
		{
			continue;
		}
		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(Unit->GetActorLocation() + FVector(0.0f, 0.0f, 115.0f), Screen, true))
		{
			continue;
		}

		const float Width = 54.0f;
		const FLinearColor TeamColor = Unit->GetTeamId() == PC->GetControlledTeamId() ? FriendlyColor : EnemyColor;
		DrawRect(FLinearColor(0.015f, 0.015f, 0.015f, 0.9f), Screen.X - Width * 0.5f, Screen.Y, Width, 7.0f);
		DrawRect(TeamColor, Screen.X - Width * 0.5f + 1.0f, Screen.Y + 1.0f, (Width - 2.0f) * Unit->GetHealthPercent(), 5.0f);

		if (Unit->IsSelected())
		{
			if (ASunriseUnit* Target = Unit->GetActionTarget())
			{
				FVector2D TargetScreen;
				if (PC->ProjectWorldLocationToScreen(Target->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f), TargetScreen, true))
				{
					DrawLine(Screen.X, Screen.Y, TargetScreen.X, TargetScreen.Y, FLinearColor(1.0f, 0.8f, 0.15f, 0.9f), 2.5f);
					DrawRect(FLinearColor(1.0f, 0.8f, 0.15f, 0.9f), TargetScreen.X - 8.0f, TargetScreen.Y - 8.0f, 16.0f, 2.0f);
					DrawRect(FLinearColor(1.0f, 0.8f, 0.15f, 0.9f), TargetScreen.X - 8.0f, TargetScreen.Y + 6.0f, 16.0f, 2.0f);
				}
			}
		}

		const FString UnitClassName = Unit->GetUnitClassDisplayName().ToString();
		float TextWidth = 0.0f;
		float TextHeight = 0.0f;
		Canvas->StrLen(GEngine->GetSmallFont(), UnitClassName, TextWidth, TextHeight);
		DrawText(UnitClassName, TeamColor, Screen.X - TextWidth * 0.45f, Screen.Y - TextHeight - 3.0f, GEngine->GetSmallFont(), 1.15f);
	}
}

void ASunriseHUD::DrawMatchPanel()
{
	const USunriseGameMatchComponent* GameMode = USunriseGameMatchComponent::Find(this);
	ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(GetOwningPlayerController());
	if (!GameMode || !PC)
	{
		return;
	}

	DrawRect(FLinearColor(0.01f, 0.015f, 0.025f, 0.78f), 18.0f, 18.0f, 310.0f, 105.0f);
	DrawText(
		FString::Printf(TEXT("PLAYER  %d"), GameMode->GetFriendlyAlive()), FriendlyColor, 34.0f, 30.0f, GEngine->GetMediumFont(), 1.2f);
	DrawText(FString::Printf(TEXT("ENEMY   %d"), GameMode->GetEnemyAlive()), EnemyColor, 180.0f, 30.0f, GEngine->GetMediumFont(), 1.2f);
	DrawText(FString::Printf(TEXT("Selected: %d"), PC->GetSelectedUnits().Num()), FLinearColor::White, 34.0f, 58.0f,
		GEngine->GetSmallFont(), 1.15f);
	DrawText(TEXT("LMB select/box | Drag unit to target | RMB move/camera | Edge scroll | Esc"), FLinearColor(0.75f, 0.8f, 0.85f), 34.0f,
		83.0f, GEngine->GetSmallFont(), 0.95f);
}
