// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/Components/GamePawnSelectorComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "Rosters/Components/GamePawnRosterComponent.h"
#include "Rosters/Messages/GamePoolChangesMessage.h"
#include "Rosters/Player/PickRule/PickModeRule_AllPick.h"
#include "Rosters/Player/PickRule/PickModeRule_FFA.h"
#include "System/SunriseTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GamePawnSelectorComponent)

namespace Rosters::Gameplay::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(PoolChange, "Gameplay.Pool.Change");
}

DEFINE_LOG_CATEGORY_STATIC(LogGamePawnSelectorComponent, Log, All)

UGamePawnSelectorComponent::UGamePawnSelectorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	UActorComponent::SetComponentTickEnabled(false);

	RuleForPickMode.Add(AllPick, UPickModeRule_AllPick::StaticClass());
	RuleForPickMode.Add(FreeForAll, UPickModeRule_FFA::StaticClass());
}

void UGamePawnSelectorComponent::TryTakePawnFromPool(
	const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> TakingPawn)
{
	if (!CurrentPickRule)
	{
		return;
	}

	TObjectPtr<UModularPawnData> ReleasedPawn;
	int32 PoolId = -1;

	if (!CurrentPickRule->TryTakePawnFromPool(Instigator, TakingPawn, PoolId, ReleasedPawn))
	{
		return;
	}

	if (PoolId == -1)
	{
		return;
	}

	SendPoolChange_Multicast(PoolId, true, TakingPawn);
	if (ReleasedPawn)
	{
		SendPoolChange_Multicast(PoolId, false, ReleasedPawn);
	}
}

void UGamePawnSelectorComponent::TryTakeRandomPawnFromPool(const UObject* Instigator)
{
	if (!CurrentPickRule)
	{
		return;
	}

	TObjectPtr<UModularPawnData> TakingPawn = nullptr;
	TObjectPtr<UModularPawnData> ReleasedPawn = nullptr;
	int32 PoolId = -1;

	if (!CurrentPickRule->TryTakeRandomPawnFromPool(Instigator, PoolId, TakingPawn, ReleasedPawn))
	{
		return;
	}

	if (PoolId == -1)
	{
		return;
	}

	SendPoolChange_Multicast(PoolId, true, TakingPawn);
	if (ReleasedPawn)
	{
		SendPoolChange_Multicast(PoolId, false, ReleasedPawn);
	}
}
void UGamePawnSelectorComponent::TryTakePawnFromPoolByClass(
	const UObject* Instigator, TSubclassOf<UObject> ClassToSearch)
{
	if (!CurrentPickRule)
	{
		return;
	}

	TObjectPtr<UModularPawnData> TakingPawn = nullptr;
	TObjectPtr<UModularPawnData> ReleasedPawn = nullptr;
	int32 PoolId = -1;

	if (!CurrentPickRule->TryTakePawnFromPoolByClass(Instigator, PoolId, ClassToSearch, TakingPawn, ReleasedPawn))
	{
		return;
	}

	if (PoolId == -1)
	{
		return;
	}

	SendPoolChange_Multicast(PoolId, true, TakingPawn);
	if (ReleasedPawn)
	{
		SendPoolChange_Multicast(PoolId, false, ReleasedPawn);
	}
}

void UGamePawnSelectorComponent::TryLockPawnInPool(
	const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> LockingPawn)
{
}

void UGamePawnSelectorComponent::ReleasePawnIntoPool(
	const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ReleasePawn)
{
	UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent();

	USunriseTeamSubsystem* const TeamSubsystem = GetWorld()->GetSubsystem<USunriseTeamSubsystem>();

	if (!IsValid(TeamSubsystem))
	{
		ensureMsgf(false, TEXT("%s: Critical Error: Team Subsystem does not exist"), *GetPathNameSafe(this));
	}

	bool bIsPartOfTeam = false;
	int32 InstigatorTeamId = -1;
	TeamSubsystem->FindTeamFromActor(Instigator, bIsPartOfTeam, InstigatorTeamId);

	if (!bIsPartOfTeam)
	{
		return;
	}

	if (PawnManager)
	{
		PawnManager->ReleasePawnInPool(InstigatorTeamId, ReleasePawn);
	}
}

