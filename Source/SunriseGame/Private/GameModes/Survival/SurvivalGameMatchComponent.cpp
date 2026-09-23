#include "GameModes/Survival/SurvivalGameMatchComponent.h"

#include <Engine/DamageEvents.h>

#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "EngineUtils.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFeatures/ExperienceDefinition.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalBuildComponent.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"
#include "GameModes/Survival/SurvivalGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"
#include "Player/SunrisePlayerController.h"
#include "Teams/System/ModularTeamAgentInterface.h"
#include "TimerManager.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalGameMatchComponent)

USurvivalGameMatchComponent::USurvivalGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	MainBaseClass = ASurvivalMainBase::StaticClass();
}

void USurvivalGameMatchComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (USunriseUnitManagerComponent* UnitManager = GetUnitManager())
	{
		UnitManager->OnUnitDied.AddUObject(this, &ThisClass::HandleUnitDied);
	}
	UExperienceManagerComponent* ExperienceManager = GetOwner()->FindComponentByClass<UExperienceManagerComponent>();
	if (ensureMsgf(ExperienceManager, TEXT("SurvivalGameMatchComponent requires ExperienceManagerComponent")))
	{
		ExperienceManager->CallOrRegister_OnExperienceLoaded_LowPriority(
			FOnExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
	}
}

void USurvivalGameMatchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USunriseUnitManagerComponent* UnitManager = GetUnitManager())
	{
		UnitManager->OnUnitDied.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void USurvivalGameMatchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, CurrentWave);
	DOREPLIFETIME(ThisClass, NextWaveServerTime);
	DOREPLIFETIME(ThisClass, MainBases);
}

USurvivalGameMatchComponent* USurvivalGameMatchComponent::Find(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<USurvivalGameMatchComponent>() : nullptr;
}

void USurvivalGameMatchComponent::RegisterBuilding(ASurvivalBuilding* Building)
{
	if (HasAuthority() && IsValid(Building) && Building->IsMainBase())
	{
		MainBases.AddUnique(Building);
	}
}

void USurvivalGameMatchComponent::UnregisterBuilding(ASurvivalBuilding* Building)
{
	if (HasAuthority())
	{
		MainBases.Remove(Building);
		EvaluateDefeatCondition();
	}
}

float USurvivalGameMatchComponent::GetSecondsUntilNextWave() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GameState && NextWaveServerTime >= 0.0f ? FMath::Max(0.0f, NextWaveServerTime - GameState->GetServerWorldTimeSeconds()) : -1.0f;
}

int32 USurvivalGameMatchComponent::GetAliveMainBaseCount() const
{
	int32 Count = 0;
	for (const ASurvivalBuilding* Base : MainBases)
	{
		Count += IsValid(Base) && Base->IsAlive() && Base->GetHealth() > 0.0f && Base->IsConstructionComplete() ? 1 : 0;
	}
	return Count;
}

int32 USurvivalGameMatchComponent::GetAliveWorkerCount() const
{
	int32 Count = 0;
	for (const ASunriseUnit* Worker : Workers)
	{
		Count += IsValid(Worker) && Worker->IsAlive() ? 1 : 0;
	}
	return Count;
}

int32 USurvivalGameMatchComponent::GetAliveWaveEnemyCount() const
{
	int32 Count = 0;
	for (const ASunriseUnit* Unit : ActiveWaveUnits)
	{
		Count += IsValid(Unit) && Unit->IsAlive() ? 1 : 0;
	}
	return Count;
}

void USurvivalGameMatchComponent::HandleExperienceLoaded(const UExperienceDefinition* CurrentExperience)
{
	if (IsValid(CurrentExperience) && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(InitializationTimer, this, &ThisClass::InitializeMode, 0.25f, false);
	}
}

