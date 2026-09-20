// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFeatures/Components/ModularGameMatchComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

UModularGameMatchComponent::UModularGameMatchComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	RoundCounter = 0;
	TeamPointsPerMatch = {{1, 0}, {2, 0}};
	ExtraTimeForTeam = {{1, 0}, {2, 0}};
}

void UModularGameMatchComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TeamData, SharedParams);

	DOREPLIFETIME(UModularGameMatchComponent, TeamPointsPerMatch);
	DOREPLIFETIME(UModularGameMatchComponent, ExtraTimeForTeam);
	DOREPLIFETIME(UModularGameMatchComponent, RoundCounter);
}

void UModularGameMatchComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UModularGameMatchComponent::SetTeamData(FGameMatchTeamData NewTeamData)
{
	TeamData = NewTeamData;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, TeamData, this);
}

void UModularGameMatchComponent::OnRep_TeamData()
{
}

void UModularGameMatchComponent::OnRep_RoundCounter()
{
}
