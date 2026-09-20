#include "Units/AI/SunriseUnitAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/ModularPawnExtensionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ModularGameplayTags.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Pawn/ModularPawnData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Units/AI/SunriseBlackboardData.h"
#include "Units/SunriseUnit.h"
#include "Vitality/Attributes/SunriseCombatSet.h"

using namespace SunriseBlackboardKeys;

ASunriseUnitAIController::ASunriseUnitAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass(TEXT("PathFollowingComponent"), UCrowdFollowingComponent::StaticClass()))
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	PerceptionComponent->OnTargetPerceptionInfoUpdated.AddDynamic(this, &ThisClass::OnPerceptionInfoChanged);
	UAISenseConfig_Damage* DamageSense = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageSense"));
	DamageSense->SetMaxAge(5.0f);
	PerceptionComponent->ConfigureSense(*DamageSense);
	bAllowStrafe = false;
}

void ASunriseUnitAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (!HasAuthority())
	{
		return;
	}

	if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(InPawn))
	{
		FActorInitStateChangedDelegate Delegate = FActorInitStateChangedDelegate::CreateWeakLambda(this,
			[this](const FActorInitStateChangedParams& Params)
			{
				if (Params.OwningActor != GetPawn())
				{
					return;
				}
				const UModularPawnExtensionComponent* Extension =
					UModularPawnExtensionComponent::FindPawnExtensionComponent(Params.OwningActor);
				const UModularPawnData* Data = Extension ? Extension->GetPawnData<UModularPawnData>() : nullptr;
				UBehaviorTree* Tree = Data ? Data->BehaviorTree.LoadSynchronous() : nullptr;
				UBlackboardComponent* BlackboardComponent = nullptr;
				if (Tree && Tree->BlackboardAsset && UseBlackboard(Tree->BlackboardAsset, BlackboardComponent))
				{
					StopOrders();
					RunBehaviorTree(Tree);
					const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
					if (Unit && !Unit->IsAlive() && BrainComponent)
					{
						BrainComponent->StopLogic(TEXT("Pawn is not alive"));
					}
				}
			});
		PawnInitStateHandle = Manager->RegisterAndCallForActorInitState(InPawn, UModularPawnExtensionComponent::NAME_ActorFeatureName,
			ModularGameplayTags::InitState_DataInitialized, MoveTemp(Delegate), true);
	}

	ConfigureCrowdFollowing();
	if (ACharacter* InCharacter = Cast<ACharacter>(InPawn))
	{
		InCharacter->GetCharacterMovement()->bUseRVOAvoidance = false;
	}
}

void ASunriseUnitAIController::OnUnPossess()
{
	if (APawn* InOldPawn = GetPawn())
	{
		if (UGameFrameworkComponentManager* Manager = UGameFrameworkComponentManager::GetForActor(InOldPawn))
		{
			Manager->UnregisterActorInitStateDelegate(InOldPawn, PawnInitStateHandle);
		}
	}
	PawnInitStateHandle.Reset();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Sunrise unit unpossessed"));
	}
	bExternalInteractionActive = false;
	StopOrders();
	PerceptionComponent->ForgetAll();
	Super::OnUnPossess();
}

bool ASunriseUnitAIController::HasActivePlayerOrder() const
{
	return Blackboard && Blackboard->GetValueAsBool(HavePlayerOrder);
}

FVector ASunriseUnitAIController::GetMovementGoal() const
{
	if (!Blackboard)
	{
		return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	}
	const FName Key = HasActivePlayerOrder() ? PlayerOrderTargetLocation : TargetLocation;
	if (Blackboard->IsVectorValueSet(Key))
	{
		return Blackboard->GetValueAsVector(Key);
	}
	return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

bool ASunriseUnitAIController::IssueMoveOrder(const FVector& Destination, bool bFromPlayer)
{
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!HasAuthority() || !Unit || !Unit->IsAlive() || !Blackboard || Destination.ContainsNaN() ||
		(!bFromPlayer && (HasActivePlayerOrder() || bExternalInteractionActive)))
	{
		return false;
	}
	const FName LocationKey = bFromPlayer ? PlayerOrderTargetLocation : TargetLocation;
	if (HasActivePlayerOrder() == bFromPlayer && Blackboard->IsVectorValueSet(LocationKey) &&
		Blackboard->GetValueAsVector(LocationKey).Equals(Destination))
	{
		return true;
	}
	const bool bWasExternalInteractionActive = bExternalInteractionActive;
	++OrderRevision;
	StopMovement();
	Blackboard->PauseObserverNotifications();
	if (bFromPlayer)
	{
		FocusTarget.Reset();
		Blackboard->ClearValue(PlayerOrderTargetActor);
		Blackboard->ClearValue(TargetActor);
		Blackboard->ClearValue(TargetLocation);
	}
	Blackboard->SetValueAsVector(bFromPlayer ? PlayerOrderTargetLocation : TargetLocation, Destination);
	Blackboard->SetValueAsBool(HavePlayerOrder, bFromPlayer);
	Blackboard->ResumeObserverNotifications(true);
	if (bFromPlayer)
	{
		SetExternalInteractionActive(false);
		// Restart even when the BT was already running: a summoned unit may still have FollowCreator active.
		if (!bWasExternalInteractionActive && BrainComponent)
		{
			BrainComponent->RestartLogic();
		}
	}
	UpdatePresentation();
	return true;
}

