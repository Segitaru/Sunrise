// Fill out your copyright notice in the Description page of Project Settings.


#include "Rosters/Systems/GamePawnRosterSubsystem.h"

UGamePawnRosterSubsystem::UGamePawnRosterSubsystem()
{
}

void UGamePawnRosterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGamePawnRosterSubsystem::Deinitialize()
{
	Super::Deinitialize();
	PlayersWithPayload.Empty();
}

