// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "IControllableEntity.generated.h"

class IIControllableEntity;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnControllingAgentChanged, TScriptInterface<IIControllableEntity>, ObjectChangingAgent,
	TScriptInterface<IIControllableEntity>, OldControllingAgent, TScriptInterface<IIControllableEntity>, NewControllingAgent);

// This class does not need to be modified.
UINTERFACE(Meta = (CannotImplementInterfaceInBlueprint = "true"))
class SUNRISEGAME_API UIControllableEntity : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SUNRISEGAME_API IIControllableEntity
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable)
	virtual TScriptInterface<IIControllableEntity> GetControllingAgent()
		PURE_VIRTUAL(IIControllableEntity::GetControllingAgent, return nullptr;);

	UFUNCTION(BlueprintCallable)
	virtual void SetControllingAgent(const TScriptInterface<IIControllableEntity> NewAgent)
		PURE_VIRTUAL(IIControllableEntity::SetControllingAgent, );

	virtual FOnControllingAgentChanged* GetOnControllingAgentChangedDelegate()
		PURE_VIRTUAL(IIControllableEntity::GetOnControllingAgentChangedDelegate, return nullptr;);

	static void ConditionalBroadcastControllingAgentChange(const TScriptInterface<IIControllableEntity>& This,
		const TScriptInterface<IIControllableEntity>& OldControllingAgent,
		const TScriptInterface<IIControllableEntity>& NewControllingAgent);
};
