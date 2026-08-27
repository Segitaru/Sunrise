// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/SunriseGameMatchComponent.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/EFExperienceManagerComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Data/EFExperienceDefinition.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/SunrisePlayerController.h"
#include "System/SunriseGameInstance.h"
#include "TimerManager.h"
#include "UI/SunriseWidgets.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

namespace SunriseMatch
{
	ESunriseUnitRole GetRoleForSlot(int32 Index)
	{
		static constexpr ESunriseUnitRole Formation[] = {ESunriseUnitRole::Melee, ESunriseUnitRole::Vanguard, ESunriseUnitRole::Ranged,
			ESunriseUnitRole::Healer, ESunriseUnitRole::Mage};
		return Formation[Index % UE_ARRAY_COUNT(Formation)];
	}
} // namespace SunriseMatch

USunriseGameMatchComponent::USunriseGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UnitClass = ASunriseUnit::StaticClass();
}

void USunriseGameMatchComponent::BeginPlay()
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

	UEFExperienceManagerComponent* ExperienceManager = GetOwner()->FindComponentByClass<UEFExperienceManagerComponent>();
	if (ensureMsgf(ExperienceManager, TEXT("SunriseGameMatchComponent requires EFExperienceManagerComponent")))
	{
		ExperienceManager->CallOrRegister_OnExperienceLoaded_LowPriority(
			FOnEFExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
	}
}

void USunriseGameMatchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, MatchResult);
}

USunriseGameMatchComponent* USunriseGameMatchComponent::Find(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<USunriseGameMatchComponent>() : nullptr;
}

int32 USunriseGameMatchComponent::GetFriendlyAlive() const
{
	const USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	return UnitManager ? UnitManager->GetFriendlyAlive() : 0;
}

int32 USunriseGameMatchComponent::GetEnemyAlive() const
{
	const USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	return UnitManager ? UnitManager->GetEnemyAlive() : 0;
}

void USunriseGameMatchComponent::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Sunrise/Maps/Test/L_MainMenu")), true);
}

void USunriseGameMatchComponent::HandleExperienceLoaded(const UEFExperienceDefinition* CurrentExperience)
{
	if (!IsValid(CurrentExperience) || !GetWorld())
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(ScenarioTimer, this, &ThisClass::InitializeScenario, ScenarioInitializationDelay, false);
}

