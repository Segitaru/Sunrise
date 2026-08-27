#include "Components/TFTeamActorComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UTFTeamActorComponent::UTFTeamActorComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UTFTeamActorComponent::SetTeamId(int32 NewTeamId)
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
	const FGenericTeamId NewGenericTeamId = IntegerToTFTeamId(NewTeamId);
	if (TeamId == NewGenericTeamId)
	{
		return false;
	}
	const FGenericTeamId PreviousTeamId = TeamId;
	TeamId = NewGenericTeamId;
	ITFTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
	return true;
}

void UTFTeamActorComponent::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	SetTeamId(TFTeamIdToInteger(NewTeamId));
}

void UTFTeamActorComponent::OnRep_TeamId(FGenericTeamId PreviousTeamId)
{
	ITFTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
}

void UTFTeamActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTFTeamActorComponent, TeamId);
}
