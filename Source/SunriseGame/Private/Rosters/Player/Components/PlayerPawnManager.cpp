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
#include "Components/ModularHeroComponent.h"
#include "Cosmetics/Components/PawnCosmeticCreatorComponent.h"
#include "Fragments/ModularPawnDataFragment.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ModularGameplayTags.h"
#include "ModularPawnData.h"
#include "NativeGameplayTags.h"
#include "Pawn/Components/ModularPawnExtensionComponent.h"
#include "Pawn/ModularCharacter.h"
#include "Rosters/Components/GamePawnRosterComponent.h"
#include "Rosters/Development/GameRostersDeveloperSettings.h"
#include "Rosters/Messages/GamePawnConfirmationMessage.h"
#include "Rosters/Messages/GamePawnSelectionMessage.h"
#include "Rosters/Systems/GamePawnRosterSubsystem.h"
#include "System/SunriseTeamAgentInterface.h"

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
	SelectedPawnDefinition = const_cast<UModularPawnData*>(NewPawnDefinition);
	OnRep_SelectedPawnDefinition();

	if (const auto MatchSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGamePawnRosterSubsystem>())
	{
		if (OwnerPS)
		{
			const FPlayerWithPayload NewPayload(OwnerPS->GetUniqueId(), OwnerPS->GetAccountId(), SelectedPawnDefinition->GetClass());
			MatchSubsystem->PlayersWithPayload.AddUnique(NewPayload);
		}
	}
}

void UPlayerPawnManager::TryTakePawn(const UModularPawnData* TakingPawn)
{
	if (bCharacterConfirmed)
	{
		return;
	}

	if (TakingPawn == SelectedPawnDefinition)
	{
		return;
	}

	TryTakePawn_OnServer(TakingPawn);
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

TSubclassOf<UModularCameraMode> UPlayerPawnManager::GetPawnCameraMode() const
{
	return PawnCameraMode;
}

void UPlayerPawnManager::TryTakePawn_OnServer_Implementation(const UModularPawnData* TakingPawn)
{
	if (!TakingPawn)
	{
		return;
	}

	const TObjectPtr<UModularPawnData> CopyObject = const_cast<UModularPawnData*>(TakingPawn);

	if (UGamePawnSelectorComponent* PawnSelector = GetPawnSelector())
	{
		PawnSelector->TryTakePawnFromPool(GetOwner(), CopyObject);
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

void UPlayerPawnManager::TryAddPawnPartComponent()
{
	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Cosmetic]: Pawn definition doesn't existed yet"));
		return;
	}

	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Cosmetic]: Owner PS doesn't existed"));
		return;
	}

	AModularCharacter* Pawn = Cast<AModularCharacter>(OwnerPS->GetPawn());
	if (!Pawn)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Cosmetic]: Pawn not valid"));
		return;
	}
	const auto PawnPartComponent = Pawn->FindComponentByClass<UPawnCosmeticCreatorComponent>();

	if (!PawnPartComponent)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Cosmetic]: Pawn Not have cosmetic component"));
		return;
	}

	PawnPartComponent->RemoveAllCharacterParts();

	for (const auto& PawnPart : SelectedPawnDefinition->PawnMeshes)
	{
		PawnPartComponent->AddCharacterPart(PawnPart);
	}

	Pawn->OnCosmeticPartAddedExternal();
}

void UPlayerPawnManager::TryAddPawnAbilities()
{
	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Abilities]: Pawn definition doesn't existed yet"));
		return;
	}

	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Abilities]: Owner PS doesn't existed"));
		return;
	}

	UModularAbilitySystemComponent* ASC = OwnerPS->GetModularAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Abilities]: Not valid ASC component"));
		return;
	}

	RemoveGrantedAbility(ASC);

	AddGrantedAbilities(ASC, SelectedPawnDefinition->AbilitySets);
}


void UPlayerPawnManager::TryAddPawnComponents()
{
	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Components]: Pawn definition doesn't existed yet"));
		return;
	}

	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Components]: Owner PS doesn't existed"));
		return;
	}

	CurrentGrantedComponents.TakeFromActor(GetOwner());

	for (const auto& ComponentSet : SelectedPawnDefinition->ComponentsSets)
	{
		ComponentSet.GiveComponentsToActor(GetOwner(), &CurrentGrantedComponents);
	}
}

void UPlayerPawnManager::TryActivateFragments()
{
	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Components]: Pawn definition doesn't existed yet"));
		return;
	}

	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Components]: Owner PS doesn't existed"));
		return;
	}

	for (const auto& Fragment : SelectedPawnDefinition->Fragments)
	{
		Fragment->Activate(OwnerPS, OwnerPS->GetPawn());
	}
}

void UPlayerPawnManager::RemoveGrantedAbility(UModularAbilitySystemComponent* FromASC)
{
	CurrentGrantedAbility.TakeFromAbilitySystem(FromASC);
}

