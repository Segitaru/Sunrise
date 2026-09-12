// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/ModularGameMode.h"

#include "AssetRegistry/AssetData.h"
#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameFeatures/ExperienceDefinition.h"
#include "GameFeatures/UserFacingExperienceDefinition.h"
#include "GameFramework/GameStateBase.h"
#include "GameMapsSettings.h"
#include "GameMode/ModularGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "ModularAIController.h"
#include "ModularLogChannels.h"
#include "ModularPawnData.h"
#include "Pawn/Components/ModularPawnExtensionComponent.h"
#include "Pawn/ModularPawn.h"
#include "Player/ModularPlayerController.h"
#include "Player/ModularPlayerSpawningManagerComponent.h"
#include "Player/ModularPlayerState.h"
#include "Settings/ModularDeveloperSettings.h"
#include "Settings/ModularWorldSettings.h"
#include "System/ModularAssetManager.h"
#include "System/ModularGameSession.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularGameMode)

AModularGameModeBase::AModularGameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AModularGameStateBase::StaticClass();
	PlayerControllerClass = AModularPlayerController::StaticClass();
	PlayerStateClass = AModularPlayerState::StaticClass();
	DefaultPawnClass = AModularPawn::StaticClass();
	GameSessionClass = AModularGameSession::StaticClass();
	ReplaySpectatorPlayerControllerClass = AModularReplayPlayerController::StaticClass();
}

const UModularPawnData* AModularGameModeBase::GetPawnDataForController(const AController* InController) const
{
	// See if pawn data is already set on the player state
	if (InController != nullptr)
	{
		if (const AModularPlayerState* ModularPS = InController->GetPlayerState<AModularPlayerState>())
		{
			if (const UModularPawnData* PawnData = ModularPS->GetPawnData<UModularPawnData>())
			{
				return PawnData;
			}
		}
	}

	// If not, fall back to the default for the current experience
	check(GameState);
	UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();
	check(ExperienceComponent);

	if (ExperienceComponent->IsExperienceLoaded())
	{
		const UExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData.ToSoftObjectPath().IsValid())
		{
			return Experience->DefaultPawnData.LoadSynchronous();
		}

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		return UModularAssetManager::Get().GetDefaultPawnData();
	}

	// Experience not loaded yet, so there is no pawn data to be had
	return nullptr;
}

void AModularGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Wait for the next frame to give time to initialize startup settings
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void AModularGameModeBase::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;

	// Precedence order (highest wins)
	//  - Matchmaking assignment (if present)
	//  - URL Options override
	//  - Developer Settings (PIE only)
	//  - Command Line override
	//  - World Settings
	//  - Dedicated server
	//  - Default experience

	UWorld* World = GetWorld();

	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UModularDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}

	// see if the command line wants to set the experience
	if (!ExperienceId.IsValid())
	{
		FString ExperienceFromCommandLine;
		if (FParse::Value(FCommandLine::Get(), TEXT("Experience="), ExperienceFromCommandLine))
		{
			ExperienceId = FPrimaryAssetId::ParseTypeAndName(ExperienceFromCommandLine);
			if (!ExperienceId.PrimaryAssetType.IsValid())
			{
				ExperienceId =
					FPrimaryAssetId(FPrimaryAssetType(UExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromCommandLine));
			}
			ExperienceIdSource = TEXT("CommandLine");
		}
	}

	// see if the world settings has a default experience
	if (!ExperienceId.IsValid())
	{
		if (AModularWorldSettings* TypedWorldSettings = Cast<AModularWorldSettings>(GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	UModularAssetManager& AssetManager = UModularAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogExperienceFramework, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"),
			*ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}

	// Final fallback to the default experience
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// This will start to host as a dedicated server
			return;
		}

		//@TODO: Pull this from a config setting or something
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("ModularExperienceDefinition"), FName("B_ModularDefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

bool AModularGameModeBase::TryDedicatedServerLogin()
{
	// Some basic code to register as an active dedicated server, this would be heavily modified by the game
	FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	{
		// Only register if this is the default map on a dedicated server
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

		// Dedicated servers may need to do an online login
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &AModularGameModeBase::OnUserInitializedForDedicatedServer);

		// There are no local users on dedicated server, but index 0 means the default platform user which is handled by the online login code
		if (!UserSubsystem->TryToLoginForOnlinePlay(0))
		{
			OnUserInitializedForDedicatedServer(
				nullptr, false, FText(), ECommonUserPrivilege::CanPlayOnline, ECommonUserOnlineContext::Default);
		}

		return true;
	}

	return false;
}

void AModularGameModeBase::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
{
	FPrimaryAssetType UserExperienceType = UUserFacingExperienceDefinition::StaticClass()->GetFName();

	// Figure out what UserFacingExperience to load
	FPrimaryAssetId UserExperienceId;
	FString UserExperienceFromCommandLine;
	if (FParse::Value(FCommandLine::Get(), TEXT("UserExperience="), UserExperienceFromCommandLine) ||
		FParse::Value(FCommandLine::Get(), TEXT("Playlist="), UserExperienceFromCommandLine))
	{
		UserExperienceId = FPrimaryAssetId::ParseTypeAndName(UserExperienceFromCommandLine);
		if (!UserExperienceId.PrimaryAssetType.IsValid())
		{
			UserExperienceId = FPrimaryAssetId(FPrimaryAssetType(UserExperienceType), FName(*UserExperienceFromCommandLine));
		}
	}

	// Search for the matching experience, it's fine to force load them because we're in dedicated server startup
	UModularAssetManager& AssetManager = UModularAssetManager::Get();
	TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssetsWithType(UserExperienceType);
	if (ensure(Handle.IsValid()))
	{
		Handle->WaitUntilComplete();
	}

	TArray<UObject*> UserExperiences;
	AssetManager.GetPrimaryAssetObjectList(UserExperienceType, UserExperiences);
	UUserFacingExperienceDefinition* FoundExperience = nullptr;
	UUserFacingExperienceDefinition* DefaultExperience = nullptr;

	for (UObject* Object : UserExperiences)
	{
		UUserFacingExperienceDefinition* UserExperience = Cast<UUserFacingExperienceDefinition>(Object);
		if (ensure(UserExperience))
		{
			if (UserExperience->GetPrimaryAssetId() == UserExperienceId)
			{
				FoundExperience = UserExperience;
				break;
			}

			if (UserExperience->bIsDefaultExperience && DefaultExperience == nullptr)
			{
				DefaultExperience = UserExperience;
			}
		}
	}

	if (FoundExperience == nullptr)
	{
		FoundExperience = DefaultExperience;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (ensure(FoundExperience && GameInstance))
	{
		// Actually host the game
		UCommonSession_HostSessionRequest* HostRequest = FoundExperience->CreateHostingRequest(this);
		if (ensure(HostRequest))
		{
			HostRequest->OnlineMode = OnlineMode;

			// TODO override other parameters?

			UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
			SessionSubsystem->HostSession(nullptr, HostRequest);

			// This will handle the map travel
		}
	}
}

void AModularGameModeBase::OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		// Unbind
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &AModularGameModeBase::OnUserInitializedForDedicatedServer);

		// Dedicated servers do not require user login, but some online subsystems may expect it
		if (bSuccess && ensure(UserInfo))
		{
			UE_LOG(LogExperienceFramework, Log, TEXT("Dedicated server user login succeeded for id %s, starting online server"),
				*UserInfo->GetNetId().ToString());
		}
		else
		{
			UE_LOG(LogExperienceFramework, Log,
				TEXT("Dedicated server user login unsuccessful, starting online server as login is not required"));
		}

		HostDedicatedServerMatch(ECommonSessionOnlineMode::Online);
	}
}

