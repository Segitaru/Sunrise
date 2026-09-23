#include "GameModes/Survival/Components/SurvivalProductionComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "TimerManager.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalProductionComponent)

USurvivalProductionComponent::USurvivalProductionComponent()
{
	SetIsReplicatedByDefault(true);
}

void USurvivalProductionComponent::BeginPlay()
{
	Super::BeginPlay();
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
	if (!Barracks || !Barracks->HasAuthority() || !Barracks->IsAlive() || Barracks->GetBuildingRole() != ESurvivalBuildingRole::Barracks ||
		!Option || Option->UnitDefinition.IsNull() || Queue.Num() >= MaxQueueSize || !Match ||
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
		Unit = USunriseUnitManagerComponent::SpawnUnit(
			PawnData, FTransform(Barracks->GetActorRotation(), Barracks->GetActorTransform().TransformPosition(SpawnOffset)), Controller);
	}
	if (Unit && Option)
	{
		ProducedPopulation.Add(Unit, FMath::Max(1, Option->PopulationCost));
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
