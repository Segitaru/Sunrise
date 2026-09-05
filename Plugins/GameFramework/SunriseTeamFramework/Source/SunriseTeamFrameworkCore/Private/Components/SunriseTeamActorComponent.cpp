#include "Components/SunriseTeamActorComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

USunriseTeamActorComponent::USunriseTeamActorComponent()
{
	SetIsReplicatedByDefault(true);
}

bool USunriseTeamActorComponent::SetTeamId(int32 NewTeamId)
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
	const FGenericTeamId NewGenericTeamId = IntegerToSunriseTeamId(NewTeamId);
	if (TeamId == NewGenericTeamId)
	{
		return false;
	}
	const FGenericTeamId PreviousTeamId = TeamId;
	TeamId = NewGenericTeamId;
	ISunriseTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
	return true;
}

void USunriseTeamActorComponent::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	SetTeamId(SunriseTeamIdToInteger(NewTeamId));
}

void USunriseTeamActorComponent::OnRep_TeamId(FGenericTeamId PreviousTeamId)
{
	ISunriseTeamAgentInterface::ConditionalBroadcastTeamChanged(this, this, PreviousTeamId, TeamId);
}

void USunriseTeamActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USunriseTeamActorComponent, TeamId);
}
