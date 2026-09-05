// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/Player/PickRule/PickModeRule_FFA.h"

#include "ModularPawnData.h"
#include "Rosters/Components/GamePawnRosterComponent.h"
#include "Rosters/Player/Components/PlayerPawnManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PickModeRule_FFA)

bool UPickModeRule_FFA::TryTakePawnFromPool(const UObject* Instigator,
	const TObjectPtr<UModularPawnData> TakingPawn, int32& PoolId,
	TObjectPtr<UModularPawnData>& ReleasedPawn)
{
	int32 LocalPoolId = -1;
	if (!GetTeamIdFromTarget(Instigator, LocalPoolId))
	{
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

	if (!PawnManager->CheckContainPawnInPool(LocalPoolId, TakingPawn))
	{
		return false;
	}

	if (const AActor* InstigatorActor = Cast<AActor>(Instigator))
	{
		if (UPlayerPawnManager* PlayerPawnManager = InstigatorActor->FindComponentByClass<UPlayerPawnManager>())
		{
			PlayerPawnManager->SetSelectedPawnDefinition(TakingPawn);
		}
	}
	return true;
}

bool UPickModeRule_FFA::TryTakeRandomPawnFromPool(const UObject* Instigator, int32& PoolId,
	TObjectPtr<UModularPawnData>& TakingPawn, TObjectPtr<UModularPawnData>& ReleasedPawn)
{
	int32 LocalPoolId = -1;
	if (!GetTeamIdFromTarget(Instigator, LocalPoolId))
	{
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
	}

	RandomPawn = PawnManager->GetRandomPawnFromPool(LocalPoolId);
	if (!RandomPawn)
	{
		return false;
	}

	if (const AActor* InstigatorActor = Cast<AActor>(Instigator))
	{
		if (UPlayerPawnManager* PlayerPawnManager = InstigatorActor->FindComponentByClass<UPlayerPawnManager>())
		{
			PlayerPawnManager->SetSelectedPawnDefinition(RandomPawn);
			PlayerPawnManager->CommitRandomPawn();
		}
	}
	return true;
}
