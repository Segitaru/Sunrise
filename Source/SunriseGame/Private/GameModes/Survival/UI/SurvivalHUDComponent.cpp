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
	constexpr float PanelBottomMargin = 20.0f;
	constexpr float ButtonWidth = 190.0f;
	constexpr float ButtonHeight = 28.0f;
	constexpr float ButtonGap = 4.0f;
	constexpr float PanelGap = 16.0f;

	float GetPanelStartX(float ViewportWidth, bool bShowBuildPanel, bool bShowProductionPanel)
	{
		const int32 PanelCount = static_cast<int32>(bShowBuildPanel) + static_cast<int32>(bShowProductionPanel);
		const float TotalWidth = PanelCount * ButtonWidth + FMath::Max(0, PanelCount - 1) * PanelGap;
		return FMath::Max(0.0f, (ViewportWidth - TotalWidth) * 0.5f);
	}

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
	if (PlacementPreview)
	{
		UpdatePlacementPreview();
		if (Controller->WasInputKeyJustReleased(EKeys::RightMouseButton))
		{
			EndPlacement();
		}
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

bool USurvivalHUDComponent::HandlePrimaryClick()
{
	if (LastPrimaryClickFrame == GFrameCounter)
	{
		return bLastPrimaryClickHandled;
	}
	LastPrimaryClickFrame = GFrameCounter;
	bLastPrimaryClickHandled = false;

	ASunrisePlayerController* Controller = GetController();
	if (!Controller)
	{
		return false;
	}
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	if (!Controller->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}
	Controller->GetViewportSize(ViewportX, ViewportY);
	if (HandleCommandPanelClick(FVector2D(MouseX, MouseY), FVector2D(ViewportX, ViewportY)))
	{
		bLastPrimaryClickHandled = true;
		return true;
	}

	FHitResult Hit;
	if (!Controller->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
	{
		return false;
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
		bLastPrimaryClickHandled = true;
		return true;
	}

	ASurvivalBuilding* ClickedBuilding = Cast<ASurvivalBuilding>(Hit.GetActor());
	SelectBuilding(ClickedBuilding);
	bLastPrimaryClickHandled = ClickedBuilding != nullptr;
	return bLastPrimaryClickHandled;
}

bool USurvivalHUDComponent::HandleCommandPanelClick(const FVector2D& MousePosition, const FVector2D& ViewportSize)
{
	ASunrisePlayerController* Controller = GetController();
	APlayerState* PlayerState = Controller ? Controller->PlayerState : nullptr;
	USurvivalBuildComponent* Build = PlayerState ? PlayerState->FindComponentByClass<USurvivalBuildComponent>() : nullptr;
	ASunriseUnit* Worker = FindSelectedWorker();
	ASurvivalBuilding* Building = SelectedBuilding.Get();
	USurvivalProductionComponent* Production = Building ? Building->FindComponentByClass<USurvivalProductionComponent>() : nullptr;
	if (Production && (Building->GetOwner() != PlayerState || !Building->IsConstructionComplete()))
	{
		Production = nullptr;
	}

	const int32 BuildRows = Worker && Build ? Build->GetBuildOptions().Num() : 0;
	const int32 ProductionRows = Production ? Production->GetProductionOptions().Num() : 0;
	const bool bShowBuildPanel = BuildRows > 0;
	const bool bShowProductionPanel = ProductionRows > 0;
	const int32 RowCount = FMath::Max(1, FMath::Max(BuildRows, ProductionRows));
	const float PanelY = ViewportSize.Y - SurvivalHUD::PanelBottomMargin - RowCount * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
	const float BuildX = SurvivalHUD::GetPanelStartX(ViewportSize.X, bShowBuildPanel, bShowProductionPanel);
	const float ProductionX = BuildX + (bShowBuildPanel ? SurvivalHUD::ButtonWidth + SurvivalHUD::PanelGap : 0.0f);

	if (Worker && Build)
	{
		const TArray<FSurvivalBuildOption>& Options = Build->GetBuildOptions();
		for (int32 Index = 0; Index < Options.Num(); ++Index)
		{
			const float Y = PanelY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
			if (SurvivalHUD::Contains(MousePosition, BuildX, Y, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight))
			{
				BeginPlacement(Options[Index], Worker);
				return true;
			}
		}
	}

	if (!Production)
	{
		return false;
	}
	const TArray<FSurvivalProductionOption>& Options = Production->GetProductionOptions();
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const float Y = PanelY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
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

	ASunriseUnit* Worker = FindSelectedWorker();
	const USurvivalBuildComponent* Build = PlayerState ? PlayerState->FindComponentByClass<USurvivalBuildComponent>() : nullptr;
	ASurvivalBuilding* Building = SelectedBuilding.Get();
	const USurvivalProductionComponent* Production = Building ? Building->FindComponentByClass<USurvivalProductionComponent>() : nullptr;
	if (Production && (Building->GetOwner() != PlayerState || !Building->IsConstructionComplete()))
	{
		Production = nullptr;
	}
	const int32 BuildRows = Worker && Build ? Build->GetBuildOptions().Num() : 0;
	const int32 ProductionRows = Production ? Production->GetProductionOptions().Num() : 0;
	const bool bShowBuildPanel = BuildRows > 0;
	const bool bShowProductionPanel = ProductionRows > 0;
	const int32 RowCount = FMath::Max(1, FMath::Max(BuildRows, ProductionRows));
	const float PanelY =
		HUD->GetDrawingCanvas()->ClipY - SurvivalHUD::PanelBottomMargin - RowCount * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
	const float BuildX = SurvivalHUD::GetPanelStartX(HUD->GetDrawingCanvas()->ClipX, bShowBuildPanel, bShowProductionPanel);
	const float ProductionX = BuildX + (bShowBuildPanel ? SurvivalHUD::ButtonWidth + SurvivalHUD::PanelGap : 0.0f);

	if (Worker && Build)
	{
		const TArray<FSurvivalBuildOption>& Options = Build->GetBuildOptions();
		for (int32 Index = 0; Index < Options.Num(); ++Index)
		{
			const float ButtonY = PanelY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
			HUD->DrawRect(FLinearColor(0.04f, 0.08f, 0.05f, 0.94f), BuildX, ButtonY, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight);
			HUD->DrawText(FString::Printf(TEXT("BUILD %s"), *SurvivalHUD::ShortTag(Options[Index].BuildingId)), FLinearColor::White,
				BuildX + 8.0f, ButtonY + 5.0f, GEngine->GetSmallFont(), 0.82f);
		}
	}
	if (Build && (Worker || PendingBuildingId.IsValid()) && Build->GetLastBuildFailure() != ESurvivalBuildFailure::None)
	{
		HUD->DrawText(SurvivalHUD::GetBuildFailureLabel(Build->GetLastBuildFailure()), FLinearColor(1.0f, 0.2f, 0.1f), BuildX,
			PanelY - 50.0f, GEngine->GetSmallFont(), 0.9f);
	}
	if (PendingBuildingId.IsValid())
	{
		HUD->DrawText(FString::Printf(TEXT("PLACE %s: CLICK ON TERRAIN"), *SurvivalHUD::ShortTag(PendingBuildingId)),
			FLinearColor(1.0f, 0.8f, 0.15f), BuildX, PanelY - 26.0f, GEngine->GetSmallFont(), 0.9f);
	}

	if (Building)
	{
		const FString BuildingStatus =
			Building->IsConstructionComplete()
				? FString::Printf(TEXT("BUILDING  HP %.0f / %.0f"), Building->GetHealth(), Building->GetMaxHealth())
				: FString::Printf(TEXT("CONSTRUCTION %.0f%%  HP %.0f / %.0f"), Building->GetSurvivalConstructionProgress() * 100.0f,
					  Building->GetHealth(), Building->GetMaxHealth());
		HUD->DrawText(BuildingStatus, FLinearColor(0.45f, 0.9f, 1.0f), ProductionX, PanelY - 26.0f, GEngine->GetSmallFont(), 0.9f);
		if (Production)
		{
			const TArray<FSurvivalProductionOption>& Options = Production->GetProductionOptions();
			for (int32 Index = 0; Index < Options.Num(); ++Index)
			{
				const float ButtonY = PanelY + Index * (SurvivalHUD::ButtonHeight + SurvivalHUD::ButtonGap);
				HUD->DrawRect(
					FLinearColor(0.07f, 0.06f, 0.12f, 0.94f), ProductionX, ButtonY, SurvivalHUD::ButtonWidth, SurvivalHUD::ButtonHeight);
				HUD->DrawText(FString::Printf(TEXT("TRAIN %s"), *SurvivalHUD::ShortTag(Options[Index].UnitId)), FLinearColor::White,
					ProductionX + 8.0f, ButtonY + 5.0f, GEngine->GetSmallFont(), 0.82f);
			}
		}
	}
}