void UPlayerPawnManager::AddGrantedAbilities(
	UModularAbilitySystemComponent* IntoASC, TArray<TSoftObjectPtr<UModularAbilitySet>> AbilitiesToGrand)
{
	for (const auto& AbilitySet : AbilitiesToGrand)
	{
		if (const auto LoadedAbilitySet = AbilitySet.LoadSynchronous())
		{
			LoadedAbilitySet->GiveToAbilitySystem(IntoASC, &CurrentGrantedAbility, nullptr);
		}
	}

	const FName NAME_AbilityReady("ModularAbilitiesReady");

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(GetOwner(), NAME_AbilityReady);

	GetOwner()->ForceNetUpdate();
}

void UPlayerPawnManager::SetDefaultAbilities()
{
	if (!OwnerPS)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Abilities]: Owner PS doesn't existed on try set default ability set"));
		return;
	}

	UModularAbilitySystemComponent* ASC = OwnerPS->GetModularAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("TryAddPawnAbilities(): Not valid ASC component"));
		return;
	}

	ASC->CancelAbilities();

	RemoveGrantedAbility(ASC);

	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Error, TEXT("[Abilities]: Pawn definition doesn't existed on try set default abilities"));
		return;
	}

	AddGrantedAbilities(ASC, SelectedPawnDefinition->AbilitySets);
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

void UPlayerPawnManager::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UModularHeroComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(ModularGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

bool UPlayerPawnManager::CanChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	if (CurrentState == ModularGameplayTags::InitState_DataAvailable && DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AModularPlayerState* ModularPS = GetPlayerState<AModularPlayerState>();

		return ModularPS && Manager->HasFeatureReachedInitState(
								GetOwner(), UModularHeroComponent::NAME_ActorFeatureName, ModularGameplayTags::InitState_DataInitialized);
	}

	return true;
}

void UPlayerPawnManager::HandleChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == ModularGameplayTags::InitState_DataAvailable && DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		TryTakePawnOnGameStarted();
	}
}

void UPlayerPawnManager::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UModularHeroComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == ModularGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
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

void UPlayerPawnManager::OnPawnDataLoaded()
{
	if (HasAuthority())
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: pawn data loaded: %s"), *GetNameSafe(GetOwner()));

		if (!OwnerPS->GetPawn())
		{
			UE_LOG(LogPlayerPawnManager, Warning, TEXT("[Initialize]: Pawn not existed on pawn data loaded: %s"), *GetNameSafe(GetOwner()));

			OwnerPS->OnPawnSet.AddDynamic(this, &UPlayerPawnManager::OnPawnSet);
			return;
		}

		if (UModularPawnExtensionComponent* HeroComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(OwnerPS->GetPawn()))
		{
			if (!HeroComp->HasReachedInitState(ModularGameplayTags::InitState_GameplayReady))
			{
				UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: Pawn not ready to gameplay on pawn data loaded: %s"),
					*GetNameSafe(GetOwner()));

				const auto& AscIntializeHandle =
					FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized);
				OnAbilitySystemInitializedHandle = AscIntializeHandle.GetHandle();
				HeroComp->OnAbilitySystemInitialized_RegisterAndCall(AscIntializeHandle);
				return;
			}
		}
		else
		{
			UE_LOG(LogPlayerPawnManager, Error, TEXT("[Initialize]: not found Hero component on pawn data loaded: %s"),
				*GetNameSafe(GetOwner()));
		}

		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: Pawn ready to initialize on pawn data loaded: %s"),
			*GetNameSafe(GetOwner()));

		TryAddPawnPartComponent();
		TryAddPawnAbilities();
		TryAddPawnComponents();
		TryActivateFragments();
	}
}

void UPlayerPawnManager::OnExperienceLoadedForBot(const UExperienceDefinition* CurrentExperience)
{
	const AGameStateBase* GameStateRef = GetWorld()->GetGameState();

	if (!GameStateRef)
	{
		return;
	}

	if (UGamePawnRosterComponent* PawnManager = GameStateRef->FindComponentByClass<UGamePawnRosterComponent>())
	{
		PawnManager->CallOrRegister_OnRosterReady(FOnRosterLoaded::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnRosterReady));
	}
}

