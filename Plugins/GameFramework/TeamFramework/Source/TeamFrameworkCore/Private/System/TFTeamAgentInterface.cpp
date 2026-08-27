#include "System/TFTeamAgentInterface.h"

void ITFTeamAgentInterface::ConditionalBroadcastTeamChanged(
	UObject* TeamAgent, ITFTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId)
{
	if (TeamAgent && TeamInterface && OldTeamId != NewTeamId)
	{
		if (FOnTFTeamIndexChangedDelegate* Delegate = TeamInterface->GetOnTeamIndexChangedDelegate())
		{
			Delegate->Broadcast(TeamAgent, TFTeamIdToInteger(OldTeamId), TFTeamIdToInteger(NewTeamId));
		}
	}
}
