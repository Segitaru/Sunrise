#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "Environment/Resources/SunriseResourceNode.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Units/AI/SunriseUnitAIController.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalWorkerComponent)

namespace
{
	bool IsWithinInteractionRange(const AActor* Worker, const AActor* Target, float Range)
	{
		if (!Worker || !Target)
		{
			return false;
		}
		const FBox Bounds = Target->GetComponentsBoundingBox(true);
		const FVector Location = Worker->GetActorLocation();
		const FVector ClosestPoint(FMath::Clamp(Location.X, Bounds.Min.X, Bounds.Max.X),
			FMath::Clamp(Location.Y, Bounds.Min.Y, Bounds.Max.Y), FMath::Clamp(Location.Z, Bounds.Min.Z, Bounds.Max.Z));
		return FVector::DistSquared(Location, ClosestPoint) <= FMath::Square(Range);
	}

	FVector GetInteractionApproachLocation(const AActor* Worker, const AActor* Target, float Range)
	{
		const FBox Bounds = Target->GetComponentsBoundingBox(true);
		FVector Direction = Worker->GetActorLocation() - Bounds.GetCenter();
		Direction.Z = 0.0f;
		Direction = Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = -Target->GetActorForwardVector().GetSafeNormal2D();
		}
		const FVector Extent = Bounds.GetExtent();
		const float BoundsRadius = FMath::Abs(Direction.X) * Extent.X + FMath::Abs(Direction.Y) * Extent.Y;
		return Bounds.GetCenter() + Direction * (BoundsRadius + Range * 0.5f);
	}
} // namespace

USurvivalWorkerComponent::USurvivalWorkerComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void USurvivalWorkerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CargoType);
	DOREPLIFETIME(ThisClass, CargoAmount);
}

int32 USurvivalWorkerComponent::GatherFrom(ASunriseResourceNode* Node)
{
	AActor* Worker = GetOwner();
	if (!Worker || !Worker->HasAuthority() || !IsValid(Node) || !IsWithinInteractionRange(Worker, Node, InteractionRange) ||
		(CargoAmount > 0 && CargoType != Node->GetResourceType()))
	{
		return 0;
	}
	CargoType = Node->GetResourceType();
	const int32 Extracted = Node->Extract(FMath::Min(GatherAmount, CarryCapacity - CargoAmount));
	CargoAmount += Extracted;
	return Extracted;
}

bool USurvivalWorkerComponent::DepositAt(ASurvivalBuilding* Building)
{
	AActor* Worker = GetOwner();
	if (!Worker || !Worker->HasAuthority() || !IsValid(Building) || !Building->IsAlive() || !Building->IsConstructionComplete() ||
		CargoAmount <= 0 || !IsWithinInteractionRange(Worker, Building, InteractionRange) ||
		(Building->GetBuildingRole() != ESurvivalBuildingRole::MainBase &&
			Building->GetBuildingRole() != ESurvivalBuildingRole::Storehouse))
	{
		return false;
	}
	USurvivalEconomyComponent* Economy = USurvivalEconomyComponent::Find(Building->GetOwner());
	if (!Economy)
	{
		return false;
	}
	FSurvivalResourceAmounts Deposit;
	switch (CargoType)
	{
		case ESurvivalResourceType::Food:
			Deposit.Food = CargoAmount;
			break;
		case ESurvivalResourceType::Wood:
			Deposit.Wood = CargoAmount;
			break;
		case ESurvivalResourceType::Stone:
			Deposit.Stone = CargoAmount;
			break;
		case ESurvivalResourceType::Metal:
			Deposit.Metal = CargoAmount;
			break;
	}
	Economy->AddResources(Deposit);
	CargoAmount = 0;
	return true;
}

bool USurvivalWorkerComponent::StartGatherOrder(ASunriseResourceNode* Node)
{
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	if (!Worker || !Worker->HasAuthority() || !Worker->IsAlive() || !IsValid(Node) || Node->GetRemainingAmount() <= 0)
	{
		return false;
	}
	if (CargoAmount > 0 && CargoType != Node->GetResourceType())
	{
		return false;
	}
	TargetNode = Node;
	TargetDeposit.Reset();
	TargetBuilding.Reset();
	WorkerOrderState = EWorkerOrderState::Gathering;
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(Worker->GetController()))
	{
		AI->StopOrders();
	}
	Worker->SetExternalInteractionActive(true);
	MoveOwnerTo(Node);
	SetComponentTickEnabled(true);
	return true;
}

bool USurvivalWorkerComponent::StartBuildOrder(ASurvivalBuilding* Building)
{
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	if (!Worker || !Worker->HasAuthority() || !Worker->IsAlive() || !IsValid(Building) || Building->IsConstructionComplete() ||
		Building->GetOwner() == nullptr)
	{
		return false;
	}

	UObject* ControllingAgent = Worker->GetControllingAgent().GetObject();
	const AController* Controller = Cast<AController>(ControllingAgent);
	const APlayerState* WorkerPlayerState = IsValid(Controller) ? Controller->PlayerState.Get() : Cast<APlayerState>(ControllingAgent);
	if (Building->GetOwner() != WorkerPlayerState)
	{
		return false;
	}

	TargetNode.Reset();
	TargetDeposit.Reset();
	TargetBuilding = Building;
	WorkerOrderState = EWorkerOrderState::Constructing;
	if (ASunriseUnitAIController* AI = Cast<ASunriseUnitAIController>(Worker->GetController()))
	{
		AI->StopOrders();
	}
	Worker->SetExternalInteractionActive(true);
	MoveOwnerTo(Building);
	SetComponentTickEnabled(true);
	return true;
}