void USurvivalGameMatchComponent::InitializeMode()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || GameState->PlayerArray.IsEmpty())
	{
		GetWorld()->GetTimerManager().SetTimer(InitializationTimer, this, &ThisClass::InitializeMode, 0.25f, false);
		return;
	}

	ResolvedEnemyTeamId = ResolveEnemyTeamId();
	int32 PlayerIndex = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (IsValid(PlayerState) && !PlayerState->IsOnlyASpectator() && !PlayerState->IsInactive())
		{
			InitializePlayer(PlayerState, PlayerIndex++);
		}
	}
	if (PlayerIndex == 0)
	{
		GetWorld()->GetTimerManager().SetTimer(InitializationTimer, this, &ThisClass::InitializeMode, 0.25f, false);
		return;
	}

	bPlayersInitialized = true;
	SetMatchState(ESurvivalMatchState::InProgress);
	GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::UpdateMatch, UpdateInterval, true);
	if (!Waves.IsEmpty())
	{
		NextWaveServerTime = GameState->GetServerWorldTimeSeconds() + FMath::Max(0.0f, Waves[0].Delay);
	}
}

void USurvivalGameMatchComponent::InitializePlayer(APlayerState* PlayerState, int32 PlayerIndex)
{
	USurvivalEconomyComponent* Economy = PlayerState->FindComponentByClass<USurvivalEconomyComponent>();
	if (!Economy)
	{
		Economy = NewObject<USurvivalEconomyComponent>(PlayerState, TEXT("SurvivalEconomy"));
		PlayerState->AddInstanceComponent(Economy);
		Economy->RegisterComponent();
	}
	if (!PlayerState->FindComponentByClass<USurvivalBuildComponent>())
	{
		USurvivalBuildComponent* BuildComponent = NewObject<USurvivalBuildComponent>(PlayerState, TEXT("SurvivalBuild"));
		PlayerState->AddInstanceComponent(BuildComponent);
		BuildComponent->RegisterComponent();
	}

	AController* Controller = Cast<AController>(PlayerState->GetOwner());
	APawn* PlayerPawn = Controller ? Controller->GetPawn() : nullptr;
	const FVector Origin = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector(PlayerIndex * 1600.0f, 0.0f, 100.0f);
	const FRotator Rotation = PlayerPawn ? PlayerPawn->GetActorRotation() : FRotator::ZeroRotator;
	const FVector BaseLocation = Origin + Rotation.RotateVector(MainBaseOffset);
	FActorSpawnParameters Parameters;
	Parameters.Owner = PlayerState;
	Parameters.Instigator = PlayerPawn;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ASurvivalBuilding* Base = GetWorld()->SpawnActor<ASurvivalBuilding>(MainBaseClass, BaseLocation, Rotation, Parameters);
	if (Base)
	{
		if (const IModularTeamAgentInterface* const TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState))
		{
			IModularTeamAgentInterface* const BaseTeamAgent = Cast<IModularTeamAgentInterface>(Base);
			BaseTeamAgent->SetGenericTeamId(TeamAgent->GetGenericTeamId());
		}
		RegisterBuilding(Base);
	}

	UModularPawnData* WorkerData = StartingWorkerDefinition.LoadSynchronous();
	USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	if (!WorkerData || !UnitManager || !Controller)
	{
		return;
	}
	for (int32 Index = 0; Index < FMath::Max(1, StartingWorkerCount); ++Index)
	{
		const float Angle = UE_TWO_PI * Index / FMath::Max(1, StartingWorkerCount);
		const FVector WorkerLocation = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * 240.0f;
		ASunriseUnit* Worker = USunriseUnitManagerComponent::SpawnUnit(WorkerData, FTransform(Rotation, WorkerLocation), Controller);
		if (!Worker)
		{
			continue;
		}
		if (!Worker->FindComponentByClass<USurvivalWorkerComponent>())
		{
			USurvivalWorkerComponent* WorkerComponent = NewObject<USurvivalWorkerComponent>(Worker, TEXT("SurvivalWorker"));
			Worker->AddInstanceComponent(WorkerComponent);
			WorkerComponent->RegisterComponent();
		}
		if (UControllableComponent* Controllable = UControllableComponent::FindControllableComponent(Worker))
		{
			Controllable->SetPlayerControllable(true);
		}
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
		{
			Manager->RegisterControlledEntity(Worker);
		}
		Workers.Add(Worker);
		StartingPopulationWorkers.Add(Worker);
		Economy->TryReservePopulation(1);
	}
}

