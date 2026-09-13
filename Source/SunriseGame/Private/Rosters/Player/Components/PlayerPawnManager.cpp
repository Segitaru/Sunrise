// Fill out your copyright notice in the Description page of Project Settings.

#include "Rosters/Player/Components/PlayerPawnManager.h"

#include <Net/UnrealNetwork.h>

#include "GameFramework/GameStateBase.h"
#include "Rosters/Components/GamePawnSelectorComponent.h"

// Deploy Pawn

#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Player/ModularPlayerState.h"

// Confirmation Pawn
#include "Camera/ModularCameraMode.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ModularGameplayTags.h"
#include "ModularPawnData.h"
#include "NativeGameplayTags.h"
#include "Rosters/Components/GamePawnRosterComponent.h"
#include "Rosters/Development/GameRostersDeveloperSettings.h"
#include "Rosters/Messages/GamePawnConfirmationMessage.h"
#include "Rosters/Messages/GamePawnSelectionMessage.h"
#include "Rosters/Systems/GamePawnRosterSubsystem.h"
#include "System/SunriseGameInstance.h"
#include "Teams/System/ModularTeamAgentInterface.h"
#include "Units/SunrisePawnTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerPawnManager)

UE_DEFINE_GAMEPLAY_TAG(TAG_Modular_Gameplay_PawnConfirmed, "Gameplay.Roster.PawnConfirmed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Modular_Gameplay_Selected, "Gameplay.Roster.PawnSelected");

DEFINE_LOG_CATEGORY_STATIC(LogPlayerPawnManager, All, All)

const FName UPlayerPawnManager::NAME_ActorFeatureName("PawnManager");

UPlayerPawnManager::UPlayerPawnManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UPlayerPawnManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bCharacterConfirmed);
	DOREPLIFETIME(ThisClass, SelectedPawnDefinition);
}

UModularPawnData* UPlayerPawnManager::GetSelectedPawnDefinition() const
{
	return SelectedPawnDefinition;
}

void UPlayerPawnManager::SetSelectedPawnDefinition(const UModularPawnData* NewPawnDefinition)
{
	if (!HasAuthority())
	{
		return;
	}
	SelectedPawnDefinition = const_cast<UModularPawnData*>(NewPawnDefinition);
	OnRep_SelectedPawnDefinition();
	if (!SelectedPawnDefinition)
	{
		return;
	}

	if (const auto MatchSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGamePawnRosterSubsystem>())
	{
		if (AModularPlayerState* CurrentPS = Cast<AModularPlayerState>(GetOwner()))
		{
			const FPlayerWithPayload NewPayload(CurrentPS->GetUniqueId(), CurrentPS->GetAccountId(), SelectedPawnDefinition->GetClass());
			MatchSubsystem->PlayersWithPayload.AddUnique(NewPayload);
		}
	}
}

void UPlayerPawnManager::TryTakePawn(UModularPawnData* TakingPawn)
{
	if (bCharacterConfirmed)
	{
		return;
	}

	if (TakingPawn == SelectedPawnDefinition)
	{
		return;
	}
	if (!TakingPawn)
	{
		return;
	}
	TryTakePawn_OnServer(TakingPawn->PawnDeclaration);
}

void UPlayerPawnManager::ConfirmSelectedPawn()
{
	if (!bCharacterConfirmed && SelectedPawnDefinition)
	{
		ConfirmSelectedPawn_OnServer();
	}
}

void UPlayerPawnManager::ResetSelectedPawnConfirmation()
{
	if (bCharacterConfirmed)
	{
		ResetSelectedPawnConfirmation_OnServer();
	}
}

void UPlayerPawnManager::SendMessagePlayerPawnConfirmed(bool bIsAutoSelect) const
{
	FGamePawnConfirmationMessage Message;
	Message.Verb = TAG_Modular_Gameplay_PawnConfirmed;
	Message.Instigator = GetOwner();
	Message.SelectedPawn = SelectedPawnDefinition;
	Message.bIsAutoSelect = bIsAutoSelect;
	Message.bCharacterConfirmed = bCharacterConfirmed;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}

void UPlayerPawnManager::SendMessagePlayerPawnSelected() const
{
	FGamePawnSelectionMessage Message;
	Message.Verb = TAG_Modular_Gameplay_Selected;
	Message.Instigator = GetOwner();
	Message.SelectedPawn = SelectedPawnDefinition;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}

void UPlayerPawnManager::SwapRandom()
{
	TryTakeRandomPawn_OnServer();
}

