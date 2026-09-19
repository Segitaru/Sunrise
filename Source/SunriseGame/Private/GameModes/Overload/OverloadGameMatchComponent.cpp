// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/OverloadGameMatchComponent.h"

#include <Abilities/SunriseHeroSquadAbility.h>

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFeatures/ExperienceDefinition.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Overload/Actors/OverloadEnergyCore.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"
#include "GameModes/Overload/Types/OverloadTeamIds.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SunrisePlayerController.h"
#include "System/SunriseGameInstance.h"
#include "TimerManager.h"
#include "UI/SunriseWidgets.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

DEFINE_LOG_CATEGORY_STATIC(LogOverloadHeroSpawn, Log, All);

UOverloadGameMatchComponent::UOverloadGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TowerClass = AOverloadGuardTower::StaticClass();
	CoreClass = AOverloadEnergyCore::StaticClass();
}

void UOverloadGameMatchComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	UExperienceManagerComponent* ExperienceManager = GetOwner()->FindComponentByClass<UExperienceManagerComponent>();
	if (ensureMsgf(ExperienceManager, TEXT("OverloadGameMatchComponent requires EFExperienceManagerComponent")))
	{
		ExperienceManager->CallOrRegister_OnExperienceLoaded_LowPriority(
			FOnExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
	}
}

void UOverloadGameMatchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Lanes);
	DOREPLIFETIME(ThisClass, Towers);
	DOREPLIFETIME(ThisClass, CoreActors);
	DOREPLIFETIME(ThisClass, bOverloadInitialized);
	DOREPLIFETIME(ThisClass, WinnerTeamId);
}

UOverloadGameMatchComponent* UOverloadGameMatchComponent::Find(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<UOverloadGameMatchComponent>() : nullptr;
}

int32 UOverloadGameMatchComponent::GetAliveUnitCountForTeam(int32 TeamId) const
{
	const USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	return UnitManager ? UnitManager->GetAliveUnitCountForTeam(TeamId) : 0;
}

AOverloadEnergyCore* UOverloadGameMatchComponent::GetCoreForTeam(int32 TeamId) const
{
	const TObjectPtr<AOverloadEnergyCore>* Found = CoresByTeam.Find(TeamId);
	return Found ? Found->Get() : nullptr;
}

ASunriseUnit* UOverloadGameMatchComponent::GetLivingHeroForTeam(int32 TeamId) const
{
	USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	return UnitManager ? UnitManager->GetLivingHeroForTeam(TeamId) : nullptr;
}

float UOverloadGameMatchComponent::GetHeroRespawnSeconds(int32 TeamId) const
{
	const USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	return UnitManager ? UnitManager->GetHeroRespawnSeconds(TeamId) : -1.0f;
}

ESunriseMatchResult UOverloadGameMatchComponent::GetMatchResult() const
{
	if (WinnerTeamId == INDEX_NONE)
	{
		return ESunriseMatchResult::InProgress;
	}
	const ASunrisePlayerController* Controller = Cast<ASunrisePlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	const int32 PlayerTeamId = Controller ? Controller->GetControlledTeamId() : OverloadTeamIds::InitialPlayer;
	return WinnerTeamId == PlayerTeamId ? ESunriseMatchResult::Victory : ESunriseMatchResult::Defeat;
}

void UOverloadGameMatchComponent::HandleExperienceLoaded(const UExperienceDefinition* CurrentExperience)
{
	if (!IsValid(CurrentExperience) || !GetWorld())
	{
		return;
	}
	GetWorldTimerManager().SetTimer(
		InitializationTimer, this, &UOverloadGameMatchComponent::InitializeOverloadMode, InitializationRetryDelay, false);
}

