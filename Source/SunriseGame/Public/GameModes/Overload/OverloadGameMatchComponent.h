// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/EFGameMatchComponent.h"
#include "GameModes/Overload/Types/OverloadTypes.h"
#include "Units/SunriseUnitTypes.h"

#include "OverloadGameMatchComponent.generated.h"

class AOverloadEnergyCore;
class AOverloadGuardTower;
class AOverloadLaneSpline;
class ASunriseUnit;
class UEFExperienceDefinition;
class USunriseEndScreenWidget;
class USunriseUnitManagerComponent;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverloadTeamEliminated, int32, EliminatedTeamId, int32, OverloadingTeamId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverloadWinnerDetermined, int32, WinnerTeamId);

/** Independent Overload objective condition, composed onto GameState by an Experience/Game Feature. */
UCLASS(Blueprintable, BlueprintType)
class SUNRISEGAME_API UOverloadGameMatchComponent : public UEFGameMatchComponent
{
	GENERATED_BODY()

public:
	UOverloadGameMatchComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static UOverloadGameMatchComponent* Find(const UObject* WorldContextObject);
	const TArray<AOverloadLaneSpline*>& GetLanes() const { return LaneView; }
	const TArray<AOverloadGuardTower*>& GetTowers() const { return TowerView; }
	const TArray<AOverloadEnergyCore*>& GetCores() const { return CoreView; }
	bool IsOverloadInitialized() const { return bOverloadInitialized; }
	int32 GetAliveUnitCountForTeam(int32 TeamId) const;
	AOverloadEnergyCore* GetCoreForTeam(int32 TeamId) const;
	ASunriseUnit* GetLivingHeroForTeam(int32 TeamId) const;
	float GetHeroRespawnSeconds(int32 TeamId) const;
	ESunriseMatchResult GetMatchResult() const;

	UPROPERTY(BlueprintAssignable, Category = "Overload")
	FOnOverloadTeamEliminated OnTeamEliminated;
	UPROPERTY(BlueprintAssignable, Category = "Overload")
	FOnOverloadWinnerDetermined OnWinnerDetermined;

protected:
	void HandleExperienceLoaded(const UEFExperienceDefinition* CurrentExperience);
	UFUNCTION()
	void InitializeOverloadMode();
	UFUNCTION()
	void HandleTowerCaptured(AOverloadGuardTower* Tower, int32 PreviousTeamId, int32 NewTeamId);
	UFUNCTION()
	void HandleCoreExploded(AOverloadEnergyCore* Core, int32 OverloadingTeamId);
	void PresentMatchResult();

	void BuildObjectivesForLane(AOverloadLaneSpline* Lane);
	void EnsureCore(int32 TeamId, const FVector& DesiredLocation, const FRotator& DesiredRotation);
	void RecalculateSupplyAndBalance();
	UFUNCTION()
	void EnsureEnemyHeroFollowers();
	FVector ResolveGroundLocation(const FVector& DesiredLocation) const;
	USunriseUnitManagerComponent* GetUnitManager() const;
	UFUNCTION()
	void OnRep_RuntimeState();

	UPROPERTY(EditDefaultsOnly, Category = "Overload|Classes")
	TSubclassOf<AOverloadGuardTower> TowerClass;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Classes")
	TSubclassOf<AOverloadEnergyCore> CoreClass;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Classes")
	TSubclassOf<ASunriseUnit> WaveUnitClass;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Balance")
	FOverloadBalanceTuning BalanceTuning;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Setup", meta = (ClampMin = "0.1", Units = "s"))
	float InitializationRetryDelay = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Setup", meta = (ClampMin = "100.0", Units = "cm"))
	float HeroSpawnOffset = 700.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	TArray<TObjectPtr<AOverloadLaneSpline>> Lanes;
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	TArray<TObjectPtr<AOverloadGuardTower>> Towers;
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	TArray<TObjectPtr<AOverloadEnergyCore>> CoreActors;
	UPROPERTY()
	TMap<int32, TObjectPtr<AOverloadEnergyCore>> CoresByTeam;
	TArray<AOverloadLaneSpline*> LaneView;
	TArray<AOverloadGuardTower*> TowerView;
	TArray<AOverloadEnergyCore*> CoreView;
	FTimerHandle InitializationTimer;
	FTimerHandle HeroFollowerTimer;
	int32 InitializationAttempts = 0;
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	bool bOverloadInitialized = false;
	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState)
	int32 WinnerTeamId = INDEX_NONE;
	UPROPERTY(Transient)
	TObjectPtr<USunriseEndScreenWidget> EndScreen;
	bool bMatchRecorded = false;
};
