// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/UI/OverloadHUDComponent.h"

#include "Abilities/SunriseHeroAbilities.h"
#include "Abilities/SunriseHeroSquadAbility.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"
#include "GameModes/Overload/Actors/OverloadEnergyCore.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadCaptureComponent.h"
#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"
#include "GameModes/Overload/OverloadGameMatchComponent.h"
#include "GameModes/Overload/Types/OverloadTeamIds.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

namespace OverloadHUD
{
	float GetCanvasScale(ASunriseHUD* HUD)
	{
		const UCanvas* Canvas = HUD ? HUD->GetDrawingCanvas() : nullptr;
		return Canvas ? FMath::Clamp(Canvas->ClipX / 1600.0f, 1.15f, 1.6f) : 1.15f;
	}

	FString GetCoreStateLabel(EOverloadCoreState State)
	{
		switch (State)
		{
			case EOverloadCoreState::Stable:
				return TEXT("STABLE");
			case EOverloadCoreState::Overloading:
				return TEXT("OVERLOADING");
			case EOverloadCoreState::Cooling:
				return TEXT("COOLING");
			case EOverloadCoreState::Destroyed:
				return TEXT("DESTROYED");
			default:
				return TEXT("UNKNOWN");
		}
	}

	FString GetMatchStateLabel(ESunriseMatchResult Result)
	{
		switch (Result)
		{
			case ESunriseMatchResult::Victory:
				return TEXT("VICTORY");
			case ESunriseMatchResult::Defeat:
				return TEXT("DEFEAT");
			default:
				return TEXT("IN PROGRESS");
		}
	}

	float SafeFraction(float Value, float Maximum)
	{
		return Maximum > KINDA_SMALL_NUMBER ? FMath::Clamp(Value / Maximum, 0.0f, 1.0f) : 0.0f;
	}
} // namespace OverloadHUD

