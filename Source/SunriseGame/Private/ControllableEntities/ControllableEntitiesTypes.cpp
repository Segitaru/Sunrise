// Fill out your copyright notice in the Description page of Project Settings.


#include "ControllableEntities/ControllableEntitiesTypes.h"

#include <GameFramework/PlayerState.h>

#include "ControllableEntities/IControllableEntity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ControllableEntitiesTypes)

APlayerState* UControllableEntitiesTypes::GetControllingPlayerState(TSubclassOf<APlayerState> PlayerStateClass, UObject* Source)
{
	if (IIControllableEntity* SourceInterface = Cast<IIControllableEntity>(Source))
	{
		APlayerState* Result = GetPlayerStateFromObject(PlayerStateClass, SourceInterface->GetControllingAgent().GetObject());
		return Result && (!PlayerStateClass || Result->IsA(PlayerStateClass)) ? Result : nullptr;
	}

	return nullptr;
}

APlayerState* UControllableEntitiesTypes::GetPlayerStateFromObject(TSubclassOf<APlayerState> PlayerStateClass, UObject* Source)
{
	if (APawn* TargetPawn = Cast<APawn>(Source))
	{
		if (APlayerState* TargetPS = TargetPawn->GetPlayerState())
		{
			return TargetPS;
		}
	}

	if (AController* PC = Cast<AController>(Source))
	{
		if (APlayerState* TargetPS = PC->GetPlayerState<APlayerState>())
		{
			return TargetPS;
		}
	}

	if (APlayerState* TargetPS = Cast<APlayerState>(Source))
	{
		return TargetPS;
	}

	return nullptr;
}
