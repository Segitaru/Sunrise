// Copyright Epic Games, Inc. All Rights Reserved.

#include "Teams/Data/ModularTeamInfoBase.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Teams/System/ModularTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularTeamInfoBase)

class FLifetimeProperty;

AModularTeamInfoBase::AModularTeamInfoBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TeamId(INDEX_NONE)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetPriority = 3.0f;
	SetReplicatingMovement(false);
}

void AModularTeamInfoBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamTags);
	DOREPLIFETIME_CONDITION(ThisClass, TeamId, COND_InitialOnly);
}

void AModularTeamInfoBase::BeginPlay()
{
	Super::BeginPlay();

	TryRegisterWithTeamSubsystem();
}

void AModularTeamInfoBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TeamId != INDEX_NONE)
	{
		UModularTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UModularTeamSubsystem>();
		if (TeamSubsystem)
		{
			// EndPlay can happen at weird times where the subsystem has already been destroyed
			TeamSubsystem->UnregisterTeamInfo(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AModularTeamInfoBase::RegisterWithTeamSubsystem(UModularTeamSubsystem* Subsystem)
{
	Subsystem->RegisterTeamInfo(this);
}

void AModularTeamInfoBase::TryRegisterWithTeamSubsystem()
{
	if (TeamId != INDEX_NONE)
	{
		UModularTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UModularTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void AModularTeamInfoBase::SetTeamId(int32 NewTeamId)
{
	check(HasAuthority());
	check(TeamId == INDEX_NONE);
	check(NewTeamId != INDEX_NONE);

	TeamId = NewTeamId;

	TryRegisterWithTeamSubsystem();
}

void AModularTeamInfoBase::OnRep_TeamId()
{
	TryRegisterWithTeamSubsystem();
}
