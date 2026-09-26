// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncAction_PlayerBuildManagerReady.generated.h"


class UPlayerBuildManager;
class UWorld;
struct FFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerBuilderManagerReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFailed);
/**
 * Asynchronously waits for the game state to be ready and valid and then calls the OnReady event.  Will call OnReady
 * immediately if the game state is valid already.
 */
UCLASS()
class UAsyncAction_PlayerBuildManagerReady : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	// Waits for the experience to be determined and loaded
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_PlayerBuildManagerReady* WaitForPlayerBuildManagerReady(UPlayerBuildManager* FromManager);

	virtual void Activate() override;

public:
	// Called when the experience has been determined and is ready/loaded
	UPROPERTY(BlueprintAssignable)
	FOnPlayerBuilderManagerReady OnReady;
	// Called when the experience has been determined and is ready/loaded
	UPROPERTY(BlueprintAssignable)
	FOnPlayerBuilderManagerReady OnFailed;

private:
	void OnManagerReady() const;

	TWeakObjectPtr<UPlayerBuildManager> TargetPlayerBuildManager;
};