void USurvivalGameMatchComponent::StartNextWave()
{
	if (!Waves.IsValidIndex(CurrentWave) || MatchState != ESurvivalMatchState::InProgress)
	{
		return;
	}
	ActiveWaveUnits.Reset();
	FVector SpawnOrigin = FVector::ZeroVector;
	bool bFoundSpawn = false;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(WaveSpawnActorTag))
		{
			SpawnOrigin = It->GetActorLocation();
			bFoundSpawn = true;
			break;
		}
	}
	if (!bFoundSpawn)
	{
		if (ASurvivalBuilding* Base = FindClosestLivingBase(FVector::ZeroVector))
		{
			SpawnOrigin = Base->GetActorLocation() + FVector(FallbackWaveSpawnDistance, 0.0f, 0.0f);
		}
	}

	const FSurvivalWaveDefinition& Wave = Waves[CurrentWave];
	for (const FSurvivalWaveEntry& Entry : Wave.Entries)
	{
		UModularPawnData* PawnData = Entry.UnitDefinition.LoadSynchronous();
		if (!PawnData)
		{
			continue;
		}
		for (int32 Index = 0; Index < FMath::Max(1, Entry.Count); ++Index)
		{
			const float Angle = UE_TWO_PI * Index / FMath::Max(1, Entry.Count);
			const FVector Location = SpawnOrigin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Entry.SpawnRadius;
			ASunriseUnit* Unit = USunriseUnitManagerComponent::SpawnUnit(PawnData, FTransform(Location), GetOwner());
			if (Unit)
			{
				Unit->SetTeamId(ResolvedEnemyTeamId);
				if (ASurvivalBuilding* Base = FindClosestLivingBase(Location))
				{
					Unit->IssueAutonomousMoveOrder(Base->GetActorLocation());
				}
				ActiveWaveUnits.Add(Unit);
			}
		}
	}
	++CurrentWave;
	NextWaveServerTime = -1.0f;
}

void USurvivalGameMatchComponent::UpdateMatch()
{
	if (!bPlayersInitialized || MatchState != ESurvivalMatchState::InProgress)
	{
		return;
	}
	EvaluateDefeatCondition();
	if (MatchState != ESurvivalMatchState::InProgress)
	{
		return;
	}

	for (ASunriseUnit* Enemy : ActiveWaveUnits)
	{
		if (!IsValid(Enemy) || !Enemy->IsAlive())
		{
			continue;
		}
		ASurvivalBuilding* Base = FindClosestLivingBase(Enemy->GetActorLocation());
		if (!Base)
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), Base->GetActorLocation());
		if (DistanceSquared <= FMath::Square(RaidDamageRange))
		{
			Base->TakeDamage(RaidDamagePerSecond * UpdateInterval, FDamageEvent(), Enemy->GetController(), Enemy);
		}
		else if (!Enemy->HasActivePlayerOrder())
		{
			Enemy->IssueAutonomousMoveOrder(Base->GetActorLocation());
		}
	}

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (CurrentWave == 0 && !Waves.IsEmpty() && GameState && GameState->GetServerWorldTimeSeconds() >= NextWaveServerTime)
	{
		StartNextWave();
		return;
	}
	if (CurrentWave > 0 && GetAliveWaveEnemyCount() == 0)
	{
		if (CurrentWave >= Waves.Num())
		{
			SetMatchState(ESurvivalMatchState::Victory);
		}
		else if (NextWaveServerTime < 0.0f && GameState)
		{
			NextWaveServerTime = GameState->GetServerWorldTimeSeconds() + FMath::Max(0.0f, Waves[CurrentWave].Delay);
		}
		else if (GameState && GameState->GetServerWorldTimeSeconds() >= NextWaveServerTime)
		{
			StartNextWave();
		}
	}
}

void USurvivalGameMatchComponent::SetMatchState(ESurvivalMatchState NewState)
{
	if (!HasAuthority() || MatchState == NewState)
	{
		return;
	}
	MatchState = NewState;
	OnRep_MatchState();
	if (NewState == ESurvivalMatchState::Victory || NewState == ESurvivalMatchState::Defeat)
	{
		NextWaveServerTime = -1.0f;
		GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
		if (const AGameStateBase* GameState = GetWorld()->GetGameState())
		{
			for (APlayerState* PlayerState : GameState->PlayerArray)
			{
				if (ASunrisePlayerController* Controller = PlayerState ? Cast<ASunrisePlayerController>(PlayerState->GetOwner()) : nullptr)
				{
					Controller->SetCommandsEnabled(false);
				}
			}
		}
	}
}

