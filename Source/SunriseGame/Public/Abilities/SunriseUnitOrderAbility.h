// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"
#include "NativeGameplayTags.h"

#include "SunriseUnitOrderAbility.generated.h"

class ASunriseUnit;
class USunriseHeroSquadAbility;

namespace SunriseOrders
{
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Move);
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target);
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hack);
	SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stop);
} // namespace SunriseOrders

/** Pawn-side command router. It validates player intent and forwards execution to Unit ASC abilities. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseUnitOrderAbility : public UModularGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseUnitOrderAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	static bool SendOrderEvent(AActor* Pawn, FGameplayTag Tag, const TArray<ASunriseUnit*>& Units, ASunriseUnit* SingleUnit = nullptr,
		const FVector& Location = FVector::ZeroVector, AActor* Target = nullptr);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Orders", meta = (ClampMin = "50", Units = "cm"))
	float FormationSpacing = 170.0f;

private:
	bool DispatchOrder(const FGameplayEventData& Event, const FGameplayAbilityActorInfo* ActorInfo);
};
