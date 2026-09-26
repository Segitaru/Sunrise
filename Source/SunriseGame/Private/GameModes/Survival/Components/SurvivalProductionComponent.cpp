#include "GameModes/Survival/Components/SurvivalProductionComponent.h"

#include "Components/CapsuleComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "GameModes/Survival/SurvivalGameplayTags.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "TimerManager.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalProductionComponent)

namespace
{
	ASunriseUnit* SpawnProducedUnit(ASurvivalBuilding* Producer, UModularPawnData* PawnData, AController* Controller)
	{
		if (!Producer || !PawnData || !Controller)
		{
			return nullptr;
		}
		UClass* PawnClass = PawnData->PawnClass.LoadSynchronous();
		const ASunriseUnit* UnitDefaults =
			PawnClass && PawnClass->IsChildOf(ASunriseUnit::StaticClass()) ? PawnClass->GetDefaultObject<ASunriseUnit>() : nullptr;
		const UCapsuleComponent* Capsule = UnitDefaults ? UnitDefaults->GetCapsuleComponent() : nullptr;
		UWorld* World = Producer->GetWorld();
		UNavigationSystemV1* Navigation = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!UnitDefaults || !Capsule || !Navigation)
		{
			return nullptr;
		}

		const FVector ProducerExtent = Producer->GetComponentsBoundingBox(true).GetExtent();
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		const float InitialRadius = FMath::Max(ProducerExtent.GetMax() + CapsuleRadius + 100.0f, 300.0f);
		constexpr int32 SlotsPerRing = 12;
		constexpr int32 RingCount = 3;
		for (int32 Ring = 0; Ring < RingCount; ++Ring)
		{
			const float Radius = InitialRadius + Ring * (CapsuleRadius * 2.0f + 100.0f);
			for (int32 Slot = 0; Slot < SlotsPerRing; ++Slot)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Slot) / static_cast<float>(SlotsPerRing);
				const FVector LocalDirection(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
				const FVector Candidate = Producer->GetActorLocation() + Producer->GetActorRotation().RotateVector(LocalDirection) * Radius;
				FNavLocation NavLocation;
				if (!Navigation->ProjectPointToNavigation(
						Candidate, NavLocation, FVector(CapsuleRadius * 2.0f, CapsuleRadius * 2.0f, 500.0f)))
				{
					continue;
				}
				const FVector SpawnLocation = NavLocation.Location + FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f);
				if (ASunriseUnit* Unit = USunriseUnitManagerComponent::SpawnUnit(
						PawnData, FTransform(Producer->GetActorRotation(), SpawnLocation), Controller))
				{
					return Unit;
				}
			}
		}
		return nullptr;
	}
} // namespace

USurvivalProductionComponent::USurvivalProductionComponent()
{
	SetIsReplicatedByDefault(true);
}

void USurvivalProductionComponent::BeginPlay()
{
	Super::BeginPlay();
	const ASurvivalBuilding* Building = Cast<ASurvivalBuilding>(GetOwner());
	if (Building && Building->IsMainBase() && !FindOption(SurvivalGameplayTags::Unit_Worker))
	{
		const USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
		const TSoftObjectPtr<UModularPawnData> WorkerDefinition =
			Match ? Match->GetStartingWorkerDefinition() : TSoftObjectPtr<UModularPawnData>();
		if (!WorkerDefinition.IsNull())
		{
			FSurvivalProductionOption WorkerOption;
			WorkerOption.UnitId = SurvivalGameplayTags::Unit_Worker;
			WorkerOption.UnitDefinition = WorkerDefinition;
			WorkerOption.Cost.Food = 50.0f;
			WorkerOption.ProductionTime = 5.0f;
			ProductionOptions.Add(WorkerOption);
		}
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
		{
			UnitManager->OnUnitDied.AddUObject(this, &ThisClass::HandleUnitDied);
		}
	}
}

void USurvivalProductionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this))
	{
		UnitManager->OnUnitDied.RemoveAll(this);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RefundQueuedUnits();
	}
	Super::EndPlay(EndPlayReason);
}

void USurvivalProductionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Queue);
	DOREPLIFETIME(ThisClass, CurrentCompletionServerTime);
}

void USurvivalProductionComponent::ServerQueueUnit_Implementation(FGameplayTag UnitId)
{
	ASurvivalBuilding* Barracks = Cast<ASurvivalBuilding>(GetOwner());
	const FSurvivalProductionOption* Option = FindOption(UnitId);
	USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
	APlayerState* PlayerState = Barracks ? Cast<APlayerState>(Barracks->GetOwner()) : nullptr;
	USurvivalEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	const ESurvivalBuildingRole ProducerRole = Barracks ? Barracks->GetBuildingRole() : ESurvivalBuildingRole::Generic;
	if (!Barracks || !Barracks->HasAuthority() || !Barracks->IsAlive() || !Barracks->IsConstructionComplete() ||
		(ProducerRole != ESurvivalBuildingRole::Barracks && ProducerRole != ESurvivalBuildingRole::MainBase) || !Option ||
		Option->UnitDefinition.IsNull() || Queue.Num() >= MaxQueueSize || !Match ||
		Match->GetSurvivalMatchState() != ESurvivalMatchState::InProgress || !Economy)
	{
		return;
	}
	if (!Economy->TryReservePopulation(FMath::Max(1, Option->PopulationCost)))
	{
		return;
	}
	if (!Economy->TrySpend(Option->Cost))
	{
		Economy->ReleasePopulation(FMath::Max(1, Option->PopulationCost));
		return;
	}

	Queue.Add(UnitId);
	OnRep_Queue();
	if (Queue.Num() == 1)
	{
		StartCurrentProduction();
	}
}

