// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "ControllableEntities/IControllableEntity.h"
#include "Units/SunriseUnitTypes.h"

#include "SunriseUnitManagerComponent.generated.h"

class ASunriseUnit;

USTRUCT()
struct FSunriseHeroRespawnData
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<ASunriseUnit> HeroClass;
	UPROPERTY()
	FTransform SpawnTransform;
	UPROPERTY()
	ESunriseUnitRole Role = ESunriseUnitRole::Vanguard;
	UPROPERTY()
	int32 TeamId = INDEX_NONE;
	UPROPERTY()
	TScriptInterface<IIControllableEntity> ControllingAgent;
	float RespawnDelay = 12.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSunriseUnitDiedNative, ASunriseUnit*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSunriseArmyCountChanged, int32, FriendlyAlive, int32, EnemyAlive);

/** Shared authoritative unit/hero registry. It contains no victory condition. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseUnitManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	USunriseUnitManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static USunriseUnitManagerComponent* Find(const UObject* WorldContextObject);
	void RegisterUnit(ASunriseUnit* Unit);
	void NotifyUnitDied(ASunriseUnit* Unit);
	ASunriseUnit* SpawnHeroForTeam(int32 TeamId, const FTransform& SpawnTransform, TScriptInterface<IIControllableEntity> ControllingAgent);

	int32 GetAliveUnitCountForTeam(int32 TeamId) const;
	int32 GetFriendlyAlive() const;
	int32 GetEnemyAlive() const;
	ASunriseUnit* GetLivingHeroForTeam(int32 TeamId) const;
	float GetHeroRespawnSeconds(int32 TeamId) const;
	const TArray<TObjectPtr<ASunriseUnit>>& GetUnits() const { return Units; }

	FOnSunriseUnitDiedNative OnUnitDied;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Units")
	FOnSunriseArmyCountChanged OnArmyCountChanged;

protected:
	void ScheduleHeroRespawn(ASunriseUnit* Hero);
	void RespawnHeroForTeam(int32 TeamId);

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Units")
	TSubclassOf<ASunriseUnit> UnitClass;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Hero")
	TSubclassOf<ASunriseUnit> HeroClass;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Hero")
	ESunriseUnitRole HeroRole = ESunriseUnitRole::Mage;

private:
	UPROPERTY()
	TArray<TObjectPtr<ASunriseUnit>> Units;
	UPROPERTY()
	TMap<int32, FSunriseHeroRespawnData> HeroRespawnData;
	TMap<int32, FTimerHandle> HeroRespawnTimers;
};
