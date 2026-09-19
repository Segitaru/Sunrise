#pragma once

#if UE_VERSION_5_8_x
#include "DetourCrowdAIController.h"
#else
#include "AIController.h"
#endif

#include "SunriseUnitAIController.generated.h"

class UStateTreeAIComponent;
class UStateTree;
class UAIPerceptionComponent;

struct FActorPerceptionUpdateInfo;
#if UE_VERSION_5_8_x
using ASunriseTargetAIController = ADetourCrowdAIController;
#else
using ASunriseTargetAIController = AAIController;
#endif

/** Uses Unreal's Detour crowd navigation and Gameplay StateTree brain for RTS units. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseUnitAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|AI")
	UStateTreeComponent* GetStateTreeComponent() const { return StateTreeComponent; }

	/** Enable after assigning a StateTree that fully owns unit decisions. */
	UFUNCTION(BlueprintPure, Category = "Sunrise|AI")
	bool IsStateTreeDrivingDecisions() const;

	/** Temporarily pauses authored autonomous decisions while a player order is active. */
	void SuspendDecisionLogicForPlayerOrder();

	void ResumeDecisionLogicAfterPlayerOrder();

	void OnPerceptionInfoChanged(const FActorPerceptionUpdateInfo& UpdateInfo);

protected:
	void ConfigureCrowdFollowing();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sunrise|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	/** Assign a StateTree using StateTreeComponentSchema on the Blueprint controller class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|StateTree",
		meta = (Schema = "/Script/GameplayStateTreeModule.StateTreeComponentSchema"))
	TObjectPtr<UStateTree> DecisionStateTree;

	/** Keeps the native unit decision fallback active until the authored tree is ready. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|StateTree")
	bool bStateTreeOwnsDecisionLogic = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.0"))
	float SeparationWeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float CollisionQueryRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float PathOptimizationRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.1"))
	float AvoidanceRangeMultiplier = 1.25f;

	bool bPlayerOrderSuspended = false;
};