void UOverloadHUDComponent::DrawHUD(ASunriseHUD* HUD)
{
	if (!HUD || !HUD->GetDrawingCanvas())
	{
		return;
	}

	const UOverloadGameMatchComponent* MatchComponent = UOverloadGameMatchComponent::Find(this);
	if (!MatchComponent)
	{
		return;
	}

	DrawCoreOverview(HUD, MatchComponent);
	DrawLaneOverview(HUD, MatchComponent);
	DrawObjectiveWorldOverlays(HUD, MatchComponent);

	if (ASunrisePlayerController* Controller = Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController()))
	{
		const USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(Controller);
		const ASunriseUnit* Hero = UnitManager ? UnitManager->GetHeroForPlayer(Controller) : nullptr;
		const bool bHeroAvailable = IsValid(Hero) && Hero->IsAlive() && Hero->GetAbilitySystemComponent();
		struct FAbilityPanelEntry
		{
			const TCHAR* Label;
			float Cooldown;
		};
		const FAbilityPanelEntry Abilities[] = {{TEXT("SQUAD"), USunriseHeroSquadAbility::GetCooldownRemaining(Hero)},
			{TEXT("HEALING AURA"), GetDefault<USunriseHeroHealingAuraAbility>()->GetCooldownRemaining(Hero)},
			{TEXT("BOMB"), GetDefault<USunriseHeroBombAbility>()->GetCooldownRemaining(Hero)},
			{TEXT("BLACK HOLE"), GetDefault<USunriseHeroBlackHoleAbility>()->GetCooldownRemaining(Hero)}};
		const int32 AbilityCount = UE_ARRAY_COUNT(Abilities);
		const float CanvasWidth = HUD->GetDrawingCanvas()->ClipX;
		const float Scale = FMath::Min(OverloadHUD::GetCanvasScale(HUD), CanvasWidth / (AbilityCount * 208.0f + 28.0f));
		const float Width = 200.0f * Scale;
		const float Gap = 8.0f * Scale;
		const float Height = 62.0f * Scale;
		const float StartX = (CanvasWidth - AbilityCount * Width - (AbilityCount - 1) * Gap) * 0.5f;
		const float Y = HUD->GetDrawingCanvas()->ClipY - Height - 18.0f * Scale;
		for (int32 Index = 0; Index < AbilityCount; ++Index)
		{
			const FAbilityPanelEntry& Ability = Abilities[Index];
			const float X = StartX + Index * (Width + Gap);
			FString State = TEXT("UNAVAILABLE");
			FLinearColor StateColor(0.5f, 0.5f, 0.5f);
			if (bHeroAvailable)
			{
				State = Ability.Cooldown > 0.0f ? FString::Printf(TEXT("READY IN %ds"), FMath::CeilToInt(Ability.Cooldown)) : TEXT("READY");
				StateColor = Ability.Cooldown > 0.0f ? FLinearColor(0.95f, 0.65f, 0.15f) : FLinearColor(0.25f, 0.95f, 0.4f);
			}
			HUD->DrawRect(FLinearColor(0.008f, 0.014f, 0.025f, 0.92f), X, Y, Width, Height);
			HUD->DrawText(Ability.Label, FLinearColor(0.82f, 0.86f, 0.92f), X + 12.0f * Scale, Y + 7.0f * Scale, GEngine->GetSmallFont(),
				0.9f * Scale);
			HUD->DrawText(State, StateColor, X + 12.0f * Scale, Y + 30.0f * Scale, GEngine->GetMediumFont(), 0.85f * Scale);
		}
	}
}
void UOverloadHUDComponent::DrawCoreOverview(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode)
{
	const TArray<AOverloadEnergyCore*>& Cores = GameMode->GetCores();
	const float PanelX = 345.0f;
	const float PanelWidth = 470.0f * OverloadHUD::GetCanvasScale(HUD);
	const float RowHeight = 92.0f;
	const float PanelHeight = 34.0f + FMath::Max(1, Cores.Num()) * RowHeight;
	HUD->DrawRect(FLinearColor(0.01f, 0.018f, 0.03f, 0.88f), PanelX, 18.0f, PanelWidth, PanelHeight);
	HUD->DrawText(FString::Printf(TEXT("OVERLOAD // %s // CORE STATUS"), *OverloadHUD::GetMatchStateLabel(GameMode->GetMatchResult())),
		FLinearColor(0.95f, 0.75f, 0.16f), PanelX + 14.0f, 26.0f, GEngine->GetMediumFont(), 0.9f * OverloadHUD::GetCanvasScale(HUD));

	if (!GameMode->IsOverloadInitialized())
	{
		HUD->DrawText(TEXT("Initializing lanes and objectives..."), FLinearColor::White, PanelX + 14.0f, 61.0f, GEngine->GetSmallFont(),
			0.9f * OverloadHUD::GetCanvasScale(HUD));
		return;
	}

	for (int32 Index = 0; Index < Cores.Num(); ++Index)
	{
		const AOverloadEnergyCore* Core = Cores[Index];
		if (!IsValid(Core))
		{
			continue;
		}
		const UOverloadIntegritySet* Attributes = Core->GetIntegrityAttributes();
		if (!Attributes)
		{
			continue;
		}

		const float Y = 52.0f + Index * RowHeight;
		const FLinearColor TeamColor = HUD->GetTeamColor(Core->GetTeamId());
		const float HeroRespawn = GameMode->GetHeroRespawnSeconds(Core->GetTeamId());
		FString HeroState = TEXT("HERO NONE");
		if (GameMode->GetLivingHeroForTeam(Core->GetTeamId()))
		{
			HeroState = TEXT("HERO READY");
		}
		else if (HeroRespawn >= 0.0f)
		{
			HeroState = FString::Printf(TEXT("HERO %.0fs"), HeroRespawn);
		}
		HUD->DrawText(FString::Printf(TEXT("%s  UNITS %d  %s  %s  ATTACKER %s"), *GetTeamLabel(Core->GetTeamId()),
						  GameMode->GetAliveUnitCountForTeam(Core->GetTeamId()), *HeroState,
						  *OverloadHUD::GetCoreStateLabel(Core->GetCoreState()), *GetTeamLabel(Core->GetOverloadingTeamId())),
			TeamColor, PanelX + 14.0f, Y, GEngine->GetSmallFont(), 0.88f * OverloadHUD::GetCanvasScale(HUD));

		const float BarX = PanelX + 14.0f;
		const float BarWidth = PanelWidth - 28.0f;
		DrawProgressBar(HUD, BarX, Y + 34.0f, BarWidth, 9.0f,
			OverloadHUD::SafeFraction(Attributes->GetIntegrity(), Attributes->GetMaxIntegrity()), TeamColor);
		HUD->DrawText(FString::Printf(TEXT("INTEGRITY %.0f / %.0f"), Attributes->GetIntegrity(), Attributes->GetMaxIntegrity()),
			FLinearColor::White, BarX + 4.0f, Y + 16.0f, GEngine->GetSmallFont(), 0.65f * OverloadHUD::GetCanvasScale(HUD));
		DrawProgressBar(HUD, BarX, Y + 50.0f, BarWidth, 22.0f, Core->GetOverloadPercent(), FLinearColor(1.0f, 0.25f, 0.04f, 1.0f));
		HUD->DrawText(FString::Printf(TEXT("OVERLOAD %.0f%%"), Core->GetOverloadPercent() * 100.0f), FLinearColor::White, BarX + 4.0f,
			Y + 52.0f, GEngine->GetSmallFont(), 0.65f * OverloadHUD::GetCanvasScale(HUD));
	}
}

