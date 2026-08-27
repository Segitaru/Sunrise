#include "ControllableEntities/ControllableEntitiesManager.h"

#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/Data/ControllableEntityDefinition.h"
#include "ControllableEntities/IControllableEntity.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "System/TFTeamSubsystem.h"
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
		if (Unit && Unit->GetUnitKind() == ESunriseUnitKind::Summoned)
		{
			OnEntityUnregistered.Broadcast(Unit);
			ControlledEntities.RemoveAtSwap(Index);
			Unit->Destroy();
		}
	}
}
TArray<APawn*> UControllableEntitiesManager::SpawnControlledUnitsAtLocations(
	TSoftObjectPtr<UControllableEntityDefinition> RequiredEntity, const TArray<FVector>& TargetLocations)
{
	TArray<APawn*> Result;
	AController* Controller = Cast<AController>(GetOwner());
	if (!Controller || !Controller->HasAuthority() || !GetWorld())
	{
		return Result;
	}
	UControllableEntityDefinition* Definition = RequiredEntity.LoadSynchronous();
	UClass* UnitClass = Definition ? Definition->UnitClass.LoadSynchronous() : nullptr;
	if (!UnitClass)
	{
		return Result;
	}
	UTFTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UTFTeamSubsystem>();
	const int32 TeamId = TeamSubsystem ? TeamSubsystem->FindTeamFromObject(Controller) : INDEX_NONE;
	TScriptInterface<IIControllableEntity> Agent;
	Agent.SetObject(Controller);
	Agent.SetInterface(Cast<IIControllableEntity>(Controller));
	if (!Agent)
	{
		return Result;
	}

	for (const FVector& Location : TargetLocations)
	{
		FActorSpawnParameters Params;
		Params.Owner = Controller;
		Params.Instigator = Controller->GetPawn();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ASunriseUnit* Unit = GetWorld()->SpawnActor<ASunriseUnit>(UnitClass, Location, FRotator::ZeroRotator, Params);
		if (!Unit)
		{
			continue;
		}
		Unit->SpawnDefaultController();
		Unit->SetTeamId(TeamId);
		Unit->ConfigureControl(ESunriseUnitKind::Summoned, Agent);
		if (UControllableComponent* Component = UControllableComponent::FindControllableComponent(Unit))
		{
			Component->SetEntityDefinition(Definition);
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
