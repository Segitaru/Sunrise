// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"

#include "OverloadInteractorComponent.generated.h"

class ASunriseUnit;
class UEnvQuery;
class UGameplayAbility;
class USunriseCombatSet;
struct FEnvQueryResult;

UCLASS(ClassGroup = (Overload), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UOverloadInteractorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOverloadInteractorComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void InitializeForUnit();
	UFUNCTION(BlueprintCallable, Category = "Overload|Hack")
	bool RequestHack(AActor* Target, bool bFromPlayer = false);
	UFUNCTION(BlueprintCallable, Category = "Overload|Hack")
	void CancelHack();
	void CommitHack();

	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	AActor* GetHackTarget() const { return HackTarget.Get(); }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Hack")
	TSubclassOf<UGameplayAbility> HackAbilityClass;
	UPROPERTY(EditDefaultsOnly, Category = "Overload|Hack")
	TObjectPtr<UEnvQuery> HackApproachQuery;
	UPROPERTY(EditAnywhere, Category = "Overload|Hack", meta = (ClampMin = "50.0", Units = "cm"))
	float InteractionRange = 220.0f;
	/** Units use deterministic points on this ring instead of converging on the terminal origin. */
	UPROPERTY(EditAnywhere, Category = "Overload|Hack", meta = (ClampMin = "50.0", ClampMax = "200.0", Units = "cm"))
	float ApproachRingRadius = 150.0f;
	UPROPERTY(EditAnywhere, Category = "Overload|Hack", meta = (ClampMin = "4", ClampMax = "24"))
	int32 ApproachSlotCount = 12;
	UPROPERTY(EditAnywhere, Category = "Overload|Hack", meta = (ClampMin = "5.0", Units = "cm"))
	float ApproachAcceptanceRadius = 35.0f;

private:
	void StartApproachQuery();
	void HandleApproachQueryFinished(TSharedPtr<FEnvQueryResult> Result);
	bool ShouldAbortHackForThreat() const;
	void IssueApproachMove();
	FVector ResolveFallbackApproachLocation(AActor* Target) const;

	TObjectPtr<ASunriseUnit> Unit;
	TWeakObjectPtr<AActor> HackTarget;
	FVector ApproachLocation = FVector::ZeroVector;
	float ApproachQueryStartTime = -1.0f;
	float LastObservedHealth = 0.0f;
	FGameplayAbilitySpecHandle HackAbilityHandle;
	bool bRegisteredAtTarget = false;
	bool bApproachQueryPending = false;
	bool bApproachMoveIssued = false;
	UPROPERTY(EditAnywhere, Category = "Overload|Hack", meta = (ClampMin = "0.1", Units = "s"))
	float ApproachQueryTimeout = 1.0f;
};
