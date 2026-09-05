#include "System/SunriseTeamAgentInterface.h"

void ISunriseTeamAgentInterface::ConditionalBroadcastTeamChanged(
	UObject* TeamAgent, ISunriseTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId)
{
	if (TeamAgent && TeamInterface && OldTeamId != NewTeamId)
	{
		if (FOnTeamIndexChangedDelegate* Delegate = TeamInterface->GetOnTeamIndexChangedDelegate())
		{
			Delegate->Broadcast(TeamAgent, SunriseTeamIdToInteger(OldTeamId), SunriseTeamIdToInteger(NewTeamId));
		}
	}
}