void UOverloadGameMatchComponent::InitializeOverloadMode()
{
	if (bOverloadInitialized)
	{
		return;
	}
	++InitializationAttempts;
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(this))
	{
		if (InitializationAttempts == 1 || InitializationAttempts % 20 == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overload initialization is waiting for navigation (attempt %d)"), InitializationAttempts);
		}
		GetWorld()->GetTimerManager().SetTimer(
			InitializationTimer, this, &UOverloadGameMatchComponent::InitializeOverloadMode, InitializationRetryDelay, false);
		return;
	}

	for (TActorIterator<AOverloadLaneSpline> It(GetWorld()); It; ++It)
	{
		if (!OverloadTeamIds::IsPlayable(It->GetSourceTeamId()) || !OverloadTeamIds::IsPlayable(It->GetTargetTeamId()) ||
			It->GetSourceTeamId() == It->GetTargetTeamId())
		{
			UE_LOG(LogTemp, Error, TEXT("Overload lane %s has invalid team mapping %d -> %d"), *It->GetName(), It->GetSourceTeamId(),
				It->GetTargetTeamId());
			continue;
		}
		Lanes.Add(*It);
		LaneView.Add(*It);
	}
	if (Lanes.IsEmpty())
	{
		if (InitializationAttempts == 1 || InitializationAttempts % 20 == 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Overload is waiting for at least one valid AOverloadLaneSpline in the current level (attempt %d)"),
				InitializationAttempts);
		}
		GetWorld()->GetTimerManager().SetTimer(
			InitializationTimer, this, &UOverloadGameMatchComponent::InitializeOverloadMode, InitializationRetryDelay, false);
		return;
	}

	for (AOverloadLaneSpline* Lane : Lanes)
	{
		USplineComponent* Spline = Lane->GetLaneSpline();
		const float Length = Spline->GetSplineLength();
		EnsureCore(Lane->GetSourceTeamId(), Spline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World),
			Spline->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
		EnsureCore(Lane->GetTargetTeamId(), Spline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World),
			Spline->GetRotationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World));
		BuildObjectivesForLane(Lane);
		if (UOverloadWaveSpawnerComponent* WaveSpawner = Lane->GetOrCreateWaveSpawner())
		{
			WaveSpawner->ApplyEnemyDifficulty(
				GetWorld()->GetGameInstance<USunriseGameInstance>()
					? GetWorld()->GetGameInstance<USunriseGameInstance>()->GetDifficultyTuning().EnemyCountMultiplier
					: 1.0f);
			WaveSpawner->Initialize(Lane, WaveFormations);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Overload lane %s has no wave spawner and cannot create units"), *GetNameSafe(Lane));
		}
	}

	EnsurePlayerHeroes();

	// Authored units can also interact with objectives; dynamically spawned wave units receive the same component.
	for (TActorIterator<ASunriseUnit> It(GetWorld()); It; ++It)
	{
		if (!It->FindComponentByClass<UOverloadInteractorComponent>())
		{
			UOverloadInteractorComponent* Interactor = NewObject<UOverloadInteractorComponent>(*It, TEXT("OverloadInteractor"));
			Interactor->RegisterComponent();
			Interactor->InitializeForUnit();
		}
	}

	bOverloadInitialized = true;
	GetWorld()->GetTimerManager().SetTimer(PlayerHeroInitializationTimer, this, &ThisClass::EnsurePlayerHeroes, 0.5f, true);
	RecalculateSupplyAndBalance();
	UE_LOG(LogTemp, Log, TEXT("Overload initialized: %d teams, %d lanes, %d towers"), CoresByTeam.Num(), Lanes.Num(), Towers.Num());
}

