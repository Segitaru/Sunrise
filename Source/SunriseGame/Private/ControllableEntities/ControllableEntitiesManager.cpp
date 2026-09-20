#include "ControllableEntities/ControllableEntitiesManager.h"

#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/IControllableEntity.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "ModularPawnData.h"
#include "Net/UnrealNetwork.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

UControllableEntitiesManager::UControllableEntitiesManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UControllableEntitiesManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UControllableEntitiesManager, ControlledEntities);
	DOREPLIFETIME(UControllableEntitiesManager, SelectedEntities);
}

bool UControllableEntitiesManager::CanControlEntity(const AActor* Entity) const
{
	const AController* Controller = Cast<AController>(GetOwner());
	const UControllableComponent* Component = UControllableComponent::FindControllableComponent(Entity);
	return Controller && Component && Component->CanBeControlledBy(Controller);
}

void UControllableEntitiesManager::RegisterControlledEntity(AActor* Entity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !CanControlEntity(Entity))
	{
		return;
	}
	if (!ControlledEntities.Contains(Entity))
	{
		ControlledEntities.Add(Entity);
		OnEntityRegistered.Broadcast(Entity);
	}
}

void UControllableEntitiesManager::UnregisterControlledEntity(AActor* Entity)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (SelectedEntities.Remove(Entity) > 0)
	{
		OnEntityUnselected.Broadcast(Entity);
	}
	if (ControlledEntities.Remove(Entity) > 0)
	{
		OnEntityUnregistered.Broadcast(Entity);
	}
}

void UControllableEntitiesManager::ClearSummonedUnits()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	for (int32 Index = ControlledEntities.Num() - 1; Index >= 0; --Index)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(ControlledEntities[Index]);
		if (Unit && Unit->HasPawnTag(SunrisePawnTags::Kind_Summoned))
		{
			if (SelectedEntities.Remove(Unit) > 0)
			{
				OnEntityUnselected.Broadcast(Unit);
			}
			OnEntityUnregistered.Broadcast(Unit);
			ControlledEntities.RemoveAtSwap(Index);
			Unit->Destroy();
		}
	}
}

void UControllableEntitiesManager::SelectControlledEntity(AActor* Entity)
{
	if (!GetOwner() || !CanControlEntity(Entity))
	{
		return;
	}

	if (!SelectedEntities.Contains(Entity))
	{
		SelectedEntities.Add(Entity);
		OnEntitySelected.Broadcast(Entity);
	}
}

void UControllableEntitiesManager::UnselectControlledEntity(AActor* Entity)
{
	if (!GetOwner())
	{
		return;
	}
	if (SelectedEntities.Remove(Entity) > 0)
	{
		OnEntityUnselected.Broadcast(Entity);
	}
}
TArray<AActor*> UControllableEntitiesManager::GetSelectedEntities() const
{
	return SelectedEntities;
}

TArray<APawn*> UControllableEntitiesManager::SpawnControlledUnitsAtLocations(
	TSoftObjectPtr<UModularPawnData> RequiredEntity, const TArray<FVector>& TargetLocations)
{
	TArray<APawn*> Result;
	AController* Controller = Cast<AController>(GetOwner());
	if (!Controller || !Controller->HasAuthority() || !GetWorld())
	{
		return Result;
	}
	UModularPawnData* Definition = RequiredEntity.LoadSynchronous();
	UClass* UnitClass = Definition ? Definition->PawnClass.LoadSynchronous() : nullptr;
	if (!UnitClass || !Definition->Specification.HasTag(SunrisePawnTags::Kind_Summoned) ||
		Definition->Specification.HasTag(SunrisePawnTags::Kind_Hero))
	{
		return Result;
	}
	TScriptInterface<IIControllableEntity> Agent;
	Agent.SetObject(Controller);
	Agent.SetInterface(Cast<IIControllableEntity>(Controller));
	if (!Agent)
	{
		return Result;
	}

	for (const FVector& Location : TargetLocations)
	{
		ASunriseUnit* Unit = USunriseUnitManagerComponent::SpawnUnit(Definition, FTransform(FRotator::ZeroRotator, Location), Controller);
		if (!Unit)
		{
			continue;
		}
		RegisterControlledEntity(Unit);
		Result.Add(Unit);
	}
	return Result;
}

TArray<AActor*> UControllableEntitiesManager::GetControlledEntities() const
{
	TArray<AActor*> Result;
	for (AActor* Entity : ControlledEntities)
	{
		if (IsValid(Entity))
		{
			Result.Add(Entity);
		}
	}
	return Result;
}
