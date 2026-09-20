#include "Teams/System/ModularTeamAgentInterface.h"

void IModularTeamAgentInterface::ConditionalBroadcastTeamChanged(
	UObject* TeamAgent, IModularTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId)
{
	if (TeamAgent && TeamInterface && OldTeamId != NewTeamId)
	{
		if (FOnTeamIndexChangedDelegate* Delegate = TeamInterface->GetOnTeamIndexChangedDelegate())
		{
			Delegate->Broadcast(TeamAgent, GenericTeamIdToInteger(OldTeamId), GenericTeamIdToInteger(NewTeamId));
		}
	}
}