void AModularGameModeBase::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogExperienceFramework, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogExperienceFramework, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void AModularGameModeBase::OnExperienceLoaded(const UExperienceDefinition* CurrentExperience)
{
	// Spawn any players that are already attached
	//@TODO: Here we're handling only *player* controllers, but in GetDefaultPawnClassForController_Implementation we skipped all controllers
	// GetDefaultPawnClassForController_Implementation might only be getting called for players anyways
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

bool AModularGameModeBase::IsExperienceLoaded() const
{
	check(GameState);
	UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

UClass* AModularGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UModularPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass.LoadSynchronous();
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* AModularGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient; // Never save the default player pawns into a map.
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const UModularPawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
				else
				{
					UE_LOG(LogExperienceFramework, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."),
						*GetNameSafe(SpawnedPawn));
				}
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
		else
		{
			UE_LOG(LogExperienceFramework, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."),
				*GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		}
	}
	else
	{
		UE_LOG(LogExperienceFramework, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
	}

	return nullptr;
}

bool AModularGameModeBase::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to use the start spot, always use the spawn management component.
	return false;
}

void AModularGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Delay starting new players until the experience has been loaded
	// (players who log in prior to that will be started by OnExperienceLoaded)
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

AActor* AModularGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UModularPlayerSpawningManagerComponent* PlayerSpawningComponent =
			GameState->FindComponentByClass<UModularPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AModularGameModeBase::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UModularPlayerSpawningManagerComponent* PlayerSpawningComponent =
			GameState->FindComponentByClass<UModularPlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AModularGameModeBase::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

bool AModularGameModeBase::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			return false;
		}
	}

	if (UModularPlayerSpawningManagerComponent* PlayerSpawningComponent =
			GameState->FindComponentByClass<UModularPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ControllerCanRestart(Controller);
	}

	return true;
}

void AModularGameModeBase::InitGameState()
{
	Super::InitGameState();

	// Listen for the experience load to complete
	UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(
		FOnExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void AModularGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);

	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AModularGameModeBase::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	else if (AModularAIController* BotController = Cast<AModularAIController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &AModularAIController::ServerRestartController);
	}
}

bool AModularGameModeBase::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	// Do nothing, we'll wait until PostLogin when we try to spawn the player for real.
	// Doing anything right now is no good, systems like team assignment haven't even occurred yet.
	return true;
}

void AModularGameModeBase::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// If we tried to spawn a pawn and it failed, lets try again *note* check if there's actually a pawn class
	// before we try this forever.
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			// If it's a player don't loop forever, maybe something changed and they can no longer restart if so stop trying.
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);
			}
			else
			{
				UE_LOG(LogExperienceFramework, Verbose,
					TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."),
					*GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogExperienceFramework, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."),
			*GetPathNameSafe(NewPlayer));
	}
}

AModularGameMode::AModularGameMode(const FObjectInitializer& ObjectInitializer)
{
}