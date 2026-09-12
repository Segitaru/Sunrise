// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/Components/GamePawnRosterComponent.h"

#include <Engine/AssetManager.h>

#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ModularPawnData.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Rosters/Player/Components/PlayerPawnManager.h"
#include "System/SunriseTeamSubsystem.h"
#include "UserFacingModularPawnDefinition.h"

namespace Rosters::Gameplay::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(RosterLoad, "Gameplay.Roster.Load");
}

DEFINE_LOG_CATEGORY_STATIC(LogGamePawnRosterComponent, Log, All);

UGamePawnRosterComponent::UGamePawnRosterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	UActorComponent::SetComponentTickEnabled(false);

	SearchingAssetsTypes.Empty();
}

void UGamePawnRosterComponent::CallOrRegister_OnRosterLoaded(FOnRosterLoaded::FDelegate&& Delegate)
{
	if (CurrentRoster.Num() > 0)
	{
		Delegate.Execute();
	}
	else
	{
		OnRosterLoaded.Add(MoveTemp(Delegate));
	}
}

void UGamePawnRosterComponent::CallOrRegister_OnRosterReady(FOnRosterReady::FDelegate&& Delegate)
{
	if (CurrentRoster.Num() > 0)
	{
		Delegate.Execute();
	}
	else
	{
		OnRosterReady.Add(MoveTemp(Delegate));
	}
}

void UGamePawnRosterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentRoster);
}

void UGamePawnRosterComponent::CreatePools(TArray<int32> TeamIDs)
{
	FRosterPool RosterPool;
	RosterPool.AvailablePawnsFromRoster = CurrentRoster;

	for (int32 TeamID : TeamIDs)
	{
		RosterPoolsById.Add(TeamID, RosterPool);
	}

	const USunriseTeamSubsystem* const TeamSubsystem = GetWorld()->GetSubsystem<USunriseTeamSubsystem>();
	if (!TeamSubsystem)
	{
		return;
	}

	bool bIsPartOfTeam = false;
	int32 InstigatorTeamId = -1;

	const AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
	for (APlayerState* Player : GameState->PlayerArray)
	{
		const UPlayerPawnManager* PlayerPawnManager = Player->FindComponentByClass<UPlayerPawnManager>();
		if (!PlayerPawnManager)
		{
			continue;
		}

		TeamSubsystem->FindTeamFromActor(Player, bIsPartOfTeam, InstigatorTeamId);

		if (!bIsPartOfTeam)
		{
			continue;
		}

		if (!PlayerPawnManager->GetSelectedPawnDefinition())
		{
			continue;
		}

		FString NamePawn = PlayerPawnManager->GetSelectedPawnDefinition()->PawnUIDefinition.LoadSynchronous()->PawnDisplayedName.ToString();
		RemovePawnFromPool(InstigatorTeamId, PlayerPawnManager->GetSelectedPawnDefinition());
		UE_LOG(LogGamePawnRosterComponent, Display, TEXT("Remove %s"), *NamePawn);
	}

	UE_LOG(LogGamePawnRosterComponent, Display, TEXT("Pools created, ready to interact with players"));
	OnRosterReady.Broadcast();
	OnRosterReady.Clear();
}

bool UGamePawnRosterComponent::CheckContainPawnInPool(int32 PoolId, const UModularPawnData* PawnToCheck)
{
	FRosterPool RosterPool;

	if (!GetPoolById(PoolId, RosterPool))
	{
		UE_LOG(LogGamePawnRosterComponent, Warning, TEXT("RemovePawnFromPool(): Trying to get a pool that doesn't exist"));
		return false;
	}

	if (!RosterPool.AvailablePawnsFromRoster.Contains(PawnToCheck))
	{
		return false;
	}

	return true;
}

bool UGamePawnRosterComponent::RemovePawnFromPool(int32 PoolId, const TObjectPtr<UModularPawnData> PawnToRemove)
{
	FRosterPool RosterPool;

	if (!GetPoolById(PoolId, RosterPool))
	{
		UE_LOG(LogGamePawnRosterComponent, Warning, TEXT("RemovePawnFromPool(): Trying to get a pool that doesn't exist"));
		return false;
	}

	if (!RosterPool.AvailablePawnsFromRoster.Contains(PawnToRemove))
	{
		return false;
	}

	RosterPool.AvailablePawnsFromRoster.Remove(PawnToRemove);
	RosterPool.LockedPawnsInRoster.Add(PawnToRemove);


	RosterPoolsById.Add(PoolId, RosterPool);


	return true;
}

