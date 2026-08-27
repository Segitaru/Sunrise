#include "Units/AI/SunriseUnitAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "StateTree.h"

ASunriseUnitAIController::ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);
	BrainComponent = StateTreeComponent;
	bAllowStrafe = false;
}

void ASunriseUnitAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ConfigureCrowdFollowing();
	if (DecisionStateTree && StateTreeComponent)
	{
		if (StateTreeComponent->IsRunning())
		{
			StateTreeComponent->StopLogic(TEXT("Sunrise unit repossessed"));
		}
		StateTreeComponent->SetStateTree(DecisionStateTree);
		StateTreeComponent->StartLogic();
	}

	// Do not let RVO and Detour crowd steering alter the same velocity simultaneously.
	if (ACharacter* InCharacter = Cast<ACharacter>(InPawn))
	{
		InCharacter->GetCharacterMovement()->bUseRVOAvoidance = false;
	}
}

void ASunriseUnitAIController::OnUnPossess()
{
	if (StateTreeComponent && StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StopLogic(TEXT("Sunrise unit unpossessed"));
	}
	Super::OnUnPossess();
}

bool ASunriseUnitAIController::IsStateTreeDrivingDecisions() const
{
	return !bPlayerOrderSuspended && bStateTreeOwnsDecisionLogic && StateTreeComponent && StateTreeComponent->IsRunning();
}

void ASunriseUnitAIController::SuspendDecisionLogicForPlayerOrder()
{
	bPlayerOrderSuspended = true;
	if (StateTreeComponent && StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StopLogic(TEXT("Sunrise player order has priority"));
	}
}

void ASunriseUnitAIController::ResumeDecisionLogicAfterPlayerOrder()
{
	bPlayerOrderSuspended = false;
	if (DecisionStateTree && StateTreeComponent && !StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StartLogic();
	}
}

void ASunriseUnitAIController::ConfigureCrowdFollowing()
{
	UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent());
	if (!Crowd)
	{
		return;
	}

	Crowd->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
	Crowd->SetCrowdAnticipateTurns(true, false);
	Crowd->SetCrowdObstacleAvoidance(true, false);
	Crowd->SetCrowdSeparation(true, false);
	Crowd->SetCrowdOptimizeVisibility(true, false);
	Crowd->SetCrowdOptimizeTopology(true, false);
	Crowd->SetCrowdPathOffset(true, false);
	Crowd->SetCrowdSlowdownAtGoal(true, false);
	Crowd->SetCrowdSeparationWeight(SeparationWeight, false);
	Crowd->SetCrowdCollisionQueryRange(CollisionQueryRange, false);
	Crowd->SetCrowdPathOptimizationRange(PathOptimizationRange, false);
	Crowd->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High, false);
	Crowd->SetCrowdAvoidanceRangeMultiplier(AvoidanceRangeMultiplier, false);
	Crowd->SetCrowdRotateToVelocity(true);
	Crowd->UpdateCrowdAgentParams();
}
