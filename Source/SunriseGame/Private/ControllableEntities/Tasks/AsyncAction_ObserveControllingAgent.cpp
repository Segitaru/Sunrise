// Fill out your copyright notice in the Description page of Project Settings.

#include "ControllableEntities/Tasks/AsyncAction_ObserveControllingAgent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ObserveControllingAgent)

UAsyncAction_ObserveControllingAgent* UAsyncAction_ObserveControllingAgent::ObserveControllingAgent(UObject* ControllableEntity)
{
	return InternalObserveControllingAgent(ControllableEntity);
}

void UAsyncAction_ObserveControllingAgent::Activate()
{
	if (!TeamInterfacePtr.Get())
	{
		SetReadyToDestroy();
		return;
	}

	if (const auto Delegate = TeamInterfacePtr->GetOnControllingAgentChangedDelegate())
	{
		Delegate->AddDynamic(this, &ThisClass::OnWatchedAgentChangedControllingAgent);
	}

	// Broadcast once so users get the current state
	OnControllingAgentChanged.Broadcast(TeamInterfacePtr.GetObject(), nullptr, TeamInterfacePtr->GetControllingAgent());
}

void UAsyncAction_ObserveControllingAgent::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();

	// If we're being canceled, we need to unhook everything we might have tried listening to.
	if (IIControllableEntity* TeamInterface = TeamInterfacePtr.Get())
	{
		TeamInterface->GetOnControllingAgentChangedDelegate()->RemoveAll(this);
	}
}

UAsyncAction_ObserveControllingAgent* UAsyncAction_ObserveControllingAgent::InternalObserveControllingAgent(
	const TScriptInterface<IIControllableEntity>& ControllableEntity)
{
	UAsyncAction_ObserveControllingAgent* Action = nullptr;

	if (ControllableEntity != nullptr)
	{
		Action = NewObject<UAsyncAction_ObserveControllingAgent>();
		Action->TeamInterfacePtr = ControllableEntity;
		Action->RegisterWithGameInstance(ControllableEntity.GetObject());
	}

	return Action;
}

void UAsyncAction_ObserveControllingAgent::OnWatchedAgentChangedControllingAgent(TScriptInterface<IIControllableEntity> ObjectChangingAgent,
	TScriptInterface<IIControllableEntity> OldControllingAgent, TScriptInterface<IIControllableEntity> NewControllingAgent)
{
	OnControllingAgentChanged.Broadcast(ObjectChangingAgent, OldControllingAgent, NewControllingAgent);
}