void USurvivalGameMatchComponent::HandleUnitDied(ASunriseUnit* Unit)
{
	if (StartingPopulationWorkers.Remove(TWeakObjectPtr<ASunriseUnit>(Unit)) > 0)
	{
		if (USurvivalEconomyComponent* Economy = USurvivalEconomyComponent::Find(Unit))
		{
			Economy->ReleasePopulation(1);
		}
	}
	EvaluateDefeatCondition();
}

void USurvivalGameMatchComponent::RegisterProducedWorker(ASunriseUnit* Worker)
{
	if (HasAuthority() && IsValid(Worker))
	{
		Workers.AddUnique(Worker);
	}
}

void USurvivalGameMatchComponent::EvaluateDefeatCondition()
{
	if (MatchState == ESurvivalMatchState::InProgress && GetAliveMainBaseCount() == 0 && !HasRecoveryPath())
	{
		SetMatchState(ESurvivalMatchState::Defeat);
	}
}

ASurvivalBuilding* USurvivalGameMatchComponent::GetClosestLivingMainBase(const FVector& Location) const
{
	return FindClosestLivingBase(Location);
}

int32 USurvivalGameMatchComponent::ResolveEnemyTeamId() const
{
	TSet<int32> PlayerTeams;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (GameState)
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState))
			{
				PlayerTeams.Add(GenericTeamIdToInteger(TeamAgent->GetGenericTeamId()));
			}
		}
	}
	int32 Candidate = FMath::Clamp(EnemyTeamId, 0, 254);
	while (Candidate < 254 && PlayerTeams.Contains(Candidate))
	{
		++Candidate;
	}
	if (PlayerTeams.Contains(Candidate))
	{
		for (Candidate = 0; Candidate < 255 && PlayerTeams.Contains(Candidate); ++Candidate)
		{
		}
	}
	return Candidate < 255 ? Candidate : 254;
}

ASurvivalBuilding* USurvivalGameMatchComponent::FindClosestLivingBase(const FVector& Location) const
{
	ASurvivalBuilding* Result = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (ASurvivalBuilding* Base : MainBases)
	{
		if (!IsValid(Base) || !Base->IsAlive() || Base->GetHealth() <= 0.0f)
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Location, Base->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Result = Base;
		}
	}
	return Result;
}

bool USurvivalGameMatchComponent::HasRecoveryPath() const
{
	const ASurvivalBuilding* BaseDefaults = MainBaseClass ? MainBaseClass->GetDefaultObject<ASurvivalBuilding>() : nullptr;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!BaseDefaults || !GameState)
	{
		return false;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState);
		const USurvivalEconomyComponent* Economy = PlayerState ? PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
		const bool bHasWorker = TeamAgent && HasLivingWorkerForTeam(GenericTeamIdToInteger(TeamAgent->GetGenericTeamId()));
		if (!bHasWorker)
		{
			continue;
		}
		for (const ASurvivalBuilding* Base : MainBases)
		{
			if (IsValid(Base) && Base->GetOwner() == PlayerState && Base->IsAlive() && !Base->IsConstructionComplete())
			{
				return true;
			}
		}
		if (Economy && Economy->CanAfford(BaseDefaults->GetConstructionCost()))
		{
			return true;
		}
	}
	return false;
}

bool USurvivalGameMatchComponent::HasLivingWorkerForTeam(int32 TeamId) const
{
	for (const ASunriseUnit* Worker : Workers)
	{
		if (IsValid(Worker) && Worker->IsAlive() && Worker->GetTeamId() == TeamId)
		{
			return true;
		}
	}
	if (const USunriseUnitManagerComponent* UnitManager = GetUnitManager())
	{
		for (const ASunriseUnit* Unit : UnitManager->GetUnits())
		{
			if (IsValid(Unit) && Unit->IsAlive() && Unit->GetTeamId() == TeamId &&
				(Unit->FindComponentByClass<USurvivalWorkerComponent>() || Unit->HasPawnTag(SurvivalGameplayTags::Unit_Worker)))
			{
				return true;
			}
		}
	}
	return false;
}

USunriseUnitManagerComponent* USurvivalGameMatchComponent::GetUnitManager() const
{
	return USunriseUnitManagerComponent::Find(this);
}

void USurvivalGameMatchComponent::OnRep_MatchState()
{
	OnMatchStateChanged.Broadcast(MatchState);
}
