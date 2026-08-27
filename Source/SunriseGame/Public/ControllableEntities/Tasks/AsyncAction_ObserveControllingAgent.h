// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ControllableEntities/IControllableEntity.h"
#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"

#include "AsyncAction_ObserveControllingAgent.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API UAsyncAction_ObserveControllingAgent : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	// Watches for team changes on the specified team agent
	//  - It will will fire once immediately to give the current team assignment
	//  - For anything whose controlling agent can change at runtime
	//    it will also listen for team assignment changes in the future
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Keywords = "Watch"))
	static UAsyncAction_ObserveControllingAgent* ObserveControllingAgent(UObject* ControllableEntity);

	//~UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	//~End of UBlueprintAsyncActionBase interface

	// Called when the team is set or changed
	UPROPERTY(BlueprintAssignable)
	FOnControllingAgentChanged OnControllingAgentChanged;

private:
	// Watches for team changes on the specified team actor
	static UAsyncAction_ObserveControllingAgent* InternalObserveControllingAgent(
		const TScriptInterface<IIControllableEntity>& ControllableEntity);

	UFUNCTION()
	void OnWatchedAgentChangedControllingAgent(TScriptInterface<IIControllableEntity> ObjectChangingAgent,
		TScriptInterface<IIControllableEntity> OldControllingAgent, TScriptInterface<IIControllableEntity> NewControllingAgent);

	TWeakInterfacePtr<IIControllableEntity> TeamInterfacePtr;
};
