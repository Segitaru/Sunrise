// Copyright Epic Games, Inc. All Rights Reserved.

#include "Teams/Components/ModularTeamCreationComponent.h"

#include "Engine/World.h"
#include "GameFeatures/Components/ExperienceManagerComponent.h"
#include "GameModes/ModularGameMode.h"
#include "Player/ModularPlayerState.h"
#include "Teams/Data/ModularTeamPrivateInfo.h"
#include "Teams/Data/ModularTeamPublicInfo.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


#include <GenericTeamAgentInterface.h>


#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularTeamCreationComponent)

UModularTeamCreationComponent::UModularTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PublicTeamInfoClass = AModularTeamPublicInfo::StaticClass();
	PrivateTeamInfoClass = AModularTeamPrivateInfo::StaticClass();
}

#if WITH_EDITOR
EDataValidationResult UModularTeamCreationComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	//@TODO: TEAMS: Validate that all display assets have the same properties set!

	return Result;
}
#endif

void UModularTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	UExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(
		FOnExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UModularTeamCreationComponent::OnExperienceLoaded(const UExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateTeams();
		ServerAssignPlayersToTeams();
	}
#endif
}

#if WITH_SERVER_CODE

void UModularTeamCreationComponent::ServerCreateTeams()
{
	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		ServerCreateTeam(TeamId, KVP.Value);
	}
}

void UModularTeamCreationComponent::ServerAssignPlayersToTeams()
{
	// Assign players that already exist to teams
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AModularPlayerState* ModularPS = Cast<AModularPlayerState>(PS))
		{
			ServerChooseTeamForPlayer(ModularPS);
		}
	}

	// Listen for new players logging in
	AModularGameModeBase* GameMode = Cast<AModularGameModeBase>(GameState->AuthorityGameMode);
	check(GameMode);

	GameMode->OnGameModePlayerInitialized.AddUObject(this, &ThisClass::OnPlayerInitialized);
}

void UModularTeamCreationComponent::ServerChooseTeamForPlayer(AModularPlayerState* PS)
{
	if (PS->IsOnlyASpectator())
	{
		PS->SetGenericTeamId(FGenericTeamId::NoTeam);
	}
	else
	{
		const FGenericTeamId TeamID = IntegerToGenericTeamId(GetLeastPopulatedTeamID());
		PS->SetGenericTeamId(TeamID);
	}
}

void UModularTeamCreationComponent::OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer)
{
	check(NewPlayer);
	check(NewPlayer->PlayerState);
	if (AModularPlayerState* ModularPS = Cast<AModularPlayerState>(NewPlayer->PlayerState))
	{
		ServerChooseTeamForPlayer(ModularPS);
	}
}

void UModularTeamCreationComponent::ServerCreateTeam(int32 TeamId, UModularTeamDisplayAsset* DisplayAsset)
{
	check(HasAuthority());

	//@TODO: ensure the team doesn't already exist

	UWorld* World = GetWorld();
	check(World);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AModularTeamPublicInfo* NewModular = World->SpawnActor<AModularTeamPublicInfo>(PublicTeamInfoClass, SpawnInfo);
	checkf(NewModular != nullptr, TEXT("Failed to create public team actor from class %s"), *GetPathNameSafe(*PublicTeamInfoClass));
	NewModular->SetTeamId(TeamId);
	NewModular->SetTeamDisplayAsset(DisplayAsset);

	AModularTeamPrivateInfo* NewTeamPrivateInfo = World->SpawnActor<AModularTeamPrivateInfo>(PrivateTeamInfoClass, SpawnInfo);
	checkf(
		NewTeamPrivateInfo != nullptr, TEXT("Failed to create private team actor from class %s"), *GetPathNameSafe(*PrivateTeamInfoClass));
	NewTeamPrivateInfo->SetTeamId(TeamId);
}

int32 UModularTeamCreationComponent::GetLeastPopulatedTeamID() const
{
	const int32 NumTeams = TeamsToCreate.Num();
	if (NumTeams > 0)
	{
		TMap<int32, uint32> TeamMemberCounts;
		TeamMemberCounts.Reserve(NumTeams);

		for (const auto& KVP : TeamsToCreate)
		{
			const int32 TeamId = KVP.Key;
			TeamMemberCounts.Add(TeamId, 0);
		}

		AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (AModularPlayerState* ModularPS = Cast<AModularPlayerState>(PS))
			{
				const int32 PlayerTeamID = ModularPS->GetTeamId();

				if ((PlayerTeamID != INDEX_NONE) && !ModularPS->IsInactive()) // do not count unassigned or disconnected players
				{
					check(TeamMemberCounts.Contains(PlayerTeamID)) TeamMemberCounts[PlayerTeamID] += 1;
				}
			}
		}

		// sort by lowest team population, then by team ID
		int32 BestTeamId = INDEX_NONE;
		uint32 BestPlayerCount = TNumericLimits<uint32>::Max();
		for (const auto& KVP : TeamMemberCounts)
		{
			const int32 TestTeamId = KVP.Key;
			const uint32 TestTeamPlayerCount = KVP.Value;

			if (TestTeamPlayerCount < BestPlayerCount)
			{
				BestTeamId = TestTeamId;
				BestPlayerCount = TestTeamPlayerCount;
			}
			else if (TestTeamPlayerCount == BestPlayerCount)
			{
				if ((TestTeamId < BestTeamId) || (BestTeamId == INDEX_NONE))
				{
					BestTeamId = TestTeamId;
					BestPlayerCount = TestTeamPlayerCount;
				}
			}
		}

		return BestTeamId;
	}

	return INDEX_NONE;
}
#endif // WITH_SERVER_CODE
