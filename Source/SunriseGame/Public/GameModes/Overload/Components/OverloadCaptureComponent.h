// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "OverloadCaptureComponent.generated.h"

class ASunriseUnit;
class UTFTeamActorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverloadCaptureProgress, int32, HackingTeamId, float, NormalizedProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverloadCaptureCompleted, int32, PreviousTeamId, int32, NewTeamId);

UCLASS(ClassGroup = (Overload), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UOverloadCaptureComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOverloadCaptureComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Overload|Hack")
	void RegisterHacker(ASunriseUnit* Unit);
	UFUNCTION(BlueprintCallable, Category = "Overload|Hack")
	void UnregisterHacker(ASunriseUnit* Unit);
	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	float GetCaptureProgress() const { return CaptureProgress; }
	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	int32 GetActiveHackingTeamId() const { return ActiveHackingTeamId; }
	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	int32 GetActiveCapturingUnitCount() const { return ActiveCapturingUnitCount; }
	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	bool IsCaptureContested() const { return bCaptureContested; }
	UFUNCTION(BlueprintPure, Category = "Overload|Hack")
	float GetHackRadius() const { return HackRadius; }

	UPROPERTY(BlueprintAssignable, Category = "Overload|Hack")
	FOnOverloadCaptureProgress OnCaptureProgress;
	UPROPERTY(BlueprintAssignable, Category = "Overload|Hack")
	FOnOverloadCaptureCompleted OnCaptured;

protected:
	/** Seconds required by one uncontested unit before tower resistance is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Hack", meta = (ClampMin = "0.1", Units = "s"))
	float BaseHackSeconds = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Hack", meta = (ClampMin = "50.0", Units = "cm"))
	float HackRadius = 260.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Hack", meta = (ClampMin = "0.0"))
	float IdleDecayPerSecond = 0.12f;

private:
	void PruneAndCountHackers(TMap<int32, int32>& OutCounts);
	void CountPresentTeams(const FVector& CaptureLocation, TMap<int32, int32>& OutCounts) const;
	void UpdateHackerInteractionLocks(bool bLockCapturingTeam, int32 CapturingTeamId);
	void DecayCapture(float DeltaTime);
	float GetOwnerHackResistance() const;

	UPROPERTY(Replicated)
	float CaptureProgress = 0.0f;
	UPROPERTY(Replicated)
	int32 ActiveHackingTeamId = INDEX_NONE;
	UPROPERTY(Replicated)
	int32 ActiveCapturingUnitCount = 0;
	UPROPERTY(Replicated)
	bool bCaptureContested = false;
	UPROPERTY()
	TObjectPtr<UTFTeamActorComponent> TeamComponent;
	TArray<TWeakObjectPtr<ASunriseUnit>> Hackers;
};