void UOverloadHUDComponent::DrawLaneOverview(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode)
{
	const float PanelWidth = 470.0f * OverloadHUD::GetCanvasScale(HUD);
	const float PanelX = HUD->GetDrawingCanvas()->ClipX - PanelWidth - 18.0f;
	const float PanelY = 18.0f;
	float PanelHeight = 54.0f;
	for (const AOverloadLaneSpline* Lane : GameMode->GetLanes())
	{
		if (IsValid(Lane))
		{
			PanelHeight += 37.0f + Lane->GetSpawnedTowers().Num() * 17.0f;
		}
	}
	HUD->DrawRect(FLinearColor(0.008f, 0.014f, 0.025f, 0.9f), PanelX, PanelY, PanelWidth, PanelHeight);
	HUD->DrawText(FString::Printf(TEXT("POWER LANES  %d"), GameMode->GetLanes().Num()), FLinearColor(0.95f, 0.75f, 0.16f), PanelX + 14.0f,
		PanelY + 10.0f, GEngine->GetMediumFont(), 0.9f * OverloadHUD::GetCanvasScale(HUD));

	float Y = PanelY + 42.0f;
	for (int32 LaneIndex = 0; LaneIndex < GameMode->GetLanes().Num(); ++LaneIndex)
	{
		const AOverloadLaneSpline* Lane = GameMode->GetLanes()[LaneIndex];
		if (!IsValid(Lane))
		{
			continue;
		}
		const UOverloadWaveSpawnerComponent* WaveSpawner = Lane->GetWaveSpawner();
		const float WaveSeconds = WaveSpawner ? WaveSpawner->GetSecondsUntilNextWave() : -1.0f;
		HUD->DrawRect(FLinearColor(0.035f, 0.05f, 0.075f, 0.92f), PanelX + 8.0f, Y - 4.0f, PanelWidth - 16.0f, 25.0f);
		HUD->DrawText(FString::Printf(TEXT("LANE %d  %s %d -> %s %d | WAVE %s | %d POINTS"), LaneIndex + 1,
						  *GetTeamLabel(Lane->GetSourceTeamId()), WaveSpawner ? WaveSpawner->GetAliveUnitCount(Lane->GetSourceTeamId()) : 0,
						  *GetTeamLabel(Lane->GetTargetTeamId()), WaveSpawner ? WaveSpawner->GetAliveUnitCount(Lane->GetTargetTeamId()) : 0,
						  WaveSeconds >= 0.0f ? *FString::Printf(TEXT("%.1fs"), WaveSeconds) : TEXT("OFF"), Lane->GetSpawnedTowers().Num()),
			FLinearColor::White, PanelX + 14.0f, Y, GEngine->GetSmallFont(), 0.78f * OverloadHUD::GetCanvasScale(HUD));
		Y += 27.0f;

		for (int32 TowerIndex = 0; TowerIndex < Lane->GetSpawnedTowers().Num(); ++TowerIndex)
		{
			const AOverloadGuardTower* Tower = Lane->GetSpawnedTowers()[TowerIndex];
			if (!IsValid(Tower))
			{
				continue;
			}
			const UOverloadIntegritySet* Attributes = Tower->GetIntegrityAttributes();
			const UOverloadDefenseSet* Defense = Tower->GetDefenseAttributes();
			const UOverloadHackSet* Hack = Tower->GetHackAttributes();
			const UOverloadCaptureComponent* Capture = Tower->GetCaptureComponent();
			if (!Attributes || !Defense || !Hack || !Capture)
			{
				continue;
			}
			const float IntegrityPercent = OverloadHUD::SafeFraction(Attributes->GetIntegrity(), Attributes->GetMaxIntegrity());
			HUD->DrawText(
				FString::Printf(TEXT("#%02d %s/O%s L%d HP %.0f%% ATK %.0f ARM %.0f RES %.2f HACK %s x%d %.0f%%%s"), TowerIndex + 1,
					*GetTeamLabel(Tower->GetTeamId()), *GetTeamLabel(Tower->GetOriginalTeamId()), Tower->GetTierIndex(),
					IntegrityPercent * 100.0f, Defense->GetAttackPower(), Defense->GetArmor(), Hack->GetHackResistance(),
					*GetTeamLabel(Capture->GetActiveHackingTeamId()), Capture->GetActiveCapturingUnitCount(),
					Capture->GetCaptureProgress() * 100.0f, Capture->IsCaptureContested() ? TEXT(" CONTESTED") : TEXT("")),
				HUD->GetTeamColor(Tower->GetTeamId()), PanelX + 17.0f, Y, GEngine->GetSmallFont(),
				0.68f * OverloadHUD::GetCanvasScale(HUD));
			Y += 17.0f;
		}
		Y += 10.0f;
	}
}

