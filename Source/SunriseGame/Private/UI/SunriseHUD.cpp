// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseHUD.h"

#include <initializer_list>

#include "Components/GameFrameworkComponentManager.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameModes/Overload/Types/OverloadTeamIds.h"
#include "GameModes/SunriseGameMatchComponent.h"
#include "Input/ModularInputConfig.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Pawn/Components/ModularPawnExtensionComponent.h"
#include "Pawn/ModularPawnData.h"
#include "Player/SunrisePlayerController.h"
#include "UI/Components/SunriseHUDComponent.h"
#include "UI/SunriseUI.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"


namespace SunriseHUDInput
{
	FString CompactKeyName(const FKey& Key)
	{
		if (Key == EKeys::LeftMouseButton)
		{
			return TEXT("LMB");
		}
		if (Key == EKeys::RightMouseButton)
		{
			return TEXT("RMB");
		}
		if (Key == EKeys::MiddleMouseButton)
		{
			return TEXT("MMB");
		}
		return Key.GetDisplayName().ToString();
	}

	FString GetKeysForAction(ASunrisePlayerController* PC, TFunctionRef<bool(const UInputAction*)> Predicate)
	{
		const ASunrisePawn* Pawn = PC ? Cast<ASunrisePawn>(PC->GetPawn()) : nullptr;
		const UModularPawnExtensionComponent* Extension = Pawn ? UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn) : nullptr;
		const UModularPawnData* PawnData = Extension ? Extension->GetPawnData<UModularPawnData>() : nullptr;
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		const UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		if (!PawnData || !InputSubsystem)
		{
			return TEXT("-");
		}

		for (const UModularInputConfig* Config : PawnData->InputConfigs)
		{
			if (!Config)
			{
				continue;
			}
			TArray<const UInputAction*> Actions;
			for (const FModularInputAction& Entry : Config->NativeInputActions)
			{
				if (Entry.InputAction && Predicate(Entry.InputAction))
				{
					Actions.Add(Entry.InputAction);
				}
			}
			for (const FModularInputAction& Entry : Config->AbilityInputActions)
			{
				if (Entry.InputAction && Predicate(Entry.InputAction))
				{
					Actions.Add(Entry.InputAction);
				}
			}
			for (const UInputAction* Action : Actions)
			{
				TArray<FString> Names;
				for (const FKey& Key : InputSubsystem->QueryKeysMappedToAction(Action))
				{
					Names.AddUnique(CompactKeyName(Key));
				}
				if (!Names.IsEmpty())
				{
					return FString::Join(Names, TEXT("/"));
				}
			}
		}
		return TEXT("-");
	}

	FString GetActionKeys(ASunrisePlayerController* PC, std::initializer_list<const TCHAR*> Fragments)
	{
		return GetKeysForAction(PC,
			[Fragments](const UInputAction* Action)
			{
				const FString Name = Action ? Action->GetName() : FString();
				for (const TCHAR* Fragment : Fragments)
				{
					if (Name.Contains(Fragment, ESearchCase::IgnoreCase))
					{
						return true;
					}
				}
				return false;
			});
	}
} // namespace SunriseHUDInput

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