void USurvivalWorkerComponent::CancelWorkerOrder()
{
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	if (Worker && Worker->HasAuthority())
	{
		if (AAIController* AI = Cast<AAIController>(Worker->GetController()))
		{
			AI->StopMovement();
		}
		Worker->SetExternalInteractionActive(false);
	}
	TargetNode.Reset();
	TargetDeposit.Reset();
	TargetBuilding.Reset();
	WorkerOrderState = EWorkerOrderState::None;
	SetComponentTickEnabled(false);
}

void USurvivalWorkerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	if (!Worker || !Worker->HasAuthority() || !Worker->IsAlive() || WorkerOrderState == EWorkerOrderState::None)
	{
		CancelWorkerOrder();
		return;
	}

	MoveRefreshRemaining -= DeltaTime;
	if (WorkerOrderState == EWorkerOrderState::Constructing)
	{
		ASurvivalBuilding* Building = TargetBuilding.Get();
		if (!Building || !Building->IsAlive() || Building->IsConstructionComplete())
		{
			CancelWorkerOrder();
			return;
		}
		if (!IsWithinInteractionRange(Worker, Building, InteractionRange))
		{
			if (MoveRefreshRemaining <= 0.0f)
			{
				MoveOwnerTo(Building);
			}
			return;
		}
		Worker->StopMoving();
		if (Building->ApplyConstructionWork(DeltaTime))
		{
			CancelWorkerOrder();
		}
		return;
	}
	if (WorkerOrderState == EWorkerOrderState::Gathering)
	{
		ASunriseResourceNode* Node = TargetNode.Get();
		if (!Node || (Node->GetRemainingAmount() <= 0 && CargoAmount <= 0))
		{
			CancelWorkerOrder();
			return;
		}
		if (!IsWithinInteractionRange(Worker, Node, InteractionRange))
		{
			if (MoveRefreshRemaining <= 0.0f)
			{
				MoveOwnerTo(Node);
			}
			return;
		}
		Worker->StopMoving();
		GatherFrom(Node);
		if (CargoAmount >= CarryCapacity || Node->GetRemainingAmount() <= 0)
		{
			TargetDeposit = FindClosestDepositBuilding();
			if (!TargetDeposit.IsValid())
			{
				CancelWorkerOrder();
				return;
			}
			WorkerOrderState = EWorkerOrderState::Returning;
			MoveOwnerTo(TargetDeposit.Get());
		}
		return;
	}

	ASurvivalBuilding* Deposit = TargetDeposit.Get();
	if (!Deposit || !Deposit->IsAlive())
	{
		Deposit = FindClosestDepositBuilding();
		TargetDeposit = Deposit;
	}
	if (!Deposit)
	{
		CancelWorkerOrder();
		return;
	}
	if (!IsWithinInteractionRange(Worker, Deposit, InteractionRange))
	{
		if (MoveRefreshRemaining <= 0.0f)
		{
			MoveOwnerTo(Deposit);
		}
		return;
	}
	Worker->StopMoving();
	if (DepositAt(Deposit) && TargetNode.IsValid() && TargetNode->GetRemainingAmount() > 0)
	{
		WorkerOrderState = EWorkerOrderState::Gathering;
		MoveOwnerTo(TargetNode.Get());
	}
	else
	{
		CancelWorkerOrder();
	}
}

ASurvivalBuilding* USurvivalWorkerComponent::FindClosestDepositBuilding() const
{
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	UObject* ControllingAgent = Worker ? Worker->GetControllingAgent().GetObject() : nullptr;
	const AController* Controller = Cast<AController>(ControllingAgent);
	const APlayerState* PlayerState = IsValid(Controller) ? Controller->PlayerState.Get() : Cast<APlayerState>(ControllingAgent);
	ASurvivalBuilding* Result = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ASurvivalBuilding> It(GetWorld()); It; ++It)
	{
		ASurvivalBuilding* Building = *It;
		const ESurvivalBuildingRole Role = Building->GetBuildingRole();
		if (!Building->IsAlive() || !Building->IsConstructionComplete() || Building->GetOwner() != PlayerState ||
			(Role != ESurvivalBuildingRole::MainBase && Role != ESurvivalBuildingRole::Storehouse))
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Worker->GetActorLocation(), Building->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Result = Building;
		}
	}
	return Result;
}

void USurvivalWorkerComponent::MoveOwnerTo(AActor* Target)
{
	ASunriseUnit* Worker = Cast<ASunriseUnit>(GetOwner());
	AAIController* AI = Worker ? Cast<AAIController>(Worker->GetController()) : nullptr;
	if (AI && Target)
	{
		FVector Destination = GetInteractionApproachLocation(Worker, Target, InteractionRange);
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation ProjectedLocation;
			if (Navigation->ProjectPointToNavigation(Destination, ProjectedLocation, FVector(InteractionRange, InteractionRange, 500.0f)))
			{
				Destination = ProjectedLocation.Location;
			}
		}
		AI->MoveToLocation(Destination, 25.0f, true, true, false, true, nullptr, true);
		MoveRefreshRemaining = 1.0f;
	}
}