TObjectPtr<UModularPawnData> UGamePawnRosterComponent::GetRandomPawnFromPool(int32 PoolId)
{
	FRosterPool RosterPool;

	if (!GetPoolById(PoolId, RosterPool))
	{
		UE_LOG(LogGamePawnRosterComponent, Warning, TEXT("GetRandomPawnFromPool(): Trying to get a pool that doesn't exist"));
		return nullptr;
	}

	if (RosterPool.AvailablePawnsFromRoster.Num() <= 0)
	{
		return nullptr;
	}

	const int32 RandomPawnIndex = UKismetMathLibrary::RandomIntegerInRange(0, RosterPool.AvailablePawnsFromRoster.Num() - 1);

	const TObjectPtr<UModularPawnData> RandomPawn = RosterPool.AvailablePawnsFromRoster[RandomPawnIndex];

	return RandomPawn;
}

TObjectPtr<UModularPawnData> UGamePawnRosterComponent::GetPawnFromPoolByClass(int32 PoolId, TSubclassOf<UObject> SearchClass)
{
	FRosterPool RosterPool;

	if (!GetPoolById(PoolId, RosterPool))
	{
		UE_LOG(LogGamePawnRosterComponent, Warning, TEXT("GetRandomPawnFromPool(): Trying to get a pool that doesn't exist"));
		return nullptr;
	}

	if (RosterPool.AvailablePawnsFromRoster.Num() <= 0)
	{
		return nullptr;
	}

	for (const auto& Pawn : RosterPool.AvailablePawnsFromRoster)
	{
		if (Pawn->GetClass() == SearchClass)
		{
			return Pawn;
		}
	}


	return nullptr;
}

bool UGamePawnRosterComponent::ReleasePawnInPool(int32 PoolId, const TObjectPtr<UModularPawnData> ReturningPawn)
{
	FRosterPool RosterPool;

	if (!GetPoolById(PoolId, RosterPool))
	{
		return false;
	}

	if (!RosterPool.LockedPawnsInRoster.Contains(ReturningPawn))
	{
		return false;
	}

	RosterPool.AvailablePawnsFromRoster.Add(ReturningPawn);
	RosterPool.LockedPawnsInRoster.Remove(ReturningPawn);

	RosterPoolsById.Add(PoolId, RosterPool);

	return true;
}

bool UGamePawnRosterComponent::GetPoolById(int32 PoolId, FRosterPool& ReceivedPool)
{
	if (!RosterPoolsById.Contains(PoolId))
	{
		return false;
	}

	if (FRosterPool* RosterPool = RosterPoolsById.Find(PoolId))
	{
		ReceivedPool = *RosterPool;
		return true;
	}

	return false;
}

void UGamePawnRosterComponent::OnRep_CurrentRoster()
{
	if (CurrentRoster.Num() > 0)
	{
		UE_LOG(LogGamePawnRosterComponent, Display, TEXT("Loading Roster SUCCESS!"));
		OnRosterLoaded.Broadcast();
		OnRosterLoaded.Clear();
	}
	else
	{
		ensureMsgf(false, TEXT("%s: Critical Error: Roster loading failing, nothing was uploaded"), *GetPathNameSafe(this));
	}
}

void UGamePawnRosterComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();

	UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();

	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(
		FOnExperienceLoaded::FDelegate::CreateUObject(this, &UGamePawnRosterComponent::OnExperienceLoaded));
}

void UGamePawnRosterComponent::OnExperienceLoaded(const UExperienceDefinition* CurrentExperience)
{
	if (HasAuthority())
	{
		LoadRoster();
	}
}

void UGamePawnRosterComponent::LoadRoster()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	for (const FPrimaryAssetType& SearchingAssetsType : SearchingAssetsTypes)
	{
		TArray<FPrimaryAssetId> FindingPawnAssets;

		UKismetSystemLibrary::GetPrimaryAssetIdList(SearchingAssetsType, FindingPawnAssets);

		TArray<TObjectPtr<UModularPawnData>> PawnsToAppendInRoster;

		for (auto PawnAsset : FindingPawnAssets)
		{
			FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PawnAsset);

			TSubclassOf<UModularPawnData> AssetClass = Cast<UClass>(AssetPath.TryLoad()->GetClass());

			check(AssetClass);

			auto LoaddedPawnAsset = const_cast<UModularPawnData*>(GetDefault<UModularPawnData>(AssetClass));

			check(LoaddedPawnAsset != nullptr);

			if (!LoaddedPawnAsset->ShowInGame())
			{
				continue;
			}
			PawnsToAppendInRoster.Add(LoaddedPawnAsset);
		}

		UE_LOG(LogGamePawnRosterComponent, Display, TEXT("Loading Roster was ended"));

		CurrentRoster.Append(PawnsToAppendInRoster);
	}

	OnRep_CurrentRoster();
}
