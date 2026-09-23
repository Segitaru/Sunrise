#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"

#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "Net/UnrealNetwork.h"
#include "Resources/SunriseResourceNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalWorkerComponent)

USurvivalWorkerComponent::USurvivalWorkerComponent()
{
	SetIsReplicatedByDefault(true);
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
	if (!Worker || !Worker->HasAuthority() || !IsValid(Node) ||
		FVector::DistSquared(Worker->GetActorLocation(), Node->GetActorLocation()) > FMath::Square(InteractionRange) ||
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
	if (!Worker || !Worker->HasAuthority() || !IsValid(Building) || CargoAmount <= 0 ||
		FVector::DistSquared(Worker->GetActorLocation(), Building->GetActorLocation()) > FMath::Square(InteractionRange) ||
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
