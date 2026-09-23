#include "GameModes/Survival/UI/SurvivalHUDComponent.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalHUDComponent)

namespace SurvivalHUD
{
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
} // namespace SurvivalHUD

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
	const float Width = 600.0f;
	const float Height = 108.0f;
	HUD->DrawRect(FLinearColor(0.015f, 0.025f, 0.018f, 0.92f), X, Y, Width, Height);
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
}
