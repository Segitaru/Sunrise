// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/EFGameMatchComponent.h"

#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

UEFGameMatchComponent::UEFGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	RoundCounter = 0;
	TeamPointsPerMatch = {{1, 0}, {2, 0}};
	ExtraTimeForTeam = {{1, 0}, {2, 0}};
}

void UEFGameMatchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TeamData, SharedParams);

	DOREPLIFETIME(UEFGameMatchComponent, TeamPointsPerMatch);
	DOREPLIFETIME(UEFGameMatchComponent, ExtraTimeForTeam);
	DOREPLIFETIME(UEFGameMatchComponent, RoundCounter);
}

void UEFGameMatchComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UEFGameMatchComponent::SetTeamData(FGameMatchTeamData NewTeamData)
{
	TeamData = NewTeamData;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, TeamData, this);
}

void UEFGameMatchComponent::OnRep_TeamData()
{
}

void UEFGameMatchComponent::OnRep_RoundCounter()
{
}