bool ASunriseUnitAIController::IssueTargetOrder(ASunriseUnit* Target, bool bFromPlayer)
{
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!HasAuthority() || !Unit || !Unit->IsAlive() || !Blackboard || !IsValid(Target) || !Target->IsAlive() || Target == Unit ||
		(!bFromPlayer && (HasActivePlayerOrder() || bExternalInteractionActive)))
	{
		return false;
	}
	if (!Unit->CanTargetWithWeapon(Target))
	{
		return bFromPlayer && IssueMoveOrder(Target->GetActorLocation(), true);
	}
	const bool bWasExternalInteractionActive = bExternalInteractionActive;
	++OrderRevision;
	StopMovement();
	Blackboard->PauseObserverNotifications();
	if (bFromPlayer)
	{
		FocusTarget.Reset();
		Blackboard->ClearValue(PlayerOrderTargetLocation);
		Blackboard->ClearValue(TargetActor);
		Blackboard->ClearValue(TargetLocation);
	}
	Blackboard->SetValueAsObject(bFromPlayer ? PlayerOrderTargetActor : TargetActor, Target);
	Blackboard->SetValueAsBool(HavePlayerOrder, bFromPlayer);
	Blackboard->ResumeObserverNotifications(true);
	if (bFromPlayer)
	{
		SetExternalInteractionActive(false);
		// Restart even when the BT was already running: a summoned unit may still have FollowCreator active.
		if (!bWasExternalInteractionActive && BrainComponent)
		{
			BrainComponent->RestartLogic();
		}
	}
	UpdatePresentation();
	return true;
}

void ASunriseUnitAIController::StopOrders()
{
	if (!HasAuthority())
	{
		return;
	}
	++OrderRevision;
	FocusTarget.Reset();
	const bool bWasExternalInteractionActive = bExternalInteractionActive;
	bExternalInteractionActive = false;
	if (Blackboard)
	{
		Blackboard->PauseObserverNotifications();
		Blackboard->ClearValue(PlayerOrderTargetActor);
		Blackboard->ClearValue(PlayerOrderTargetLocation);
		Blackboard->ClearValue(TargetActor);
		Blackboard->ClearValue(TargetLocation);
		Blackboard->SetValueAsBool(HavePlayerOrder, false);
		Blackboard->ResumeObserverNotifications(true);
	}
	StopMovement();
	UpdatePresentation();
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (bWasExternalInteractionActive && Unit && Unit->IsAlive() && BrainComponent)
	{
		BrainComponent->RestartLogic();
	}
}

bool ASunriseUnitAIController::ApplyFocusTarget(ASunriseUnit* Target, float Duration)
{
	if (!FMath::IsFinite(Duration) || Duration <= 0.0f || !GetWorld() || !IssueTargetOrder(Target, false))
	{
		return false;
	}
	FocusTarget = Target;
	FocusTargetExpiryTime = GetWorld()->GetTimeSeconds() + Duration;
	return true;
}

void ASunriseUnitAIController::SetExternalInteractionActive(bool bActive)
{
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!HasAuthority() || !Unit || !Unit->IsAlive() || bExternalInteractionActive == bActive || (bActive && HasActivePlayerOrder()))
	{
		return;
	}
	bExternalInteractionActive = bActive;
	if (BrainComponent)
	{
		if (bActive)
		{
			// Stop, rather than pause, so a latent MoveTo cannot keep competing with the external owner.
			BrainComponent->StopLogic(TEXT("External interaction owns movement"));
			StopMovement();
		}
		else
		{
			BrainComponent->RestartLogic();
		}
	}
	UpdatePresentation();
}

