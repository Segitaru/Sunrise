// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/Player/PickRule/PickModeRule.h"

#include "GameFramework/GameStateBase.h"
#include "ModularPawnData.h"
#include "Rosters/Components/GamePawnRosterComponent.h"
#include "Rosters/Player/Components/PlayerPawnManager.h"
#include "Teams/System/ModularTeamSubsystem.h"


DEFINE_LOG_CATEGORY_STATIC(LogPickModeRule, All, All)

#include UE_INLINE_GENERATED_CPP_BY_NAME(PickModeRule)

UPickModeRule::UPickModeRule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UGamePawnRosterComponent* UPickModeRule::GetPawnManagerComponent() const
{
	if (!GetOuter())
	{
		return nullptr;
	}
	const auto World = GetWorld();

	const AGameStateBase* GameStateRef = World->GetGameState();
	if (!GameStateRef)
	{
		return nullptr;
	}

	UGamePawnRosterComponent* PawnManager = GameStateRef->FindComponentByClass<UGamePawnRosterComponent>();

	return PawnManager;
}

bool UPickModeRule::GetTeamIdFromTarget(const UObject* TargetObject, int32& CurrentTeamId) const
{
	if (!GetWorld())
	{
		ensureMsgf(false, TEXT("%s: Critical Error: World does not exist"), *GetPathNameSafe(this));
	}

	const UModularTeamSubsystem* const TeamSubsystem = GetWorld()->GetSubsystem<UModularTeamSubsystem>();

	if (!IsValid(TeamSubsystem))
	{
		ensureMsgf(false, TEXT("%s: Critical Error: Team Subsystem does not exist"), *GetPathNameSafe(this));
	}

	bool bIsPartOfTeam = false;

	TeamSubsystem->FindTeamFromActor(TargetObject, bIsPartOfTeam, CurrentTeamId);

	return bIsPartOfTeam;
}

bool UPickModeRule::TryTakePawnFromPool(
	const UObject* Instigator, const TObjectPtr<UModularPawnData> TakingPawn, int32& PoolId, TObjectPtr<UModularPawnData>& ReleasedPawn)
{
	if (!GetTeamIdFromTarget(Instigator, PoolId))
	{
		UE_LOG(LogPickModeRule, Warning, TEXT("Not Part of team"));
		return false;
	}

	UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent();
	if (!PawnManager)
	{
		ensureMsgf(false,
			TEXT("%s: Critical Error: Game Pawn Roster Component does not exist, "
				 " when try take pawn from pool!"),
			*GetPathNameSafe(this));
	}

	if (!PawnManager->RemovePawnFromPool(PoolId, TakingPawn))
	{
		UE_LOG(LogPickModeRule, Display, TEXT("The selected pawn is already taken or is not in the roster"));

		return false;
	}

	if (const AActor* InstigatorActor = Cast<AActor>(Instigator))
	{
		if (UPlayerPawnManager* PlayerPawnManager = InstigatorActor->FindComponentByClass<UPlayerPawnManager>())
		{
			ReleasedPawn = PlayerPawnManager->GetSelectedPawnDefinition();
			if (ReleasedPawn)
			{
				if (!PawnManager->ReleasePawnInPool(PoolId, ReleasedPawn))
				{
					UE_LOG(LogPickModeRule, Warning, TEXT("Try release not actual pawn in pool"));
				}
			}

			PlayerPawnManager->SetSelectedPawnDefinition(TakingPawn);
		}
	}

	return true;
}