void UOverloadGameMatchComponent::EnsurePlayerHeroes()
{
	USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!HasAuthority() || !GameState || !UnitManager || WinnerTeamId != INDEX_NONE)
	{
		return;
	}
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AController* Controller = IsValid(PlayerState) ? Cast<AController>(PlayerState->GetOwner()) : nullptr;
		const IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState);
		if (!IsValid(Controller) || Controller->PlayerState != PlayerState || !TeamAgent || PlayerState->IsInactive() ||
			PlayerState->IsOnlyASpectator())
		{
			continue;
		}
		const int32 TeamId = GenericTeamIdToInteger(TeamAgent->GetGenericTeamId());
		if (!OverloadTeamIds::IsPlayable(TeamId))
		{
			continue;
		}
		AOverloadEnergyCore* Core = GetCoreForTeam(TeamId);
		if (!IsValid(Core) || Core->GetCoreState() == EOverloadCoreState::Destroyed)
		{
			continue;
		}
		ASunriseUnit* Hero = UnitManager->GetHeroForPlayer(Controller);
		if (!Hero)
		{
			FTransform SpawnTransform;
			if (!TryResolveHeroSpawnTransform(Core, TeamId, SpawnTransform))
			{
				UE_LOG(LogOverloadHeroSpawn, Verbose, TEXT("Waiting for ground near core: PlayerState=%s Team=%d Core=%s Location=%s"),
					*GetNameSafe(PlayerState), TeamId, *GetNameSafe(Core), *Core->GetActorLocation().ToString());
				continue;
			}
			Hero = UnitManager->SpawnHeroForPlayer(Controller, SpawnTransform);
			if (Hero)
			{
				UE_LOG(LogOverloadHeroSpawn, Log, TEXT("Hero placed: Hero=%s Team=%d Core=%s CoreLocation=%s HeroLocation=%s"),
					*GetNameSafe(Hero), TeamId, *GetNameSafe(Core), *Core->GetActorLocation().ToString(),
					*Hero->GetActorLocation().ToString());
			}
		}
		if (!Hero)
		{
			continue; // Selection or its PawnData may arrive after Experience/initial login.
		}
		if (!Hero->FindComponentByClass<UOverloadInteractorComponent>())
		{
			UOverloadInteractorComponent* Interactor = NewObject<UOverloadInteractorComponent>(Hero, TEXT("OverloadInteractor"));
			Interactor->RegisterComponent();
			Interactor->InitializeForUnit();
		}
	}
}

void UOverloadGameMatchComponent::HandleTowerCaptured(AOverloadGuardTower* Tower, int32 PreviousTeamId, int32 NewTeamId)
{
	UE_LOG(LogTemp, Log, TEXT("Overload tower %s captured: team %d -> %d"), *GetNameSafe(Tower), PreviousTeamId, NewTeamId);
	RecalculateSupplyAndBalance();
}

void UOverloadGameMatchComponent::HandleCoreExploded(AOverloadEnergyCore* Core, int32 OverloadingTeamId)
{
	if (!Core)
	{
		return;
	}
	OnTeamEliminated.Broadcast(Core->GetOriginalTeamId(), OverloadingTeamId);
	int32 RemainingTeam = INDEX_NONE;
	int32 RemainingCount = 0;
	for (const TPair<int32, TObjectPtr<AOverloadEnergyCore>>& Pair : CoresByTeam)
	{
		if (IsValid(Pair.Value) && Pair.Value->GetCoreState() != EOverloadCoreState::Destroyed)
		{
			RemainingTeam = Pair.Key;
			++RemainingCount;
		}
	}
	if (RemainingCount == 1)
	{
		WinnerTeamId = RemainingTeam;
		OnWinnerDetermined.Broadcast(RemainingTeam);
		PresentMatchResult();
	}
}