void UOverloadHUDComponent::DrawObjectiveWorldOverlays(ASunriseHUD* HUD, const UOverloadGameMatchComponent* GameMode)
{
	for (const AOverloadEnergyCore* Core : GameMode->GetCores())
	{
		DrawCoreWorldOverlay(HUD, Core);
	}
	for (const AOverloadGuardTower* Tower : GameMode->GetTowers())
	{
		DrawTowerWorldOverlay(HUD, Tower);
	}
}

void UOverloadHUDComponent::DrawCoreWorldOverlay(ASunriseHUD* HUD, const AOverloadEnergyCore* Core)
{
	const ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController());
	if (!PC || !IsValid(Core) || !Core->WasRecentlyRendered(0.25f))
	{
		return;
	}
	const UOverloadIntegritySet* Attributes = Core->GetIntegrityAttributes();
	if (!Attributes)
	{
		return;
	}
	FVector2D Screen;
	if (!PC->ProjectWorldLocationToScreen(Core->GetActorLocation() + FVector(0.0f, 0.0f, 360.0f), Screen, true))
	{
		return;
	}

	const float X = Screen.X - 110.0f;
	const float Y = Screen.Y;
	HUD->DrawRect(FLinearColor(0.005f, 0.01f, 0.02f, 0.88f), X, Y, 220.0f, 50.0f);
	HUD->DrawText(
		FString::Printf(TEXT("CORE %s  %s"), *GetTeamLabel(Core->GetTeamId()), *OverloadHUD::GetCoreStateLabel(Core->GetCoreState())),
		HUD->GetTeamColor(Core->GetTeamId()), X + 6.0f, Y + 3.0f, GEngine->GetSmallFont(), 0.75f * OverloadHUD::GetCanvasScale(HUD));
	DrawProgressBar(HUD, X + 6.0f, Y + 20.0f, 208.0f, 8.0f,
		OverloadHUD::SafeFraction(Attributes->GetIntegrity(), Attributes->GetMaxIntegrity()), HUD->GetTeamColor(Core->GetTeamId()));
	DrawProgressBar(HUD, X + 6.0f, Y + 34.0f, 208.0f, 8.0f, Core->GetOverloadPercent(), FLinearColor(1.0f, 0.2f, 0.02f, 1.0f));
}

void UOverloadHUDComponent::DrawTowerWorldOverlay(ASunriseHUD* HUD, const AOverloadGuardTower* Tower)
{
	const ASunrisePlayerController* PC = Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController());
	if (!PC || !IsValid(Tower) || !Tower->WasRecentlyRendered(0.25f))
	{
		return;
	}
	const UOverloadIntegritySet* Attributes = Tower->GetIntegrityAttributes();
	const UOverloadCaptureComponent* Capture = Tower->GetCaptureComponent();
	if (!Attributes || !Capture)
	{
		return;
	}
	FVector2D Screen;
	if (!PC->ProjectWorldLocationToScreen(Tower->GetActorLocation() + FVector(0.0f, 0.0f, 350.0f), Screen, true))
	{
		return;
	}

	const float X = Screen.X - 85.0f;
	const float Y = Screen.Y;
	HUD->DrawRect(FLinearColor(0.005f, 0.01f, 0.02f, 0.86f), X, Y, 170.0f, 43.0f);
	HUD->DrawText(FString::Printf(TEXT("TOWER %s  L%d"), *GetTeamLabel(Tower->GetTeamId()), Tower->GetTierIndex()),
		HUD->GetTeamColor(Tower->GetTeamId()), X + 5.0f, Y + 2.0f, GEngine->GetSmallFont(), 0.7f * OverloadHUD::GetCanvasScale(HUD));
	DrawProgressBar(HUD, X + 5.0f, Y + 17.0f, 160.0f, 7.0f,
		OverloadHUD::SafeFraction(Attributes->GetIntegrity(), Attributes->GetMaxIntegrity()), HUD->GetTeamColor(Tower->GetTeamId()));
	DrawProgressBar(
		HUD, X + 5.0f, Y + 29.0f, 160.0f, 7.0f, Capture->GetCaptureProgress(), HUD->GetTeamColor(Capture->GetActiveHackingTeamId()));
}

void UOverloadHUDComponent::DrawProgressBar(
	ASunriseHUD* HUD, float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor)
{
	HUD->DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f), X, Y, Width, Height);
	HUD->DrawRect(FillColor, X + 1.0f, Y + 1.0f, (Width - 2.0f) * FMath::Clamp(Fraction, 0.0f, 1.0f), Height - 2.0f);
}

FString UOverloadHUDComponent::GetTeamLabel(int32 TeamId)
{
	return TeamId == INDEX_NONE ? TEXT("-") : TeamId == OverloadTeamIds::Neutral ? TEXT("N") : FString::Printf(TEXT("T%d"), TeamId);
}