bool UPickModeRule::TryTakeRandomPawnFromPool(
	const UObject* Instigator, int32& PoolId, TObjectPtr<UModularPawnData>& TakingPawn, TObjectPtr<UModularPawnData>& ReleasedPawn)
{
	if (!GetTeamIdFromTarget(Instigator, PoolId))
	{
		UE_LOG(LogPickModeRule, Warning, TEXT("Not Part of team"));
		return false;
	}

	TObjectPtr<UModularPawnData> RandomPawn = nullptr;

	UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent();
	if (!PawnManager)
	{
		ensureMsgf(false,
			TEXT("%s: Critical Error: Game Pawn Roster Component does not exist, "
				 " when try take random pawn from pool!"),
			*GetPathNameSafe(this));
		return false;
	}

	RandomPawn = PawnManager->GetRandomPawnFromPool(PoolId);
	if (!RandomPawn)
	{
		UE_LOG(LogPickModeRule, Display, TEXT("The random pawn is already taken or is not in the roster"));
		return false;
	}

	if (!PawnManager->RemovePawnFromPool(PoolId, RandomPawn))
	{
		return false;
	}

	TakingPawn = RandomPawn;


	// TODO: Replace in correct place (return pawn in pool)
	if (const AActor* InstigatorActor = Cast<AActor>(Instigator))
	{
		if (UPlayerPawnManager* PlayerPawnManager = InstigatorActor->FindComponentByClass<UPlayerPawnManager>())
		{
			if (const TObjectPtr<UModularPawnData> OldPawnInfo = PlayerPawnManager->GetSelectedPawnDefinition())
			{
				if (PawnManager->ReleasePawnInPool(PoolId, OldPawnInfo))
				{
					ReleasedPawn = OldPawnInfo;
				}
				else
				{
					UE_LOG(LogPickModeRule, Warning, TEXT("Try release not actual pawn in pool"));
				}
			}
			PlayerPawnManager->SetSelectedPawnDefinition(RandomPawn);
			PlayerPawnManager->CommitRandomPawn();
		}
	}
	return true;
}
bool UPickModeRule::TryTakePawnFromPoolByClass(const UObject* Instigator, int32& PoolId, TSubclassOf<UObject> ClassToSearch,
	TObjectPtr<UModularPawnData>& TakingPawn, TObjectPtr<UModularPawnData>& ReleasedPawn)
{

	if (!GetTeamIdFromTarget(Instigator, PoolId))
	{
		UE_LOG(LogPickModeRule, Warning, TEXT("Not Part of team"));
		return false;
	}

	UModularPawnData* FoundPawn = nullptr;

	UGamePawnRosterComponent* PawnManager = GetPawnManagerComponent();
	if (!PawnManager)
	{
		ensureMsgf(false,
			TEXT("%s: Critical Error: Game Pawn Roster Component does not exist, "
				 " when try take random pawn from pool!"),
			*GetPathNameSafe(this));
		return false;
	}

	FoundPawn = PawnManager->GetPawnFromPoolByClass(PoolId, ClassToSearch);
	if (!FoundPawn)
	{
		UE_LOG(LogPickModeRule, Display, TEXT("Cant find pawn, is already taken or is not in the roster"));
		return false;
	}

	if (!PawnManager->RemovePawnFromPool(PoolId, FoundPawn))
	{
		return false;
	}

	TakingPawn = FoundPawn;


	// TODO: Replace in correct place (return pawn in pool)
	if (const AActor* InstigatorActor = Cast<AActor>(Instigator))
	{
		if (UPlayerPawnManager* PlayerPawnManager = InstigatorActor->FindComponentByClass<UPlayerPawnManager>())
		{
			if (UModularPawnData* OldPawnInfo = PlayerPawnManager->GetSelectedPawnDefinition())
			{
				if (PawnManager->ReleasePawnInPool(PoolId, OldPawnInfo))
				{
					ReleasedPawn = OldPawnInfo;
				}
				else
				{
					UE_LOG(LogPickModeRule, Warning, TEXT("Try release not actual pawn in pool"));
				}
			}
			PlayerPawnManager->SetSelectedPawnDefinition(FoundPawn);
			PlayerPawnManager->CommitRandomPawn();
		}
	}
	return true;
}

bool UPickModeRule::OnPawnConfirmed(const UObject* Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn, int32& PoolId,
	TArray<TObjectPtr<UModularPawnData>>& BlockedPawn)
{
	return true;
}
bool UPickModeRule::OnPawnReleased(const UObject* Instigator, const TObjectPtr<UModularPawnData> ConfirmedPawn, int32& PoolId,
	TArray<TObjectPtr<UModularPawnData>>& BlockedPawn)
{
	return true;
}
