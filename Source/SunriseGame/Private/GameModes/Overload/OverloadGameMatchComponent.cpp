// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/OverloadGameMatchComponent.h"

#include <Abilities/SunriseHeroSquadAbility.h>

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/EFExperienceManagerComponent.h"
#include "Components/SplineComponent.h"
#include "Data/EFExperienceDefinition.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/Overload/Actors/OverloadEnergyCore.h"
#include "GameModes/Overload/Actors/OverloadGuardTower.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "GameModes/Overload/Components/OverloadWaveSpawnerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SunrisePlayerController.h"
#include "System/SunriseGameInstance.h"
#include "TimerManager.h"
#include "UI/SunriseWidgets.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

UOverloadGameMatchComponent::UOverloadGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TowerClass = AOverloadGuardTower::StaticClass();
	CoreClass = AOverloadEnergyCore::StaticClass();
	WaveUnitClass = ASunriseUnit::StaticClass();
}

void UOverloadGameMatchComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	UEFExperienceManagerComponent* ExperienceManager = GetOwner()->FindComponentByClass<UEFExperienceManagerComponent>();
	if (ensureMsgf(ExperienceManager, TEXT("OverloadGameMatchComponent requires EFExperienceManagerComponent")))
	{
		ExperienceManager->CallOrRegister_OnExperienceLoaded_LowPriority(
			FOnEFExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
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
	return WinnerTeamId == 0 ? ESunriseMatchResult::Victory : ESunriseMatchResult::Defeat;
}

void UOverloadGameMatchComponent::HandleExperienceLoaded(const UEFExperienceDefinition* CurrentExperience)
{
	if (!IsValid(CurrentExperience) || !GetWorld())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(
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
		if (It->GetSourceTeamId() < 0 || It->GetTargetTeamId() < 0 || It->GetSourceTeamId() == It->GetTargetTeamId())
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
			WaveSpawner->Initialize(Lane, WaveUnitClass);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Overload lane %s has no wave spawner and cannot create units"), *GetNameSafe(Lane));
		}
	}

	// Heroes are the persistent player-influence units. Lane waves remain autonomous creeps.
	for (const TPair<int32, TObjectPtr<AOverloadEnergyCore>>& Pair : CoresByTeam)
	{
		AOverloadEnergyCore* Core = Pair.Value;
		if (!Core)
		{
			continue;
		}
		TScriptInterface<IIControllableEntity> Agent;
		ASunrisePlayerController* PlayerController = Cast<ASunrisePlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		if (PlayerController && PlayerController->GetControlledTeamId() == Pair.Key)
		{
			Agent = PlayerController->GetControllingAgent();
		}
		const FVector Location = ResolveGroundLocation(Core->GetActorLocation() + Core->GetActorForwardVector() * HeroSpawnOffset);
		if (USunriseUnitManagerComponent* UnitManager = GetUnitManager())
		{
			if (ASunriseUnit* Hero = UnitManager->SpawnHeroForTeam(Pair.Key, FTransform(Core->GetActorRotation(), Location), Agent))
			{
				AOverloadLaneSpline* HeroLane = nullptr;
				for (AOverloadLaneSpline* Candidate : Lanes)
				{
					if (Candidate && (Candidate->GetSourceTeamId() == Pair.Key || Candidate->GetTargetTeamId() == Pair.Key))
					{
						HeroLane = Candidate;
						break;
					}
				}
				if (!Agent.GetObject() && PlayerController && PlayerController->GetHeroSquadAbilityClass())
				{
					Hero->SetHeroSquadAbilityClass(PlayerController->GetHeroSquadAbilityClass());
				}

				if (!Agent.GetObject() && HeroLane)
				{
					UOverloadLaneFollowerComponent* Follower =
						NewObject<UOverloadLaneFollowerComponent>(Hero, TEXT("OverloadHeroLaneFollower"));
					Follower->RegisterComponent();
					Follower->Initialize(HeroLane);
				}
			}
		}
	}

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
	GetWorld()->GetTimerManager().SetTimer(HeroFollowerTimer, this, &ThisClass::EnsureEnemyHeroFollowers, 0.5f, true);
	RecalculateSupplyAndBalance();
	UE_LOG(LogTemp, Log, TEXT("Overload initialized: %d teams, %d lanes, %d towers"), CoresByTeam.Num(), Lanes.Num(), Towers.Num());
}

void UOverloadGameMatchComponent::EnsureEnemyHeroFollowers()
{
	if (!GetWorld())
	{
		return;
	}
	ASunrisePlayerController* PlayerController = Cast<ASunrisePlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	for (TActorIterator<ASunriseUnit> It(GetWorld()); It; ++It)
	{
		ASunriseUnit* Hero = *It;
		if (!Hero || !Hero->IsHero() || !Hero->IsAlive() || Hero->GetControllingAgent().GetObject())
		{
			continue;
		}
		if (PlayerController && PlayerController->GetHeroSquadAbilityClass() && !Hero->GetHeroSquadAbilityClass())
		{
			Hero->SetHeroSquadAbilityClass(PlayerController->GetHeroSquadAbilityClass());
		}
		if (Hero->FindComponentByClass<UOverloadLaneFollowerComponent>())
		{
			continue;
		}
		AOverloadLaneSpline* HeroLane = nullptr;
		for (AOverloadLaneSpline* Candidate : Lanes)
		{
			if (Candidate && (Candidate->GetSourceTeamId() == Hero->GetTeamId() || Candidate->GetTargetTeamId() == Hero->GetTeamId()))
			{
				HeroLane = Candidate;
				break;
			}
		}
		if (HeroLane)
		{
			UOverloadLaneFollowerComponent* Follower = NewObject<UOverloadLaneFollowerComponent>(Hero, TEXT("OverloadHeroLaneFollower"));
			Follower->RegisterComponent();
			Follower->Initialize(HeroLane);
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
		const int32 InitialTeamId = bNeutralTower ? INDEX_NONE : Index < HalfCount ? Lane->GetSourceTeamId() : Lane->GetTargetTeamId();
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
	if (CoresByTeam.Contains(TeamId))
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
			const int32 PlayerTeamId = Controller ? Controller->GetControlledTeamId() : 0;
			Record.Result = WinnerTeamId == PlayerTeamId ? ESunriseMatchResult::Victory : ESunriseMatchResult::Defeat;
			Record.Difficulty = GameInstance->GetSelectedDifficulty();
			Record.DurationSeconds = GetWorld()->GetTimeSeconds();
			Record.FriendlySurvivors = GetAliveUnitCountForTeam(PlayerTeamId);
			Record.EnemiesDefeated = GetAliveUnitCountForTeam(PlayerTeamId == 0 ? 1 : 0);
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