void UPlayerPawnManager::TryTakePawn_OnServer_Implementation(const FGameplayTag& InPawnDeclaration)
{
	if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
	{
		PawnSelector->TryTakePawnFromPool(GetOwner(), InPawnDeclaration);
	}
}

void UPlayerPawnManager::TryTakeRandomPawn_OnServer_Implementation()
{
	if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
	{
		if (PayloadToSearch)
		{
			UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: try search by class"));

			PawnSelector->TryTakePawnFromPoolByClass(GetOwner(), PayloadToSearch);

			if (SelectedPawnDefinition)
			{
				UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: Search by class success!"));

				return;
			}
		}

		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: try take random pawn!"));
		PawnSelector->TryTakeRandomPawnFromPool(GetOwner());
	}
}

void UPlayerPawnManager::ConfirmSelectedPawn_OnServer_Implementation()
{
	if (SelectedPawnDefinition && !bCharacterConfirmed)
	{
		bCharacterConfirmed = true;
		OnRep_bCharacterConfirmed();

		if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
		{
			PawnSelector->OnPawnConfirmed(GetOwner(), SelectedPawnDefinition);
		}
	}
}

void UPlayerPawnManager::ResetSelectedPawnConfirmation_OnServer_Implementation()
{
	if (bCharacterConfirmed)
	{
		bCharacterConfirmed = false;
		OnRep_bCharacterConfirmed();

		if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
		{
			PawnSelector->OnPawnReset(GetOwner(), SelectedPawnDefinition);
		}
	}
}

void UPlayerPawnManager::CommitRandomPawn()
{
	bChoseWasRandom = true;

	bCharacterConfirmed = true;
	OnRep_bCharacterConfirmed();
}

void UPlayerPawnManager::TryTakeRandomPawnAfterReconnect()
{
	if (!SelectedPawnDefinition)
	{
		const AGameStateBase* GameStateRef = GetWorld()->GetGameState();

		if (!GameStateRef)
		{
			return;
		}

		UExperienceManagerComponent* ExperienceComponent = GameStateRef->FindComponentByClass<UExperienceManagerComponent>();

		check(ExperienceComponent);
		ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(
			FOnExperienceLoaded::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnExperienceLoadedForBot));
	}
}

void UPlayerPawnManager::OnRegister()
{
	Super::OnRegister();
	RegisterInitStateFeature();
}

void UPlayerPawnManager::BeginPlay()
{
	Super::BeginPlay();
	OwnerPS = GetPlayerState<AModularPlayerState>();
	ensure(TryToChangeInitState(ModularGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
	if (HasAuthority() && !SelectedPawnDefinition)
	{
		TryTakePawnOnGameStarted();
	}
}

bool UPlayerPawnManager::CanChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	if (!CurrentState.IsValid() && DesiredState == ModularGameplayTags::InitState_Spawned)
	{
		return GetPlayerState<APlayerState>() != nullptr;
	}
	if (CurrentState == ModularGameplayTags::InitState_Spawned && DesiredState == ModularGameplayTags::InitState_DataAvailable)
	{
		return IsValid(SelectedPawnDefinition);
	}
	// Selection belongs to PlayerState and must be ready before its combat Pawn can exist.
	return true;
}

void UPlayerPawnManager::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {ModularGameplayTags::InitState_Spawned, ModularGameplayTags::InitState_DataAvailable,
		ModularGameplayTags::InitState_DataInitialized, ModularGameplayTags::InitState_GameplayReady};

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UPlayerPawnManager::TryTakePawnOnGameStarted()
{
	OwnerPS = GetPlayerState<AModularPlayerState>();

	if (HasAuthority())
	{
		if (!OwnerPS)
		{
			return;
		}

		if (OwnerPS->IsABot())
		{
#if WITH_EDITOR
			const UGameRostersDeveloperSettings* LocalDeveloperSettings = GetDefault<UGameRostersDeveloperSettings>();

			if (!LocalDeveloperSettings)
			{
				return;
			}

			if (!LocalDeveloperSettings->bTakePawnOnGameStart)
			{
				return;
			}

			if (LocalDeveloperSettings->bOverridePawnDefinitionForBots)
			{
				return;
			}
#endif

			const AGameStateBase* GameState = GetWorld()->GetGameState();

			UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();

			check(ExperienceComponent);
			ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(
				FOnExperienceLoaded::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnExperienceLoadedForBot));
		}
		else
		{
			const AGameStateBase* GameState = GetWorld()->GetGameState();

			UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();

			check(ExperienceComponent);
			ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(
				FOnExperienceLoaded::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnExperienceLoadedForPlayer));
		}
	}
}

