#pragma once

#include "GenericTeamAgentInterface.h"

#include "TFTeamAgentInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnTFTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamId, int32, NewTeamId);

inline int32 TFTeamIdToInteger(FGenericTeamId TeamId)
{
	return TeamId == FGenericTeamId::NoTeam ? INDEX_NONE : static_cast<int32>(TeamId.GetId());
}

inline FGenericTeamId IntegerToTFTeamId(int32 TeamId)
{
	return TeamId == INDEX_NONE ? FGenericTeamId::NoTeam : FGenericTeamId(static_cast<uint8>(TeamId));
}

/** Common team contract for Sunrise actors and actor components. */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTFTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_BODY()
};

class TEAMFRAMEWORKCORE_API ITFTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FOnTFTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	FOnTFTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnTFTeamIndexChangedDelegate* Delegate = GetOnTeamIndexChangedDelegate();
		check(Delegate);
		return *Delegate;
	}

	static void ConditionalBroadcastTeamChanged(
		UObject* TeamAgent, ITFTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId);
};
