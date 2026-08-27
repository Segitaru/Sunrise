// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/EFGameMatchComponent.h"
#include "Units/SunriseUnitTypes.h"

#include "SunriseGameMatchComponent.generated.h"

class ASunriseUnit;
class UEFExperienceDefinition;
class USunriseEndScreenWidget;
class USunriseUnitManagerComponent;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSunriseMatchFinished, ESunriseMatchResult, Result);

/** Optional elimination scenario. Experiences may compose it with other match components. */
UCLASS(Blueprintable, BlueprintType)
class SUNRISEGAME_API USunriseGameMatchComponent : public UEFGameMatchComponent
{
	GENERATED_BODY()

public:
	USunriseGameMatchComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static USunriseGameMatchComponent* Find(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Match")
	ESunriseMatchResult GetMatchResult() const { return MatchResult; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Match")
	int32 GetFriendlyAlive() const;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Match")
	int32 GetEnemyAlive() const;
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Match")
	void ReturnToMainMenu();

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Match")
	FOnSunriseMatchFinished OnMatchFinished;

protected:
	void HandleExperienceLoaded(const UEFExperienceDefinition* CurrentExperience);
	UFUNCTION()
	void InitializeScenario();
	void HandleUnitDied(ASunriseUnit* Unit);
	void EvaluateMatch();
	void FinishMatch(ESunriseMatchResult Result);
	UFUNCTION()
	void OnRep_MatchResult();
	ASunriseUnit* SpawnUnitAtAvailableLocation(
		ESunriseTeam Team, ESunriseUnitRole InRole, const FVector& ArmyCenter, float SpawnRadius, int32 FormationIndex);
	USunriseUnitManagerComponent* GetUnitManager() const;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario")
	TSubclassOf<ASunriseUnit> UnitClass;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario", meta = (ClampMin = "1"))
	int32 BaseFriendlyUnitCount = 5;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario", meta = (ClampMin = "1"))
	int32 BaseEnemyUnitCount = 5;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario", meta = (ClampMin = "100.0", Units = "cm"))
	float ArmySpawnRadius = 650.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario", meta = (ClampMin = "300.0", Units = "cm"))
	float MinimumOpposingArmySeparation = 1200.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Scenario", meta = (ClampMin = "0.1", Units = "s"))
	float ScenarioInitializationDelay = 0.35f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_MatchResult, Category = "Sunrise|Match")
	ESunriseMatchResult MatchResult = ESunriseMatchResult::InProgress;
	UPROPERTY()
	TObjectPtr<USunriseEndScreenWidget> EndScreen;

	bool bMatchArmed = false;
	bool bScenarioInitialized = false;
	int32 NavigationWaitAttempts = 0;
	FTimerHandle ScenarioTimer;
	int32 InitialEnemyCount = 0;
	float MatchStartTime = 0.0f;
};
