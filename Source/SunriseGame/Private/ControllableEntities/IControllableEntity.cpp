// Fill out your copyright notice in the Description page of Project Settings.


#include "ControllableEntities/IControllableEntity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IControllableEntity)

void IIControllableEntity::ConditionalBroadcastControllingAgentChange(const TScriptInterface<IIControllableEntity>& This,
	const TScriptInterface<IIControllableEntity>& OldControllingAgent, const TScriptInterface<IIControllableEntity>& NewControllingAgent)
{
	if (OldControllingAgent != NewControllingAgent)
	{
		if (IIControllableEntity* Interface = This.GetInterface())
		{
			if (FOnControllingAgentChanged* Delegate = Interface->GetOnControllingAgentChangedDelegate())
			{
				Delegate->Broadcast(This, OldControllingAgent, NewControllingAgent);
			}
		}
	}
}
