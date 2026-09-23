#include "GameModes/Survival/UI/SurvivalHUDComponent.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Actors/SurvivalBuildingPlacementPreview.h"
#include "GameModes/Survival/Components/SurvivalBuildComponent.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/Components/SurvivalProductionComponent.h"
#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "GameModes/Survival/SurvivalGameplayTags.h"
#include "InputCoreTypes.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"
#include "UI/SunriseWidgets.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalHUDComponent)

namespace SurvivalHUD
{
	constexpr float PanelX = 20.0f;
	constexpr float PanelBottomMargin = 20.0f;
	constexpr float ButtonWidth = 190.0f;
	constexpr float ButtonHeight = 28.0f;
	constexpr float ButtonGap = 4.0f;

	const TCHAR* GetStateLabel(ESurvivalMatchState State)
	{
		switch (State)
		{
			case ESurvivalMatchState::Initializing:
				return TEXT("INITIALIZING");
			case ESurvivalMatchState::InProgress:
				return TEXT("SURVIVE");
			case ESurvivalMatchState::Victory:
				return TEXT("VICTORY");
			case ESurvivalMatchState::Defeat:
				return TEXT("DEFEAT");
			default:
				return TEXT("UNKNOWN");
		}
	}

	bool Contains(const FVector2D& Point, float X, float Y, float Width, float Height)
	{
		return Point.X >= X && Point.X <= X + Width && Point.Y >= Y && Point.Y <= Y + Height;
	}

	FString ShortTag(const FGameplayTag Tag)
	{
		FString Result = Tag.ToString();
		int32 Separator = INDEX_NONE;
		return Result.FindLastChar(TEXT('.'), Separator) ? Result.Mid(Separator + 1) : Result;
	}

	const TCHAR* GetBuildFailureLabel(ESurvivalBuildFailure Failure)
	{
		switch (Failure)
		{
			case ESurvivalBuildFailure::None:
				return TEXT("");
			case ESurvivalBuildFailure::OutOfRange:
				return TEXT("BUILDER TOO FAR");
			case ESurvivalBuildFailure::Blocked:
				return TEXT("PLACEMENT BLOCKED");
			case ESurvivalBuildFailure::LimitReached:
				return TEXT("BUILDING LIMIT REACHED");
			case ESurvivalBuildFailure::InsufficientResources:
				return TEXT("INSUFFICIENT RESOURCES");
			case ESurvivalBuildFailure::InvalidBuilder:
				return TEXT("SELECT A LIVING WORKER");
			case ESurvivalBuildFailure::MatchEnded:
				return TEXT("MATCH ENDED");
			default:
				return TEXT("BUILD FAILED");
		}
	}
} // namespace SurvivalHUD

USurvivalHUDComponent::USurvivalHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USurvivalHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndPlacement();
	if (EndScreen)
	{
		EndScreen->RemoveFromParent();
		EndScreen = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void USurvivalHUDComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ASunrisePlayerController* Controller = GetController();
	if (!Controller || !Controller->IsLocalPlayerController())
	{
		return;
	}
	const USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
	if (Match && Match->GetSurvivalMatchState() != ESurvivalMatchState::Initializing &&
		Match->GetSurvivalMatchState() != ESurvivalMatchState::InProgress)
	{
		ShowEndScreen();
		EndPlacement();
		return;
	}
	if (ASunriseUnit* Worker = FindSelectedWorker())
	{
		LastSelectedWorker = Worker;
	}
	const bool bPrimaryDown = Controller->IsInputKeyDown(EKeys::LeftMouseButton);
	if (bPrimaryButtonWasDown && !bPrimaryDown)
	{
		HandlePrimaryClick();
	}
	bPrimaryButtonWasDown = bPrimaryDown;
	if (PlacementPreview)
	{
		UpdatePlacementPreview();
		const bool bSecondaryDown = Controller->IsInputKeyDown(EKeys::RightMouseButton);
		if (bSecondaryButtonWasDown && !bSecondaryDown)
		{
			EndPlacement();
		}
		bSecondaryButtonWasDown = bSecondaryDown;
	}
	else
	{
		bSecondaryButtonWasDown = false;
	}
}

ASunrisePlayerController* USurvivalHUDComponent::GetController() const
{
	const ASunriseHUD* HUD = Cast<ASunriseHUD>(GetOwner());
	return HUD ? Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController()) : nullptr;
}

