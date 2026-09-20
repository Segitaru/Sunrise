#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"

#include "SunriseUnitAIController.generated.h"

class ASunriseUnit;

/** Owns server-side Blackboard intent; Behavior Tree nodes execute movement and combat. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseUnitAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	bool IssueMoveOrder(const FVector& Destination, bool bFromPlayer);
	bool IssueTargetOrder(ASunriseUnit* Target, bool bFromPlayer);
	void StopOrders();
	void HandleUnitDeath(const ASunriseUnit* DeadUnit);
	bool HasActivePlayerOrder() const;
	FVector GetMovementGoal() const;

	bool ApplyFocusTarget(ASunriseUnit* Target, float Duration);
	void SetExternalInteractionActive(bool bActive);
	bool IsExternalInteractionActive() const { return bExternalInteractionActive; }

	/** Called by perception and the root BT service, including when a target dies or becomes incompatible. */
	void RefreshTargets();
	bool CanAttackTarget(const ASunriseUnit* Target) const;

	/** A completed/failed BT move may only consume the order it started with. Aborts never consume orders. */
	uint32 GetOrderRevision() const { return OrderRevision; }
	void CompleteMoveOrder(FName LocationKey, uint32 Revision, bool bSucceeded);

protected:
	UFUNCTION()
	void OnPerceptionInfoChanged(const FActorPerceptionUpdateInfo& UpdateInfo);

	void ConfigureCrowdFollowing();
	void UpdatePresentation();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.0"))
	float SeparationWeight = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float CollisionQueryRange = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float PathOptimizationRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.1"))
	float AvoidanceRangeMultiplier = 1.25f;

	FDelegateHandle PawnInitStateHandle;
	TWeakObjectPtr<ASunriseUnit> FocusTarget;
	float FocusTargetExpiryTime = 0.0f;
	uint32 OrderRevision = 0;
	bool bExternalInteractionActive = false;
};
