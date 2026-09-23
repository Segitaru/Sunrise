// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Components/SunriseHeroAbilityHUDComponent.h"

#include "Abilities/SunriseHeroAbilities.h"
#include "Abilities/SunriseHeroSquadAbility.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"
#include "Units/SunriseUnit.h"

namespace SunriseHeroAbilityHUD
{
	float GetCanvasScale(const ASunriseHUD* HUD)
	{
		const UCanvas* Canvas = HUD ? HUD->GetDrawingCanvas() : nullptr;
		return Canvas ? FMath::Clamp(Canvas->ClipX / 1600.0f, 1.15f, 1.6f) : 1.15f;
	}

	const ASunriseUnit* FindSelectedHero(const ASunriseHUD* HUD)
	{
		const ASunrisePlayerController* Controller = HUD ? Cast<ASunrisePlayerController>(HUD->GetOwningPlayerController()) : nullptr;
		const UControllableEntitiesManager* Manager =
			Controller ? UControllableEntitiesManager::FindControllableEntitiesManager(Controller) : nullptr;
		if (!Manager)
		{
			return nullptr;
		}

		for (AActor* Entity : Manager->GetSelectedEntities())
		{
			ASunriseUnit* Unit = Cast<ASunriseUnit>(Entity);
			if (IsValid(Unit) && Unit->IsAlive() && Unit->IsHero() && Unit->GetAbilitySystemComponent() && Manager->CanControlEntity(Unit))
			{
				return Unit;
			}
		}
		return nullptr;
	}
} // namespace SunriseHeroAbilityHUD

void USunriseHeroAbilityHUDComponent::DrawHUD(ASunriseHUD* HUD)
{
	const ASunriseUnit* Hero = SunriseHeroAbilityHUD::FindSelectedHero(HUD);
	if (!HUD || !HUD->GetDrawingCanvas() || !Hero)
	{
		return;
	}

	struct FAbilityPanelEntry
	{
		const TCHAR* Label;
		float Cooldown;
	};
	const FAbilityPanelEntry Abilities[] = {{TEXT("SQUAD"), USunriseHeroSquadAbility::GetCooldownRemaining(Hero)},
		{TEXT("HEALING AURA"), GetDefault<USunriseHeroHealingAuraAbility>()->GetCooldownRemaining(Hero)},
		{TEXT("BOMB"), GetDefault<USunriseHeroBombAbility>()->GetCooldownRemaining(Hero)},
		{TEXT("BLACK HOLE"), GetDefault<USunriseHeroBlackHoleAbility>()->GetCooldownRemaining(Hero)}};

	constexpr int32 AbilityCount = UE_ARRAY_COUNT(Abilities);
	const float CanvasWidth = HUD->GetDrawingCanvas()->ClipX;
	const float Scale =
		FMath::Min(SunriseHeroAbilityHUD::GetCanvasScale(HUD), CanvasWidth / (static_cast<float>(AbilityCount) * 208.0f + 28.0f));
	const float Width = 200.0f * Scale;
	const float Gap = 8.0f * Scale;
	const float Height = 62.0f * Scale;
	const float StartX = (CanvasWidth - AbilityCount * Width - (AbilityCount - 1) * Gap) * 0.5f;
	const float Y = HUD->GetDrawingCanvas()->ClipY - Height - 18.0f * Scale;

	for (int32 Index = 0; Index < AbilityCount; ++Index)
	{
		const FAbilityPanelEntry& Ability = Abilities[Index];
		const float X = StartX + Index * (Width + Gap);
		const bool bOnCooldown = Ability.Cooldown > 0.0f;
		const FString State = bOnCooldown ? FString::Printf(TEXT("READY IN %ds"), FMath::CeilToInt(Ability.Cooldown)) : TEXT("READY");
		const FLinearColor StateColor = bOnCooldown ? FLinearColor(0.95f, 0.65f, 0.15f) : FLinearColor(0.25f, 0.95f, 0.4f);

		HUD->DrawRect(FLinearColor(0.008f, 0.014f, 0.025f, 0.92f), X, Y, Width, Height);
		HUD->DrawText(
			Ability.Label, FLinearColor(0.82f, 0.86f, 0.92f), X + 12.0f * Scale, Y + 7.0f * Scale, GEngine->GetSmallFont(), 0.9f * Scale);
		HUD->DrawText(State, StateColor, X + 12.0f * Scale, Y + 30.0f * Scale, GEngine->GetMediumFont(), 0.85f * Scale);
	}
}
