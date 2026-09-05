#pragma once

#include "GenericTeamAgentInterface.h"

#include "SunriseTeamAgentInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamId, int32, NewTeamId);

inline int32 SunriseTeamIdToInteger(FGenericTeamId TeamId)
{
	return TeamId == FGenericTeamId::NoTeam ? INDEX_NONE : static_cast<int32>(TeamId.GetId());
}

inline FGenericTeamId IntegerToSunriseTeamId(int32 TeamId)
{
	return TeamId == INDEX_NONE ? FGenericTeamId::NoTeam : FGenericTeamId(static_cast<uint8>(TeamId));
}

/** Common team contract for Sunrise actors and actor components. */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class USunriseTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_BODY()
};

class SUNRISETEAMFRAMEWORKCORE_API ISunriseTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	FOnTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnTeamIndexChangedDelegate* Delegate = GetOnTeamIndexChangedDelegate();
		check(Delegate);
		return *Delegate;
	}

	static void ConditionalBroadcastTeamChanged(
		UObject* TeamAgent, ISunriseTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId);
};