void UOverloadGameMatchComponent::BuildObjectivesForLane(AOverloadLaneSpline* Lane)
{
	TArray<AOverloadGuardTower*> LaneTowers;
	USplineComponent* Spline = Lane->GetLaneSpline();
	const int32 CheckpointCount = Lane->GetCheckpointCount();
	const int32 HalfCount = CheckpointCount / 2;
	const bool bHasNeutralCenter = CheckpointCount % 2 != 0;
	const int32 CenterIndex = HalfCount;
	for (int32 Index = 0; Index < CheckpointCount; ++Index)
	{
		const bool bNeutralTower = bHasNeutralCenter && Index == CenterIndex;
		const int32 InitialTeamId = bNeutralTower		? OverloadTeamIds::Neutral
									: Index < HalfCount ? Lane->GetSourceTeamId()
														: Lane->GetTargetTeamId();
		const int32 TierIndex = bNeutralTower ? 1 : Index < HalfCount ? HalfCount - Index : Index - (CheckpointCount - 1) / 2;
		const float Distance = Lane->GetCheckpointDistance(Index);
		const FVector Location = ResolveGroundLocation(Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
		const FRotator Rotation = Spline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AOverloadGuardTower* Tower = GetWorld()->SpawnActor<AOverloadGuardTower>(TowerClass, Location, Rotation, Parameters);
		if (!Tower)
		{
			continue;
		}
		Tower->InitializeTower(InitialTeamId, TierIndex, Lane);
		Tower->OnTowerCaptured.AddDynamic(this, &UOverloadGameMatchComponent::HandleTowerCaptured);
		Towers.Add(Tower);
		TowerView.Add(Tower);
		LaneTowers.Add(Tower);
		UE_LOG(LogTemp, Log, TEXT("Overload lane %s tower %d/%d initialized for team %d at tier %d"), *GetNameSafe(Lane), Index + 1,
			CheckpointCount, InitialTeamId, TierIndex);
	}
	Lane->SetSpawnedTowers(LaneTowers);
}

void UOverloadGameMatchComponent::EnsureCore(int32 TeamId, const FVector& DesiredLocation, const FRotator& DesiredRotation)
{
	if (!OverloadTeamIds::IsPlayable(TeamId) || CoresByTeam.Contains(TeamId))
	{
		return;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AOverloadEnergyCore* Core =
		GetWorld()->SpawnActor<AOverloadEnergyCore>(CoreClass, ResolveGroundLocation(DesiredLocation), DesiredRotation, Parameters);
	if (!Core)
	{
		return;
	}
	Core->InitializeCore(TeamId, BalanceTuning.CoreOverloadSeconds, BalanceTuning.CoreCoolingPerSecond);
	Core->OnCoreExploded.AddDynamic(this, &UOverloadGameMatchComponent::HandleCoreExploded);
	CoresByTeam.Add(TeamId, Core);
	CoreActors.Add(Core);
	CoreView.Add(Core);
}

void UOverloadGameMatchComponent::RecalculateSupplyAndBalance()
{
	TMap<int32, int32> OriginalPointCount;
	TMap<int32, int32> LostOriginalPoints;
	TMap<int32, int32> ForeignPointsHeld;
	TMap<int32, TMap<int32, int32>> AttackersByVictim;
	for (AOverloadGuardTower* Tower : Towers)
	{
		if (!IsValid(Tower))
		{
			continue;
		}
		const int32 Original = Tower->GetOriginalTeamId();
		const int32 Current = Tower->GetTeamId();
		if (!OverloadTeamIds::IsPlayable(Original))
		{
			continue; // Neutral objectives are not part of a core supply chain.
		}
		OriginalPointCount.FindOrAdd(Original)++;
		if (Current != Original)
		{
			LostOriginalPoints.FindOrAdd(Original)++;
			ForeignPointsHeld.FindOrAdd(Current)++;
			AttackersByVictim.FindOrAdd(Original).FindOrAdd(Current)++;
		}
	}

	for (AOverloadGuardTower* Tower : Towers)
	{
		if (!IsValid(Tower))
		{
			continue;
		}
		const bool bOriginalOwner = Tower->GetTeamId() == Tower->GetOriginalTeamId();
		const float DefenderMultiplier =
			bOriginalOwner ? 1.0f + LostOriginalPoints.FindRef(Tower->GetOriginalTeamId()) * BalanceTuning.DefenderBoostPerLostPoint : 1.0f;
		const float LeaderWeakening =
			1.0f / (1.0f + ForeignPointsHeld.FindRef(Tower->GetTeamId()) * BalanceTuning.LeaderWeakeningPerCapturedPoint);
		Tower->ApplyBalanceMultipliers(DefenderMultiplier, LeaderWeakening);
	}

	for (const TPair<int32, TObjectPtr<AOverloadEnergyCore>>& Pair : CoresByTeam)
	{
		const int32 TeamId = Pair.Key;
		AOverloadEnergyCore* Core = Pair.Value;
		const int32 Total = OriginalPointCount.FindRef(TeamId);
		const bool bCompromised = Total > 0 && LostOriginalPoints.FindRef(TeamId) >= Total;
		int32 LeadingAttacker = INDEX_NONE;
		int32 LeadingCount = 0;
		if (const TMap<int32, int32>* Attackers = AttackersByVictim.Find(TeamId))
		{
			for (const TPair<int32, int32>& Attacker : *Attackers)
			{
				if (Attacker.Value > LeadingCount)
				{
					LeadingAttacker = Attacker.Key;
					LeadingCount = Attacker.Value;
				}
			}
		}
		Core->SetSupplyCompromised(bCompromised, LeadingAttacker);
	}
}

bool UOverloadGameMatchComponent::TryResolveHeroSpawnTransform(
	const AOverloadEnergyCore* Core, int32 TeamId, FTransform& OutTransform) const
{
	(void)TeamId;
	if (!IsValid(Core))
	{
		return false;
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	APlayerStart* BestStart = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (AActor* Actor : PlayerStarts)
	{
		APlayerStart* Candidate = Cast<APlayerStart>(Actor);
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(Candidate->GetActorLocation(), Core->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestStart = Candidate;
		}
	}

	if (!BestStart)
	{
		return false;
	}

	// SpawnHeroForPlayer adds the capsule half-height and performs the authoritative collision check.
	OutTransform = BestStart->GetActorTransform();
	return true;
}

FVector UOverloadGameMatchComponent::ResolveGroundLocation(const FVector& DesiredLocation) const
{
	FVector Result = DesiredLocation;
	FHitResult Hit;
	const FVector Start = DesiredLocation + FVector(0.0f, 0.0f, 1000.0f);
	const FVector End = DesiredLocation - FVector(0.0f, 0.0f, 3000.0f);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		Result.Z = Hit.ImpactPoint.Z;
	}
	return Result;
}

USunriseUnitManagerComponent* UOverloadGameMatchComponent::GetUnitManager() const
{
	return USunriseUnitManagerComponent::Find(this);
}

void UOverloadGameMatchComponent::PresentMatchResult()
{
	if (GetOwner() && GetOwner()->HasAuthority() && !bMatchRecorded)
	{
		if (USunriseGameInstance* GameInstance = GetWorld()->GetGameInstance<USunriseGameInstance>())
		{
			FSunriseMatchRecord Record;
			const ASunrisePlayerController* Controller = Cast<ASunrisePlayerController>(UGameplayStatics::GetPlayerController(this, 0));
			const int32 PlayerTeamId = Controller ? Controller->GetControlledTeamId() : OverloadTeamIds::InitialPlayer;
			Record.Result = WinnerTeamId == PlayerTeamId ? ESunriseMatchResult::Victory : ESunriseMatchResult::Defeat;
			Record.Difficulty = GameInstance->GetSelectedDifficulty();
			Record.DurationSeconds = GetWorld()->GetTimeSeconds();
			Record.FriendlySurvivors = GetAliveUnitCountForTeam(PlayerTeamId);
			Record.EnemiesDefeated = 0;
			if (const USunriseUnitManagerComponent* UnitManager = GetUnitManager())
			{
				for (const ASunriseUnit* Unit : UnitManager->GetUnits())
				{
					if (IsValid(Unit) && !Unit->IsAlive() && OverloadTeamIds::IsPlayable(Unit->GetTeamId()) &&
						Unit->GetTeamId() != PlayerTeamId)
					{
						++Record.EnemiesDefeated;
					}
				}
			}
			Record.CompletedAt = FDateTime::Now();
			GameInstance->RecordMatch(Record);
			bMatchRecorded = true;
		}
	}

	if (EndScreen || WinnerTeamId == INDEX_NONE)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (ASunrisePlayerController* SunriseController = Cast<ASunrisePlayerController>(PlayerController))
	{
		SunriseController->SetCommandsEnabled(false);
	}

	EndScreen = CreateWidget<USunriseEndScreenWidget>(PlayerController, USunriseEndScreenWidget::StaticClass());
	if (EndScreen)
	{
		EndScreen->SetResult(GetMatchResult());
		EndScreen->AddToViewport(100);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, EndScreen, EMouseLockMode::DoNotLock);
		PlayerController->bShowMouseCursor = true;
	}
}

void UOverloadGameMatchComponent::OnRep_RuntimeState()
{
	LaneView.Reset(Lanes.Num());
	for (AOverloadLaneSpline* Lane : Lanes)
	{
		if (IsValid(Lane))
		{
			LaneView.Add(Lane);
		}
	}
	TowerView.Reset(Towers.Num());
	for (AOverloadGuardTower* Tower : Towers)
	{
		if (IsValid(Tower))
		{
			TowerView.Add(Tower);
		}
	}
	CoreView.Reset(CoreActors.Num());
	CoresByTeam.Reset();
	for (AOverloadEnergyCore* Core : CoreActors)
	{
		if (IsValid(Core))
		{
			CoreView.Add(Core);
			CoresByTeam.Add(Core->GetTeamId(), Core);
		}
	}

	if (WinnerTeamId != INDEX_NONE)
	{
		PresentMatchResult();
	}
}
