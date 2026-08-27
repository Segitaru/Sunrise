#pragma once

#include "DetourCrowdAIController.h"

#include "SunriseUnitAIController.generated.h"

class UStateTreeAIComponent;
class UStateTree;

/** Uses Unreal's Detour crowd navigation and Gameplay StateTree brain for RTS units. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseUnitAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|AI")
	UStateTreeAIComponent* GetStateTreeComponent() const { return StateTreeComponent; }

	/** Enable after assigning a StateTree that fully owns unit decisions. */
	UFUNCTION(BlueprintPure, Category = "Sunrise|AI")
	bool IsStateTreeDrivingDecisions() const;
	/** Temporarily pauses authored autonomous decisions while a player order is active. */
	void SuspendDecisionLogicForPlayerOrder();
	void ResumeDecisionLogicAfterPlayerOrder();

protected:
	void ConfigureCrowdFollowing();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	/** Assign a StateTree using StateTreeAIComponentSchema on the Blueprint controller class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|StateTree",
		meta = (Schema = "/Script/GameplayStateTreeModule.StateTreeAIComponentSchema"))
	TObjectPtr<UStateTree> DecisionStateTree;

	/** Keeps the native unit decision fallback active until the authored tree is ready. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|StateTree")
	bool bStateTreeOwnsDecisionLogic = false;
	bool bPlayerOrderSuspended = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.0"))
	float SeparationWeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float CollisionQueryRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "100.0", Units = "cm"))
	float PathOptimizationRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|AI|Crowd", meta = (ClampMin = "0.1"))
	float AvoidanceRangeMultiplier = 1.25f;
};