void UGamePawnSelectorComponent::OnPawnConfirmed(
	const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn)
{
	if (!CurrentPickRule)
	{
		return;
	}

	int32 PoolId;
	TArray<TObjectPtr<UModularPawnData>> Pawns;

	if (!CurrentPickRule->OnPawnConfirmed(Instigator, ConfirmedPawn, PoolId, Pawns))
	{
		return;
	}

	for (const auto& Pawn : Pawns)
	{
		SendPoolChange_Multicast(PoolId, true, Pawn);
	}
}
void UGamePawnSelectorComponent::OnPawnReset(
	const TObjectPtr<UObject> Instigator, const TObjectPtr<UModularPawnData> ReleasedPawn)
{
	if (!CurrentPickRule)
	{
		return;
	}

	int32 PoolId;
	TArray<TObjectPtr<UModularPawnData>> Pawns;

	if (!CurrentPickRule->OnPawnReleased(Instigator, ReleasedPawn, PoolId, Pawns))
	{
		return;
	}

	for (const auto& Pawn : Pawns)
	{
		SendPoolChange_Multicast(PoolId, false, Pawn);
	}
}

void UGamePawnSelectorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const auto CurrentGM = GetGameMode<AGameModeBase>())
	{
		TArray<FString> CurrentOptions;

		FString CurrentMode = UGameplayStatics::ParseOption(CurrentGM->OptionsString, TEXT("PawnPickMode"));

		// We need always any value here
		if (CurrentMode == "")
		{
			CurrentMode = TEXT("FreeForAll");
		}

		for (const auto& [Mode, RuleClass] : RuleForPickMode)
		{
			if (UEnum::GetValueAsString(Mode) == CurrentMode)
			{
				CurrentPickRule = NewObject<UPickModeRule>(this, RuleClass);
				break;
			}
		}
	}

	// Listen for the experience load to complete
	if (UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent())
	{
		PawnManager->CallOrRegister_OnRosterLoaded(
			FOnRosterLoaded::FDelegate::CreateUObject(this, &UGamePawnSelectorComponent::OnRosterLoaded));
	}
}

void UGamePawnSelectorComponent::OnRosterLoaded()
{
	if (HasAuthority())
	{
		USunriseTeamSubsystem* const TeamSubsystem = GetWorld()->GetSubsystem<USunriseTeamSubsystem>();

		if (!IsValid(TeamSubsystem))
		{
			ensureMsgf(false, TEXT("%s: Critical Error: Team Subsystem does not exist"), *GetPathNameSafe(this));
		}

		if (UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent())
		{
			UE_LOG(LogGamePawnSelectorComponent, Display, TEXT("An event for creating pools has been dispatched"));
			PawnManager->CreatePools(TeamSubsystem->GetTeamIds());
		}
		else
		{
			ensureMsgf(
				false, TEXT("%s: Critical Error: Game Pawn Roster Component does not exist!"), *GetPathNameSafe(this));
		}
	}
}

void UGamePawnSelectorComponent::SendPoolChange_Multicast_Implementation(
	int32 PawnPoolId, bool bIsLocked, const UModularPawnData* ChangedPawn)
{
	FGamePoolChangesMessage Message;
	Message.Verb = Rosters::Gameplay::Tags::PoolChange;
	Message.PoolID = PawnPoolId;
	Message.bIsLock = bIsLocked;
	Message.ChangedPawn = ChangedPawn;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}

UGamePawnRosterComponent* UGamePawnSelectorComponent::GetPawnManagerComponent() const
{
	AGameStateBase* GameStateRef = GetGameState<AGameStateBase>();
	if (!GameStateRef)
	{
		return nullptr;
	}

	UGamePawnRosterComponent* PawnManager =
		GetGameState<AGameStateBase>()->FindComponentByClass<UGamePawnRosterComponent>();

	return PawnManager;
}