ASunriseUnit* USurvivalHUDComponent::FindSelectedWorker() const
{
	const ASunrisePlayerController* Controller = GetController();
	const UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller);
	if (!Manager)
	{
		return nullptr;
	}
	for (AActor* Entity : Manager->GetSelectedEntities())
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Entity);
		if (IsValid(Unit) && Unit->IsAlive() &&
			(Unit->FindComponentByClass<USurvivalWorkerComponent>() || Unit->HasPawnTag(SurvivalGameplayTags::Unit_Worker)))
		{
			return Unit;
		}
	}
	return nullptr;
}

void USurvivalHUDComponent::HandlePrimaryClick()
{
	ASunrisePlayerController* Controller = GetController();
	if (!Controller)
	{
		return;
	}
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	if (!Controller->GetMousePosition(MouseX, MouseY))
	{
		return;
	}
	Controller->GetViewportSize(ViewportX, ViewportY);
	if (HandleCommandPanelClick(FVector2D(MouseX, MouseY), static_cast<float>(ViewportY)))
	{
		return;
	}

	FHitResult Hit;
	if (!Controller->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
	{
		return;
	}
	if (PendingBuildingId.IsValid() && PendingBuilder.IsValid())
	{
		APlayerState* PlayerState = Controller->PlayerState;
		USurvivalBuildComponent* Build = PlayerState ? PlayerState->FindComponentByClass<USurvivalBuildComponent>() : nullptr;
		if (Build)
		{
			const FVector Location =
				PlacementPreview ? PlacementPreview->GetActorLocation() : Hit.ImpactPoint + FVector(0.0f, 0.0f, PendingPlacementExtent.Z);
			Build->ServerRequestBuild(PendingBuildingId, FTransform(FRotator::ZeroRotator, Location), PendingBuilder.Get());
		}
		EndPlacement();
		return;
	}

	SelectBuilding(Cast<ASurvivalBuilding>(Hit.GetActor()));
}

bool USurvivalHUDComponent::HandleCommandPanelClick(const FVector2D& MousePosition, float ViewportHeight)
{
	ASunrisePlayerController* Controller = GetController();
	APlayerState* PlayerState = Controller ? Controller->PlayerState : nullptr;
	USurvivalBuildComponent* Build = PlayerState ? PlayerState->FindComponentByClass<USurvivalBuildComponent>() : nullptr;
	const float BuildY = ViewportHeight - SurvivalHUD::PanelBottomMargin - 4.0f * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
	if (Build)
	{
		const TArray<FSurvivalBuildOption>& Options = Build->GetBuildOptions();
		for (int32 Index = 0; Index < FMath::Min(4, Options.Num()); ++Index)
		{
			const float Y = BuildY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
			if (SurvivalHUD::Contains(MousePosition, SurvivalHUD::PanelX, Y, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight))
			{
				ASunriseUnit* Worker = FindSelectedWorker();
				if (!Worker && LastSelectedWorker.IsValid() && LastSelectedWorker->IsAlive())
				{
					Worker = LastSelectedWorker.Get();
				}
				if (Worker)
				{
					BeginPlacement(Options[Index], Worker);
				}
				return true;
			}
		}
	}

	ASurvivalBuilding* Building = SelectedBuilding.Get();
	USurvivalProductionComponent* Production = Building ? Building->FindComponentByClass<USurvivalProductionComponent>() : nullptr;
	if (!Production || Building->GetOwner() != PlayerState)
	{
		return false;
	}
	const TArray<FSurvivalProductionOption>& Options = Production->GetProductionOptions();
	const float ProductionX = SurvivalHUD::PanelX + SurvivalHUD::ButtonWidth + 16.0f;
	for (int32 Index = 0; Index < FMath::Min(4, Options.Num()); ++Index)
	{
		const float Y = BuildY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
		if (SurvivalHUD::Contains(MousePosition, ProductionX, Y, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight))
		{
			Production->ServerQueueUnit(Options[Index].UnitId);
			return true;
		}
	}
	return false;
}

void USurvivalHUDComponent::BeginPlacement(const FSurvivalBuildOption& Option, ASunriseUnit* Builder)
{
	EndPlacement();
	if (!IsValid(Builder) || !Option.BuildingId.IsValid() || !GetWorld())
	{
		return;
	}
	PendingBuilder = Builder;
	PendingBuildingId = Option.BuildingId;
	PendingPlacementExtent = Option.PlacementExtent;
	FActorSpawnParameters Parameters;
	Parameters.Owner = GetController();
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PlacementPreview = GetWorld()->SpawnActor<ASurvivalBuildingPlacementPreview>(
		ASurvivalBuildingPlacementPreview::StaticClass(), FTransform::Identity, Parameters);
	if (PlacementPreview)
	{
		PlacementPreview->Configure(Option);
		UpdatePlacementPreview();
	}
}

void USurvivalHUDComponent::UpdatePlacementPreview()
{
	ASunrisePlayerController* Controller = GetController();
	if (!Controller || !PlacementPreview)
	{
		return;
	}
	FHitResult Hit;
	if (Controller->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
	{
		PlacementPreview->SetActorLocation(Hit.ImpactPoint + FVector(0.0f, 0.0f, PendingPlacementExtent.Z));
	}
}

void USurvivalHUDComponent::EndPlacement()
{
	if (PlacementPreview)
	{
		PlacementPreview->Destroy();
		PlacementPreview = nullptr;
	}
	PendingBuilder.Reset();
	PendingBuildingId = FGameplayTag();
	PendingPlacementExtent = FVector::ZeroVector;
}

void USurvivalHUDComponent::ShowEndScreen()
{
	if (EndScreen)
	{
		return;
	}
	ASunrisePlayerController* Controller = GetController();
	const USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
	if (!Controller || !Match)
	{
		return;
	}
	const ESurvivalMatchState State = Match->GetSurvivalMatchState();
	if (State != ESurvivalMatchState::Victory && State != ESurvivalMatchState::Defeat)
	{
		return;
	}
	Controller->SetCommandsEnabled(false);
	EndScreen = CreateWidget<USunriseEndScreenWidget>(Controller, USunriseEndScreenWidget::StaticClass());
	if (EndScreen)
	{
		EndScreen->SetResult(State == ESurvivalMatchState::Victory ? ESunriseMatchResult::Victory : ESunriseMatchResult::Defeat);
		EndScreen->AddToViewport(100);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, EndScreen, EMouseLockMode::DoNotLock);
		Controller->bShowMouseCursor = true;
	}
}

void USurvivalHUDComponent::SelectBuilding(ASurvivalBuilding* Building)
{
	if (SelectedBuilding.Get() == Building)
	{
		return;
	}
	if (SelectedBuilding.IsValid())
	{
		SelectedBuilding->SetLocallySelected(false);
	}
	SelectedBuilding = Building;
	if (SelectedBuilding.IsValid())
	{
		SelectedBuilding->SetLocallySelected(true);
	}
}

void USurvivalHUDComponent::DrawHUD(ASunriseHUD* HUD)
{
	if (!HUD || !HUD->GetDrawingCanvas())
	{
		return;
	}
	const USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
	const ASunrisePlayerController* Controller = Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController());
	const APlayerState* PlayerState = Controller ? Controller->PlayerState : nullptr;
	const USurvivalEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	if (!Match)
	{
		return;
	}

	const float X = 345.0f;
	const float Y = 18.0f;
	HUD->DrawRect(FLinearColor(0.015f, 0.025f, 0.018f, 0.92f), X, Y, 600.0f, 108.0f);
	HUD->DrawText(FString::Printf(TEXT("SURVIVAL // %s"), SurvivalHUD::GetStateLabel(Match->GetSurvivalMatchState())),
		FLinearColor(0.35f, 0.95f, 0.45f), X + 14.0f, Y + 8.0f, GEngine->GetMediumFont(), 1.0f);
	if (Economy)
	{
		const FSurvivalResourceAmounts Resources = Economy->GetResources();
		HUD->DrawText(FString::Printf(TEXT("FOOD %.0f   WOOD %.0f   STONE %.0f   METAL %.0f"), Resources.Food, Resources.Wood,
						  Resources.Stone, Resources.Metal),
			FLinearColor::White, X + 14.0f, Y + 36.0f, GEngine->GetSmallFont(), 0.92f);
		HUD->DrawText(FString::Printf(TEXT("POPULATION %d / %d"), Economy->GetPopulation(), Economy->GetPopulationCap()),
			FLinearColor(0.85f, 0.9f, 1.0f), X + 14.0f, Y + 59.0f, GEngine->GetSmallFont(), 0.88f);
	}
	const float WaveSeconds = Match->GetSecondsUntilNextWave();
	const FString WaveText = WaveSeconds >= 0.0f ? FString::Printf(TEXT("NEXT %.0fs"), WaveSeconds) : TEXT("ACTIVE");
	HUD->DrawText(
		FString::Printf(TEXT("BASES %d   WORKERS %d   ENEMIES %d   WAVE %d/%d %s"), Match->GetAliveMainBaseCount(),
			Match->GetAliveWorkerCount(), Match->GetAliveWaveEnemyCount(), Match->GetCurrentWave(), Match->GetTotalWaves(), *WaveText),
		FLinearColor(0.95f, 0.78f, 0.22f), X + 210.0f, Y + 59.0f, GEngine->GetSmallFont(), 0.88f);

	const float BuildY =
		HUD->GetDrawingCanvas()->ClipY - SurvivalHUD::PanelBottomMargin - 4.0f * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
	const USurvivalBuildComponent* Build = PlayerState ? PlayerState->FindComponentByClass<USurvivalBuildComponent>() : nullptr;
	if (Build)
	{
		const TArray<FSurvivalBuildOption>& Options = Build->GetBuildOptions();
		for (int32 Index = 0; Index < FMath::Min(4, Options.Num()); ++Index)
		{
			const float ButtonY = BuildY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
			HUD->DrawRect(FLinearColor(0.04f, 0.08f, 0.05f, 0.94f), SurvivalHUD::PanelX, ButtonY, SurvivalHUD::ButtonWidth,
				SurvivalHUD::ButtonHeight);
			HUD->DrawText(FString::Printf(TEXT("BUILD %s"), *SurvivalHUD::ShortTag(Options[Index].BuildingId)), FLinearColor::White,
				SurvivalHUD::PanelX + 8.0f, ButtonY + 5.0f, GEngine->GetSmallFont(), 0.82f);
		}
	}
	if (Build && Build->GetLastBuildFailure() != ESurvivalBuildFailure::None)
	{
		HUD->DrawText(SurvivalHUD::GetBuildFailureLabel(Build->GetLastBuildFailure()), FLinearColor(1.0f, 0.2f, 0.1f), SurvivalHUD::PanelX,
			BuildY - 50.0f, GEngine->GetSmallFont(), 0.9f);
	}
	if (PendingBuildingId.IsValid())
	{
		HUD->DrawText(FString::Printf(TEXT("PLACE %s: CLICK ON TERRAIN"), *SurvivalHUD::ShortTag(PendingBuildingId)),
			FLinearColor(1.0f, 0.8f, 0.15f), SurvivalHUD::PanelX, BuildY - 26.0f, GEngine->GetSmallFont(), 0.9f);
	}

	ASurvivalBuilding* Building = SelectedBuilding.Get();
	if (Building)
	{
		const float PanelX = SurvivalHUD::PanelX + SurvivalHUD::ButtonWidth + 16.0f;
		HUD->DrawText(FString::Printf(TEXT("BUILDING  HP %.0f / %.0f"), Building->GetHealth(), Building->GetMaxHealth()),
			FLinearColor(0.45f, 0.9f, 1.0f), PanelX, BuildY - 26.0f, GEngine->GetSmallFont(), 0.9f);
		if (const USurvivalProductionComponent* Production = Building->FindComponentByClass<USurvivalProductionComponent>())
		{
			const TArray<FSurvivalProductionOption>& Options = Production->GetProductionOptions();
			for (int32 Index = 0; Index < FMath::Min(4, Options.Num()); ++Index)
			{
				const float ButtonY = BuildY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
				HUD->DrawRect(
					FLinearColor(0.07f, 0.06f, 0.12f, 0.94f), PanelX, ButtonY, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight);
				HUD->DrawText(FString::Printf(TEXT("TRAIN %s"), *SurvivalHUD::ShortTag(Options[Index].UnitId)), FLinearColor::White,
					PanelX + 8.0f, ButtonY + 5.0f, GEngine->GetSmallFont(), 0.82f);
			}
		}
	}
}