void ASunriseHUD::DrawSelectedUnitsCount(ASunrisePlayerController* PC)
{
	const auto* const EntitiesManager = PC->FindComponentByClass<UControllableEntitiesManager>();
	if (!EntitiesManager)
	{
		return;
	}

	TArray<ASunriseUnit*> Units;

	for (AActor* CurrentEntity : EntitiesManager->GetSelectedEntities())
	{
		ASunriseUnit* const Unit = Cast<ASunriseUnit>(CurrentEntity);
		if (IsValid(Unit) && Unit->IsAlive() && EntitiesManager && EntitiesManager->CanControlEntity(Unit))
		{
			Units.Add(Unit);
		}
	}

	if (UIWidget)
	{
		UIWidget->SetSelectedUnitsCount(Units.Num());
	}
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
		FVector2D StartScreen;
		FVector2D CurrentScreen;
		if (PC->ProjectWorldLocationToScreen(BoxStartWorldLocation, StartScreen, true) &&
			PC->ProjectWorldLocationToScreen(BoxCurrentWorldLocation, CurrentScreen, true))
		{
			const FVector2D Min(FMath::Min(StartScreen.X, CurrentScreen.X), FMath::Min(StartScreen.Y, CurrentScreen.Y));
			const FVector2D Max(FMath::Max(StartScreen.X, CurrentScreen.X), FMath::Max(StartScreen.Y, CurrentScreen.Y));
			DrawRect(SelectionBoxColor, Min.X, Min.Y, Max.X - Min.X, Max.Y - Min.Y);
		}
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

	DrawSelectedUnitsCount(PC);
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

void ASunriseHUD::DragSelectUpdate(FVector StartWorldLocation, FVector CurrentWorldLocation, bool bDraw)
{
	bDrawBox = bDraw;
	BoxStartWorldLocation = StartWorldLocation;
	BoxCurrentWorldLocation = CurrentWorldLocation;
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
		const FLinearColor TeamColor = GetTeamColor(Unit->GetTeamId());
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
	if (!PC)
	{
		return;
	}

	DrawRect(FLinearColor(0.01f, 0.015f, 0.025f, 0.78f), 18.0f, 18.0f, 310.0f, 128.0f);
	if (GameMode)
	{
		DrawText(
			FString::Printf(TEXT("PLAYER  %d"), GameMode->GetFriendlyAlive()), FriendlyColor, 34.0f, 30.0f, GEngine->GetMediumFont(), 1.2f);
		DrawText(FString::Printf(TEXT("ENEMY   %d"), GameMode->GetEnemyAlive()), EnemyColor, 180.0f, 30.0f, GEngine->GetMediumFont(), 1.2f);
	}
	else
	{
		DrawText(TEXT("RTS CONTROLS"), FLinearColor(0.95f, 0.75f, 0.16f), 34.0f, 30.0f, GEngine->GetMediumFont(), 1.0f);
	}


	const auto* const EntitiesManager = PC->FindComponentByClass<UControllableEntitiesManager>();
	TArray<ASunriseUnit*> Units;
	if (EntitiesManager)
	{
		for (AActor* CurrentEntity : EntitiesManager->GetSelectedEntities())
		{
			ASunriseUnit* const Unit = Cast<ASunriseUnit>(CurrentEntity);
			if (IsValid(Unit) && Unit->IsAlive() && EntitiesManager->CanControlEntity(Unit))
			{
				Units.Add(Unit);
			}
		}
	}

	DrawText(FString::Printf(TEXT("Selected: %d"), Units.Num()), FLinearColor::White, 34.0f, 58.0f, GEngine->GetSmallFont(), 1.15f);


	const FString SelectKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Selection"), TEXT("Select")});
	const FString BoxKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("SelectionArea")});
	const FString OrderKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Order")});
	const FString StopKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Stop")});
	const FString PrimaryKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Primary")});
	const FString SecondaryKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Secondary")});
	const FString OptionalKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Optional")});
	const FString UltimateKey = SunriseHUDInput::GetActionKeys(PC, {TEXT("Ultimate")});
	DrawText(FString::Printf(TEXT("%s select | %s box | %s order | %s stop"), *SelectKey, *BoxKey, *OrderKey, *StopKey),
		FLinearColor(0.75f, 0.8f, 0.85f), 34.0f, 83.0f, GEngine->GetSmallFont(), 0.82f);
	DrawText(FString::Printf(TEXT("%s/%s/%s/%s abilities | ESC pause"), *PrimaryKey, *SecondaryKey, *OptionalKey, *UltimateKey),
		FLinearColor(0.75f, 0.8f, 0.85f), 34.0f, 101.0f, GEngine->GetSmallFont(), 0.82f);
}

FLinearColor ASunriseHUD::GetTeamColor(int32 TeamId) const
{
	const ASunrisePlayerController* Controller = Cast<ASunrisePlayerController>(GetOwningPlayerController());
	const int32 PlayerTeamId = Controller ? Controller->GetControlledTeamId() : INDEX_NONE;
	if (TeamId == INDEX_NONE || TeamId == OverloadTeamIds::Neutral || PlayerTeamId == INDEX_NONE)
	{
		return FLinearColor(0.62f, 0.66f, 0.72f, 1.0f);
	}
	return TeamId == PlayerTeamId ? FriendlyColor : EnemyColor;
}
