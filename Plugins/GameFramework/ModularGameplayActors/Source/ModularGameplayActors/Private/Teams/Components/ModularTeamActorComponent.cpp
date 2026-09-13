#include "Teams/Components/ModularTeamActorComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UModularTeamActorComponent::UModularTeamActorComponent()
{
	SetIsReplicatedByDefault(true);
}

void UModularTeamActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UModularTeamActorComponent, TeamId);
}

int32 UModularTeamActorComponent::GetTeamId() const
{
	return GenericTeamIdToInteger(TeamId);
}

bool UModularTeamActorComponent::SetTeamId(int32 NewTeamId)
{
	if (NewTeamId < INDEX_NONE || NewTeamId > MAX_uint8 - 1)
	{
		return false;
	}
	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasActorBegunPlay() && !Owner->HasAuthority())
	{
		return false;
	}
	const FGenericTeamId NewGenericTeamId = IntegerToGenericTeamId(NewTeamId);
	if (TeamId == NewGenericTeamId)
	{
		return false;
	}
	const FGenericTeamId PreviousTeamId = TeamId;
	TeamId = NewGenericTeamId;
	IModularTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
	return true;
}

void UModularTeamActorComponent::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	SetTeamId(GenericTeamIdToInteger(NewTeamId));
}

void UModularTeamActorComponent::OnRep_TeamId(FGenericTeamId PreviousTeamId)
{
	IModularTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
}