void USunriseGameMatchComponent::InitializeScenario()
{
	if (bScenarioInitialized)
	{
		return;
	}
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Navigation || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(this))
	{
		++NavigationWaitAttempts;
		if (NavigationWaitAttempts % 20 == 0)
			UE_LOG(LogTemp, Warning, TEXT("Sunrise match is waiting for navigation (%d attempts)"), NavigationWaitAttempts);
		GetWorld()->GetTimerManager().SetTimer(ScenarioTimer, this, &ThisClass::InitializeScenario, 0.25f, false);
		return;
	}
	bScenarioInitialized = true;

	USunriseUnitManagerComponent* UnitManager = GetUnitManager();
	if (!ensure(UnitManager))
	{
		return;
	}

	TArray<AActor*> ExistingUnits;
	UGameplayStatics::GetAllActorsOfClass(this, ASunriseUnit::StaticClass(), ExistingUnits);
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Sunrise elimination scenario requires at least two PlayerStart actors; found %d"), PlayerStarts.Num());
		bScenarioInitialized = false;
		return;
	}

	int32 FirstStartIndex = 0;
	int32 SecondStartIndex = 1;
	float GreatestDistanceSquared = -1.0f;
	for (int32 First = 0; First < PlayerStarts.Num() - 1; ++First)
	{
		for (int32 Second = First + 1; Second < PlayerStarts.Num(); ++Second)
		{
			const float DistanceSquared =
				FVector::DistSquared2D(PlayerStarts[First]->GetActorLocation(), PlayerStarts[Second]->GetActorLocation());
			if (DistanceSquared > GreatestDistanceSquared)
			{
				GreatestDistanceSquared = DistanceSquared;
				FirstStartIndex = First;
				SecondStartIndex = Second;
			}
		}
	}

	FVector FriendlyCenter = PlayerStarts[FirstStartIndex]->GetActorLocation();
	FVector EnemyCenter = PlayerStarts[SecondStartIndex]->GetActorLocation();
	if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (FVector::DistSquared2D(PlayerPawn->GetActorLocation(), EnemyCenter) <
			FVector::DistSquared2D(PlayerPawn->GetActorLocation(), FriendlyCenter))
		{
			Swap(FriendlyCenter, EnemyCenter);
		}
	}

	const float CenterDistance = FMath::Sqrt(GreatestDistanceSquared);
	const float SafeRadiusFromDistance = (CenterDistance - MinimumOpposingArmySeparation) * 0.5f;
	const float EffectiveSpawnRadius = FMath::Clamp(SafeRadiusFromDistance, 150.0f, ArmySpawnRadius);
	if (CenterDistance - EffectiveSpawnRadius * 2.0f < MinimumOpposingArmySeparation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Sunrise PlayerStarts are only %.0f cm apart; %.0f cm separation cannot be guaranteed"),
			CenterDistance, MinimumOpposingArmySeparation);
	}

	// Never destroy units owned by another match component. Existing authored/spawned units
	// participate in the composed elimination condition and the scenario adds its own armies.
	for (AActor* Actor : ExistingUnits)
	{
		UnitManager->RegisterUnit(Cast<ASunriseUnit>(Actor));
	}

	const USunriseGameInstance* GameInstance = GetWorld()->GetGameInstance<USunriseGameInstance>();
	const FSunriseDifficultyTuning Tuning = GameInstance ? GameInstance->GetDifficultyTuning() : FSunriseDifficultyTuning();
	const int32 FriendlyCount = FMath::Max(1, BaseFriendlyUnitCount);
	const int32 EnemyCount = FMath::Max(1, FMath::RoundToInt(BaseEnemyUnitCount * Tuning.EnemyCountMultiplier));
	for (int32 Index = 0; Index < FriendlyCount; ++Index)
	{
		if (!SpawnUnitAtAvailableLocation(
				ESunriseTeam::Friendly, SunriseMatch::GetRoleForSlot(Index), FriendlyCenter, EffectiveSpawnRadius, Index))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn friendly Sunrise unit %d/%d"), Index + 1, FriendlyCount);
		}
	}
	for (int32 Index = 0; Index < EnemyCount; ++Index)
	{
		if (ASunriseUnit* Enemy = SpawnUnitAtAvailableLocation(
				ESunriseTeam::Enemy, SunriseMatch::GetRoleForSlot(Index), EnemyCenter, EffectiveSpawnRadius, Index))
		{
			Enemy->ApplyDifficultyScaling(Tuning.EnemyHealthMultiplier, Tuning.EnemyPowerMultiplier);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy Sunrise unit %d/%d"), Index + 1, EnemyCount);
		}
	}

	InitialEnemyCount = GetEnemyAlive();
	MatchStartTime = GetWorld()->GetTimeSeconds();
	bMatchArmed = GetFriendlyAlive() > 0 && GetEnemyAlive() > 0;
	UnitManager->OnArmyCountChanged.Broadcast(GetFriendlyAlive(), GetEnemyAlive());
	EvaluateMatch();
}

void USunriseGameMatchComponent::HandleUnitDied(ASunriseUnit* Unit)
{
	(void)Unit;
	EvaluateMatch();
}

void USunriseGameMatchComponent::EvaluateMatch()
{
	if (!bMatchArmed || MatchResult != ESunriseMatchResult::InProgress)
	{
		return;
	}
	if (GetEnemyAlive() == 0)
	{
		FinishMatch(ESunriseMatchResult::Victory);
	}
	else if (GetFriendlyAlive() == 0)
	{
		FinishMatch(ESunriseMatchResult::Defeat);
	}
}

