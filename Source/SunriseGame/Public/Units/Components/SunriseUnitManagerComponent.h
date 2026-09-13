#pragma once

#include "Components/GameStateComponent.h"

#include "SunriseUnitManagerComponent.generated.h"

class ASunriseUnit;
class AController;
class UModularPawnData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSunriseUnitDiedNative, ASunriseUnit*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSunriseArmyCountChanged, int32, FriendlyAlive, int32, EnemyAlive);

/** Shared unit registry and initial spawning. Each Pawn owns death and respawn through GAS. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseUnitManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	USunriseUnitManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static USunriseUnitManagerComponent* Find(const UObject* WorldContextObject);
	/** Transform is the Pawn origin. Team is inherited through the owner's team interface, when available. */
	static ASunriseUnit* SpawnUnit(const UModularPawnData* PawnData, const FTransform& Transform, AActor* Owner);
	ASunriseUnit* SpawnHeroForPlayer(AController* Controller, const FTransform& GroundTransform);
	ASunriseUnit* GetHeroForPlayer(const AController* Controller) const;
	const AController* GetPlayerForHero(const ASunriseUnit* Hero) const;
	void RegisterUnit(ASunriseUnit* Unit);
	void NotifyUnitDied(ASunriseUnit* Unit);
	int32 GetAliveUnitCountForTeam(int32 TeamId) const;
	int32 GetFriendlyAlive() const;
	int32 GetEnemyAlive() const;
	ASunriseUnit* GetLivingHeroForTeam(int32 TeamId) const;
	/** Legacy team HUD reports the earliest pending respawn among that team's Pawns. */
	float GetHeroRespawnSeconds(int32 TeamId) const;
	const TArray<TObjectPtr<ASunriseUnit>>& GetUnits() const { return Units; }
	FOnSunriseUnitDiedNative OnUnitDied;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Units")
	FOnSunriseArmyCountChanged OnArmyCountChanged;

private:
	// Server-only identity, independent of Pawn possession and Actor.Owner; retained while the hero is dead.
	TMap<TWeakObjectPtr<const AController>, TWeakObjectPtr<ASunriseUnit>> PlayerHeroes;
	UPROPERTY()
	TArray<TObjectPtr<ASunriseUnit>> Units;
};
