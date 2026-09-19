#include "Units/AI/SunriseUnitAIController.h"

#include <Components/GameFrameworkComponentManager.h>
#include <Perception/AIPerceptionComponent.h>

#include "Components/ModularPawnExtensionComponent.h"
#include "Components/StateTreeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ModularGameplayTags.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "StateTree.h"

ASunriseUnitAIController::ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);
	BrainComponent = StateTreeComponent;
	bAllowStrafe = false;
	PerceptionComponent->OnTargetPerceptionInfoUpdated.AddDynamic(this, OnPerceptionInfoChanged);
}

void ASunriseUnitAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(InPawn))
	{
		// Bind as a weak lambda because this is not a UObject but is guaranteed to be valid as long as ThisObject is
		FActorInitStateChangedDelegate Delegate = FActorInitStateChangedDelegate::CreateWeakLambda(this,
			[this](const FActorInitStateChangedParams& Params)
			{
				if (UModularPawnExtensionComponent* PawnExtensionComponent =
						UModularPawnExtensionComponent::FindPawnExtensionComponent(Params.OwningActor))
				{
					if (UBehaviorTree* const LoadedTree =
							PawnExtensionComponent->GetPawnData<UModularPawnData>()->BehaviorTree.LoadSynchronous())
					{
						RunBehaviorTree(LoadedTree);
					}
				}
			});

		Manager->RegisterAndCallForActorInitState(InPawn, UModularPawnExtensionComponent::NAME_ActorFeatureName,
			ModularGameplayTags::InitState_DataInitialized, MoveTemp(Delegate), true);
	}


	ConfigureCrowdFollowing();
	if (DecisionStateTree && StateTreeComponent)
	{
		if (StateTreeComponent->IsRunning())
		{
			StateTreeComponent->StopLogic(TEXT("Sunrise unit repossessed"));
		}
#if UE_VERSION_5_8_x
		StateTreeComponent->SetStateTree(DecisionStateTree);
#endif

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

void ASunriseUnitAIController::OnPerceptionInfoChanged(const FActorPerceptionUpdateInfo& UpdateInfo)
{
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
