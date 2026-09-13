#pragma once

#include "GenericTeamAgentInterface.h"

#include "ModularTeamAgentInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamId, int32, NewTeamId);

inline int32 GenericTeamIdToInteger(FGenericTeamId TeamId)
{
	return TeamId == FGenericTeamId::NoTeam ? INDEX_NONE : static_cast<int32>(TeamId.GetId());
}

inline FGenericTeamId IntegerToGenericTeamId(int32 TeamId)
{
	return TeamId == INDEX_NONE ? FGenericTeamId::NoTeam : FGenericTeamId(static_cast<uint8>(TeamId));
}

/** Common team contract for Modular actors and actor components. */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UModularTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_BODY()
};

class MODULARGAMEPLAYACTORS_API IModularTeamAgentInterface : public IGenericTeamAgentInterface
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
		UObject* TeamAgent, IModularTeamAgentInterface* TeamInterface, FGenericTeamId OldTeamId, FGenericTeamId NewTeamId);
};