void ASunriseUnitAIController::OnPerceptionInfoChanged(const FActorPerceptionUpdateInfo& UpdateInfo)
{
	if (ASunriseUnit* Damager = Cast<ASunriseUnit>(UpdateInfo.Target))
	{
		if (UpdateInfo.Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
		{
			ApplyFocusTarget(Damager, 5.0f);
		}
	}
	// Query all senses: losing one stimulus must not discard an actor still perceived by another sense.
	RefreshTargets();
}

void ASunriseUnitAIController::RefreshTargets()
{
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!HasAuthority() || !Unit || !Unit->IsAlive() || !Blackboard || bExternalInteractionActive)
	{
		return;
	}
	Blackboard->PauseObserverNotifications();
	if (HasActivePlayerOrder())
	{
		ASunriseUnit* PlayerTarget = Cast<ASunriseUnit>(Blackboard->GetValueAsObject(PlayerOrderTargetActor));
		if (!Unit->CanTargetWithWeapon(PlayerTarget) && !Blackboard->IsVectorValueSet(PlayerOrderTargetLocation))
		{
			++OrderRevision;
			Blackboard->ClearValue(PlayerOrderTargetActor);
			Blackboard->SetValueAsBool(HavePlayerOrder, false);
		}
	}
	ASunriseUnit* Best = nullptr;
	if (!HasActivePlayerOrder())
	{
		if (GetWorld()->GetTimeSeconds() < FocusTargetExpiryTime && Unit->CanTargetWithWeapon(FocusTarget.Get()))
		{
			Best = FocusTarget.Get();
		}
		else
		{
			FocusTarget.Reset();
			TArray<AActor*> PerceivedActors;
			PerceptionComponent->GetCurrentlyPerceivedActors(TSubclassOf<UAISense>(), PerceivedActors);
			float BestScore = TNumericLimits<float>::Max();
			for (AActor* Actor : PerceivedActors)
			{
				ASunriseUnit* Candidate = Cast<ASunriseUnit>(Actor);
				if (!Unit->CanTargetWithWeapon(Candidate))
				{
					continue;
				}
				const float Distance = FVector::DistSquared2D(Unit->GetActorLocation(), Candidate->GetActorLocation());
				const float Score =
					Unit->HasPawnTag(SunrisePawnTags::Class_Healer) ? Candidate->GetHealthPercent() * 100000000.0f + Distance : Distance;
				if (Score < BestScore)
				{
					BestScore = Score;
					Best = Candidate;
				}
			}
		}
	}
	Blackboard->SetValueAsObject(TargetActor, Best);
	Blackboard->ResumeObserverNotifications(true);
	UpdatePresentation();
}

bool ASunriseUnitAIController::CanAttackTarget(const ASunriseUnit* Target) const
{
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!HasAuthority() || !Unit || !Unit->IsAlive() || bExternalInteractionActive || !Unit->CanTargetWithWeapon(Target) ||
		!Unit->GetCombatSet())
	{
		return false;
	}
	// The BT node resolves Target from its own configured Blackboard key. Do not
	// reject a valid target because another branch currently owns the active key.
	return FVector::Distance(Unit->GetActorLocation(), Target->GetActorLocation()) <= Unit->GetCombatSet()->GetActionRange() * 1.15;
}

void ASunriseUnitAIController::CompleteMoveOrder(FName LocationKey, uint32 Revision, bool bSucceeded)
{
	if (!HasAuthority() || !Blackboard || Revision != OrderRevision ||
		(LocationKey != PlayerOrderTargetLocation && LocationKey != TargetLocation) || !Blackboard->IsVectorValueSet(LocationKey))
	{
		return;
	}
	++OrderRevision;
	Blackboard->PauseObserverNotifications();
	Blackboard->ClearValue(LocationKey);
	if (LocationKey == PlayerOrderTargetLocation)
	{
		Blackboard->SetValueAsBool(HavePlayerOrder, false);
	}
	Blackboard->ResumeObserverNotifications(true);
	UpdatePresentation();
	if (ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn()); Unit && bSucceeded)
	{
		Unit->OnMoveCompleted.Broadcast(Unit);
	}
}

void ASunriseUnitAIController::UpdatePresentation()
{
	ASunriseUnit* Unit = Cast<ASunriseUnit>(GetPawn());
	if (!Unit || !Blackboard)
	{
		return;
	}
	ASunriseUnit* Target =
		!bExternalInteractionActive
			? Cast<ASunriseUnit>(Blackboard->GetValueAsObject(HasActivePlayerOrder() ? PlayerOrderTargetActor : TargetActor))
			: nullptr;
	ESunriseOrderState State = ESunriseOrderState::Idle;
	if (Target)
	{
		State = Unit->HasPawnTag(SunrisePawnTags::Class_Healer) ? ESunriseOrderState::Healing : ESunriseOrderState::Attacking;
	}
	else if (!bExternalInteractionActive &&
			 Blackboard->IsVectorValueSet(HasActivePlayerOrder() ? PlayerOrderTargetLocation : TargetLocation))
	{
		State = ESunriseOrderState::Moving;
	}
	Unit->SetAIOrderPresentation(Target, State);
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