void UPlayerPawnManager::ForceUpdate()
{
	UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: Force update on reconnect! %s"), *GetNameSafe(GetOwner()));

	const bool bDefinitionValid = IsValid(SelectedPawnDefinition);

	if (!bDefinitionValid)
	{
		SelectedPawnDefinition = nullptr;

		UE_LOG(
			LogPlayerPawnManager, Display, TEXT("[Reconnect]: Pawn definition not existed for, call random %s"), *GetNameSafe(GetOwner()));

		TryTakePawnOnGameStarted();
		return;
	}

	UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: Mark pawn definition is dirty, call force update for: %s"),
		*GetNameSafe(GetOwner()));

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SelectedPawnDefinition, this);

	SetSelectedPawnDefinition(SelectedPawnDefinition);

	GetOwner()->ForceNetUpdate();
}

void UPlayerPawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	if (IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(GetOwner()))
	{
		TeamAgent->GetTeamChangedDelegateChecked().RemoveDynamic(this, &ThisClass::OnTeamChanged);
	}
	Super::EndPlay(EndPlayReason);

	const UWorld* CurrentWorld = GetWorld();

	if (HasAuthority() && CurrentWorld && !CurrentWorld->bIsTearingDown)
	{
		if (SelectedPawnDefinition && !CurrentWorld->bIsTearingDown)
		{
			if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
			{
				PawnSelector->ReleasePawnIntoPool(GetOwner(), SelectedPawnDefinition);
			}
		}
	}

	if (ForceTakePawnHandle.IsValid())
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: Cancel force take pawn because end play was calling %s"),
			*GetNameSafe(OwnerPS));
	}

	ClearForceTakeTimer();
}

void UPlayerPawnManager::OnExperienceLoadedForBot(const UExperienceDefinition* CurrentExperience)
{
	WaitForRosterReady();
}

void UPlayerPawnManager::WaitForRosterReady()
{
	const AGameStateBase* GameStateRef = GetWorld()->GetGameState();

	if (!GameStateRef)
	{
		return;
	}

	if (UGamePawnRosterComponent* PawnManager = GameStateRef->FindComponentByClass<UGamePawnRosterComponent>())
	{
		PawnManager->CallOrRegister_OnRosterReady(FOnRosterReady::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnRosterReady));
	}
}

void UPlayerPawnManager::OnExperienceLoadedForPlayer(const UExperienceDefinition* CurrentExperience)
{
	const UGamePawnRosterSubsystem* RosterSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGamePawnRosterSubsystem>();
	if (RosterSubsystem && RosterSubsystem->bIsPlayWorld)
	{
		WaitForRosterReady();
	}
}

void UPlayerPawnManager::RestoreSavedPawnSelection_OnClient_Implementation()
{
	const USunriseGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance<USunriseGameInstance>() : nullptr;
	const UModularPawnData* SavedDefinition = GameInstance ? GameInstance->GetSelectedPawnDefinitionId() : nullptr;
	RestoreSavedPawnSelection_OnServer(IsValid(SavedDefinition) ? SavedDefinition->PawnDeclaration : FGameplayTag());
}