const FSurvivalProductionOption* USurvivalProductionComponent::FindOption(FGameplayTag UnitId) const
{
	return ProductionOptions.FindByPredicate(
		[UnitId](const FSurvivalProductionOption& Option)
		{
			return Option.UnitId.IsValid() && Option.UnitId == UnitId;
		});
}

void USurvivalProductionComponent::StartCurrentProduction()
{
	const FSurvivalProductionOption* Option = Queue.IsEmpty() ? nullptr : FindOption(Queue[0]);
	UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!Option || !World || !GameState)
	{
		RefundQueuedUnits();
		return;
	}
	const float Delay = FMath::Max(0.1f, Option->ProductionTime);
	CurrentCompletionServerTime = GameState->GetServerWorldTimeSeconds() + Delay;
	World->GetTimerManager().SetTimer(ProductionTimer, this, &ThisClass::FinishCurrentProduction, Delay, false);
	OnRep_Queue();
}

void USurvivalProductionComponent::FinishCurrentProduction()
{
	ASurvivalBuilding* Barracks = Cast<ASurvivalBuilding>(GetOwner());
	const FSurvivalProductionOption* Option = Queue.IsEmpty() ? nullptr : FindOption(Queue[0]);
	APlayerState* PlayerState = Barracks ? Cast<APlayerState>(Barracks->GetOwner()) : nullptr;
	AController* Controller = PlayerState ? Cast<AController>(PlayerState->GetOwner()) : nullptr;
	USurvivalEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	USunriseUnitManagerComponent* UnitManager = USunriseUnitManagerComponent::Find(this);
	UModularPawnData* PawnData = Option ? Option->UnitDefinition.LoadSynchronous() : nullptr;
	ASunriseUnit* Unit = nullptr;
	if (Barracks && Barracks->IsAlive() && Option && Controller && UnitManager && PawnData)
	{
		Unit = SpawnProducedUnit(Barracks, PawnData, Controller);
	}
	if (Unit && Option)
	{
		ProducedPopulation.Add(Unit, FMath::Max(1, Option->PopulationCost));
		if (UControllableComponent* Controllable = UControllableComponent::FindControllableComponent(Unit))
		{
			Controllable->SetPlayerControllable(true);
		}
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
		{
			Manager->RegisterControlledEntity(Unit);
		}
		if (Option->UnitId == SurvivalGameplayTags::Unit_Worker)
		{
			if (!Unit->FindComponentByClass<USurvivalWorkerComponent>())
			{
				USurvivalWorkerComponent* WorkerComponent = NewObject<USurvivalWorkerComponent>(Unit, TEXT("SurvivalWorker"));
				Unit->AddInstanceComponent(WorkerComponent);
				WorkerComponent->RegisterComponent();
			}
			if (USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this))
			{
				Match->RegisterProducedWorker(Unit);
			}
		}
	}
	else if (Economy && Option)
	{
		Economy->AddResources(Option->Cost);
		Economy->ReleasePopulation(FMath::Max(1, Option->PopulationCost));
	}

	if (!Queue.IsEmpty())
	{
		Queue.RemoveAt(0);
	}
	CurrentCompletionServerTime = -1.0f;
	OnRep_Queue();
	if (!Queue.IsEmpty())
	{
		StartCurrentProduction();
	}
}

void USurvivalProductionComponent::HandleUnitDied(ASunriseUnit* Unit)
{
	if (const int32* PopulationCost = ProducedPopulation.Find(Unit))
	{
		if (ASurvivalBuilding* Barracks = Cast<ASurvivalBuilding>(GetOwner()))
		{
			if (APlayerState* PlayerState = Cast<APlayerState>(Barracks->GetOwner()))
			{
				if (USurvivalEconomyComponent* Economy = PlayerState->FindComponentByClass<USurvivalEconomyComponent>())
				{
					Economy->ReleasePopulation(*PopulationCost);
				}
			}
		}
		ProducedPopulation.Remove(Unit);
	}
}

void USurvivalProductionComponent::RefundQueuedUnits()
{
	ASurvivalBuilding* Barracks = Cast<ASurvivalBuilding>(GetOwner());
	APlayerState* PlayerState = Barracks ? Cast<APlayerState>(Barracks->GetOwner()) : nullptr;
	USurvivalEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	if (Economy)
	{
		for (const FGameplayTag UnitId : Queue)
		{
			if (const FSurvivalProductionOption* Option = FindOption(UnitId))
			{
				Economy->AddResources(Option->Cost);
				Economy->ReleasePopulation(FMath::Max(1, Option->PopulationCost));
			}
		}
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProductionTimer);
	}
	Queue.Reset();
	CurrentCompletionServerTime = -1.0f;
	OnRep_Queue();
}

void USurvivalProductionComponent::OnRep_Queue()
{
	OnProductionChanged.Broadcast();
}