void USunriseGameMatchComponent::FinishMatch(ESunriseMatchResult Result)
{
	MatchResult = Result;
	if (USunriseGameInstance* SunriseGI = GetWorld()->GetGameInstance<USunriseGameInstance>())
	{
		FSunriseMatchRecord Record;
		Record.Result = Result;
		Record.Difficulty = SunriseGI->GetSelectedDifficulty();
		Record.DurationSeconds = FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - MatchStartTime);
		Record.FriendlySurvivors = GetFriendlyAlive();
		Record.EnemiesDefeated = FMath::Max(0, InitialEnemyCount - GetEnemyAlive());
		Record.CompletedAt = FDateTime::Now();
		SunriseGI->RecordMatch(Record);
	}
	OnMatchFinished.Broadcast(Result);

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
		EndScreen->SetResult(Result);
		EndScreen->AddToViewport(100);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, EndScreen, EMouseLockMode::DoNotLock);
		PlayerController->bShowMouseCursor = true;
	}
}

void USunriseGameMatchComponent::OnRep_MatchResult()
{
	if (MatchResult != ESunriseMatchResult::InProgress)
	{
		OnMatchFinished.Broadcast(MatchResult);
	}
}

ASunriseUnit* USunriseGameMatchComponent::SpawnUnitAtAvailableLocation(
	ESunriseTeam Team, ESunriseUnitRole InRole, const FVector& ArmyCenter, float SpawnRadius, int32 FormationIndex)
{
	if (!UnitClass || !GetWorld())
	{
		return nullptr;
	}
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ASunriseUnit* Defaults = UnitClass->GetDefaultObject<ASunriseUnit>();
	const float Radius = Defaults && Defaults->GetCapsuleComponent() ? Defaults->GetCapsuleComponent()->GetScaledCapsuleRadius() : 42.0f;
	const float HalfHeight =
		Defaults && Defaults->GetCapsuleComponent() ? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.0f;

	for (int32 Attempt = 0; Attempt < 64; ++Attempt)
	{
		const float Angle = FormationIndex * 2.399963f + Attempt * 0.71f;
		const float Distance = FMath::Min(SpawnRadius, 140.0f + 95.0f * FormationIndex + Attempt * 25.0f);
		FVector Candidate = ArmyCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
		FNavLocation Projected;
		const bool bProjected = Navigation && Navigation->ProjectPointToNavigation(Candidate, Projected, FVector(300.0f, 300.0f, 600.0f));
		if (bProjected)
		{
			Candidate = Projected.Location;
		}
		if (FVector::DistSquared2D(Candidate, ArmyCenter) > FMath::Square(SpawnRadius + 100.0f))
		{
			continue;
		}

		FHitResult GroundHit;
		const FVector TraceStart(Candidate.X, Candidate.Y, FMath::Max(Candidate.Z, ArmyCenter.Z) + 600.0f);
		const FVector TraceEnd(Candidate.X, Candidate.Y, FMath::Min(Candidate.Z, ArmyCenter.Z) - 3000.0f);
		FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(SunriseUnitSpawnGround), false);
		if (GetWorld()->LineTraceSingleByObjectType(
				GroundHit, TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
		{
			Candidate.Z = GroundHit.ImpactPoint.Z + HalfHeight + 2.0f;
		}
		else if (bProjected)
		{
			Candidate.Z = Projected.Location.Z + HalfHeight + 2.0f;
		}
		else
		{
			Candidate.Z = ArmyCenter.Z;
		}

		if (GetWorld()->OverlapAnyTestByObjectType(
				Candidate, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeCapsule(Radius, HalfHeight)))
		{
			continue;
		}

		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ASunriseUnit* Unit = GetWorld()->SpawnActor<ASunriseUnit>(UnitClass, Candidate, FRotator::ZeroRotator, Parameters);
		if (!Unit)
		{
			continue;
		}
		Unit->SetTeam(Team);
		Unit->SetUnitRole(InRole, true);
		if (Team == ESunriseTeam::Friendly)
		{
			if (ASunrisePlayerController* PlayerController = Cast<ASunrisePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
			{
				Unit->ConfigureControl(ESunriseUnitKind::Summoned, PlayerController->GetControllingAgent());
				if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(PlayerController))
				{
					Manager->RegisterControlledEntity(Unit);
				}
			}
		}
		if (USunriseUnitManagerComponent* UnitManager = GetUnitManager())
		{
			UnitManager->RegisterUnit(Unit);
		}
		return Unit;
	}
	return nullptr;
}

USunriseUnitManagerComponent* USunriseGameMatchComponent::GetUnitManager() const
{
	return USunriseUnitManagerComponent::Find(this);
}