void UPlayerPawnManager::OnExperienceLoadedForPlayer(const UExperienceDefinition* CurrentExperience)
{
	if (const auto MatchSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGamePawnRosterSubsystem>())
	{
		if (MatchSubsystem->bIsPlayWorld)
		{
			if (OwnerPS)
			{
				const FPlayerWithPayload NewPayload(OwnerPS->GetUniqueId(), OwnerPS->GetAccountId());

				const bool bPayloadHistoryExisted = MatchSubsystem->PlayersWithPayload.Contains(NewPayload);
				const bool bDefinitionValid = IsValid(SelectedPawnDefinition);

				if (bPayloadHistoryExisted && bDefinitionValid)
				{
					return;
				}

				if (!bPayloadHistoryExisted)
				{
					UE_LOG(
						LogPlayerPawnManager, Display, TEXT("[Reconnect]: player removed from payload history %s"), *GetNameSafe(OwnerPS));
				}

				if (!bDefinitionValid)
				{
					UE_LOG(LogPlayerPawnManager, Display, TEXT("[Reconnect]: pawn definition dosn't existed %s"), *GetNameSafe(OwnerPS));

					// We must wait copy properties!
					bForceTakePawn = true;
					GetWorld()->GetTimerManager().SetTimer(ForceTakePawnHandle, this, &ThisClass::ForceTakePawn, 0.5f, false);
					return;
				}

				UE_LOG(LogPlayerPawnManager, Warning, TEXT("[Reconnect]: take random pawn definition %s"), *GetNameSafe(OwnerPS));
				TryTakeRandomPawnAfterReconnect();
			}
		}
	}
}

void UPlayerPawnManager::OnRosterReady()
{
	if (SelectedPawnDefinition)
	{
		return;
	}

	ISunriseTeamAgentInterface* TeamAgent = Cast<ISunriseTeamAgentInterface>(GetOwner());
	if (TeamAgent->GetGenericTeamId() != FGenericTeamId::NoTeam)
	{
		TryTakeRandomPawn_OnServer();
		return;
	}

	TeamAgent->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnTeamChanged);
}

void UPlayerPawnManager::OnTeamChanged(UObject* ObjectChangingTeam, int32 OldTeamID, int32 NewTeamID)
{
	if (SelectedPawnDefinition)
	{
		return;
	}

	if (NewTeamID != -1)
	{
		TryTakeRandomPawn_OnServer();
	}
}

void UPlayerPawnManager::OnRep_bCharacterConfirmed()
{
	SendMessagePlayerPawnConfirmed(bChoseWasRandom);
}

void UPlayerPawnManager::OnRep_SelectedPawnDefinition()
{
	SendMessagePlayerPawnSelected();

	if (!SelectedPawnDefinition)
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("OnRep_SelectedPawnDefinition(): Not valid class"))
		return;
	}

	RegisterOrCallOnPawnDataLoaded();

	OnPawnDefinitionUpdated.Broadcast(SelectedPawnDefinition);

	PawnCameraMode = SelectedPawnDefinition->DefaultCameraMode.LoadSynchronous()->GetClass();
}

void UPlayerPawnManager::RegisterOrCallOnPawnDataLoaded()
{
	if (HasAuthority())
	{
		AModularPlayerState* ModularPS = Cast<AModularPlayerState>(GetOwner());

		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: wait pawn data loaded: %s"), *GetNameSafe(GetOwner()));

		ModularPS->CallOrRegister_OnPawnDataReady(FOnPawnDataReady::FDelegate::CreateUObject(this, &UPlayerPawnManager::OnPawnDataLoaded));
	}
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

void UPlayerPawnManager::OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	if (HasAuthority())
	{
		UE_LOG(LogPlayerPawnManager, Warning, TEXT("[Initialize]: Pawn set for: %s"), *GetNameSafe(GetOwner()));

		OwnerPS->OnPawnSet.RemoveDynamic(this, &UPlayerPawnManager::OnPawnSet);

		if (UModularPawnExtensionComponent* HeroComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(OwnerPS->GetPawn()))
		{
			if (!HeroComp->HasReachedInitState(ModularGameplayTags::InitState_GameplayReady))
			{
				UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: Pawn not ready to gameplay on pawn set: %s"),
					*GetNameSafe(GetOwner()));

				const auto& AscIntializeHandle =
					FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized);
				OnAbilitySystemInitializedHandle = AscIntializeHandle.GetHandle();
				HeroComp->OnAbilitySystemInitialized_RegisterAndCall(AscIntializeHandle);

				return;
			}
		}
		else
		{
			UE_LOG(LogPlayerPawnManager, Error, TEXT("[Initialize]: not found Hero component on pawn set: %s"), *GetNameSafe(GetOwner()));
		}

		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: Pawn ready to initialize on pawn set: %s"), *GetNameSafe(GetOwner()));

		TryAddPawnPartComponent();
		TryAddPawnAbilities();
		TryAddPawnComponents();
		TryActivateFragments();
	}
}

void UPlayerPawnManager::OnAbilitySystemInitialized()
{
	if (HasAuthority())
	{
		UE_LOG(LogPlayerPawnManager, Display, TEXT("[Initialize]: Pawn ready to initialize on ability system ready: %s"),
			*GetNameSafe(GetOwner()));

		TryAddPawnPartComponent();
		TryAddPawnAbilities();
		TryAddPawnComponents();
		TryActivateFragments();
	}
}