void UPlayerPawnManager::RestoreSavedPawnSelection_OnServer_Implementation(const FGameplayTag& PawnDeclaration)
{
	const APlayerState* PlayerState = GetPlayerState<APlayerState>();
	const AController* Controller = PlayerState ? Cast<AController>(PlayerState->GetOwner()) : nullptr;
	if (!HasAuthority() || !IsValid(Controller) || Controller->PlayerState != PlayerState || PlayerState->IsInactive() ||
		PlayerState->IsOnlyASpectator() || bCharacterConfirmed || SelectedPawnDefinition)
	{
		return;
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const UGamePawnRosterComponent* Roster = GameState ? GameState->FindComponentByClass<UGamePawnRosterComponent>() : nullptr;
	UGamePawnSelectorComponent* Selector = GetPawnSelector();
	if (!Roster || !Selector)
	{
		return;
	}
	const TObjectPtr<UModularPawnData>* Entry = Roster->TaggedRoster.Find(PawnDeclaration);
	if (Entry && IsValid(Entry->Get()) && (*Entry)->Specification.HasTag(SunrisePawnTags::Kind_Hero))
	{
		Selector->TryTakePawnFromPool(GetOwner(), PawnDeclaration);
		if (SelectedPawnDefinition)
		{
			UE_LOG(
				LogPlayerPawnManager, Log, TEXT("Restored saved hero %s for %s"), *PawnDeclaration.ToString(), *GetNameSafe(PlayerState));
			return;
		}
	}
	// No saved choice, or the current roster/pick rule does not permit it.
	TryTakeRandomPawn_OnServer();
}

void UPlayerPawnManager::OnRosterReady()
{
	if (!HasAuthority() || SelectedPawnDefinition)
	{
		return;
	}

	IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(GetOwner());
	if (!ensureMsgf(TeamAgent, TEXT("PlayerPawnManager requires a PlayerState with a team interface")))
	{
		return;
	}
	if (TeamAgent->GetGenericTeamId() != FGenericTeamId::NoTeam)
	{
		if (OwnerPS && OwnerPS->IsABot())
		{
			TryTakeRandomPawn_OnServer();
		}
		else
		{
			RestoreSavedPawnSelection_OnClient();
		}
		return;
	}

	TeamAgent->GetTeamChangedDelegateChecked().AddUniqueDynamic(this, &ThisClass::OnTeamChanged);
}

void UPlayerPawnManager::OnTeamChanged(UObject* ObjectChangingTeam, int32 OldTeamID, int32 NewTeamID)
{
	if (SelectedPawnDefinition)
	{
		return;
	}

	if (HasAuthority() && NewTeamID != INDEX_NONE)
	{
		OnRosterReady();
	}
}

void UPlayerPawnManager::OnRep_bCharacterConfirmed()
{
	SendMessagePlayerPawnConfirmed(bChoseWasRandom);
}

void UPlayerPawnManager::OnRep_SelectedPawnDefinition()
{
	CheckDefaultInitialization();
	SendMessagePlayerPawnSelected();

	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("OnRep_SelectedPawnDefinition(): Not valid class"))
		return;
	}

	OnPawnDefinitionUpdated.Broadcast(SelectedPawnDefinition);
}

void UPlayerPawnManager::ClearForceTakeTimer()
{
	bForceTakePawn = false;

	if (const auto CurrentWorld = GetWorld())
	{
		CurrentWorld->GetTimerManager().ClearTimer(ForceTakePawnHandle);
	}
}


UGamePawnSelectorComponent* UPlayerPawnManager::GetPawnSelector() const
{
	const UWorld* CurrentWorld = GetWorld();

	if (!CurrentWorld)
	{
		return nullptr;
	}

	const AGameStateBase* CurrentGameState = CurrentWorld->GetGameState();

	if (!CurrentGameState)
	{
		return nullptr;
	}

	return CurrentGameState->FindComponentByClass<UGamePawnSelectorComponent>();
}

void UPlayerPawnManager::ForceTakePawn()
{
	UE_LOG(LogPlayerPawnManager, Warning, TEXT("[Reconnect]: force take random pawn definition %s"), *GetNameSafe(OwnerPS));

	bForceTakePawn = false;
	const auto MatchSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGamePawnRosterSubsystem>();
	if (!MatchSubsystem)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Reconnect]: subsystem not existed! %s"), *GetNameSafe(OwnerPS));
		TryTakeRandomPawnAfterReconnect();
		return;
	}

	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Reconnect]: Owner PS not existed! %s"), *GetNameSafe(OwnerPS));
		TryTakeRandomPawnAfterReconnect();
		return;
	}

	const FPlayerWithPayload NewPayload(OwnerPS->GetUniqueId(), OwnerPS->GetAccountId());
	if (!MatchSubsystem->PlayersWithPayload.Contains(NewPayload))
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Reconnect]: player doesn't contains inside payload! %s"), *GetNameSafe(OwnerPS));
		TryTakeRandomPawnAfterReconnect();
		return;
	}

	const auto FoundedPayloadIndex = MatchSubsystem->PlayersWithPayload.FindByKey(NewPayload);

	if (!FoundedPayloadIndex)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Reconnect]: cant find payload! %s"), *GetNameSafe(OwnerPS));
		TryTakeRandomPawnAfterReconnect();
		return;
	}

	PayloadToSearch = FoundedPayloadIndex->PayloadClass;
	UE_LOG(LogPlayerPawnManager, Error, TEXT("[Reconnect]: Founded payload class %s"), *GetNameSafe(PayloadToSearch));

	TryTakeRandomPawnAfterReconnect();
}
