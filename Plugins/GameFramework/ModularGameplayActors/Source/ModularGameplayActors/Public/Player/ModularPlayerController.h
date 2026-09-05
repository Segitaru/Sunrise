// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonPlayerController.h"
#include "GameFramework/PlayerController.h"

#include "ModularPlayerController.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class UObject;

/** Minimal class that supports extension by game feature plugins */
UCLASS(MinimalAPI, Blueprintable)
class AModularPlayerController : public ACommonPlayerController
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

	//~ Begin APlayerController interface
	UE_API virtual void ReceivedPlayer() override;
	UE_API virtual void PlayerTick(float DeltaTime) override;
	//~ End APlayerController interface
	
	// Call to see if we should record a replay, subclasses could change this
	UE_API virtual bool ShouldRecordClientReplay();
};

// A player controller used for replay capture and playback
UCLASS(MinimalAPI)
class AModularReplayPlayerController : public AModularPlayerController
{
	GENERATED_BODY()

	UE_API virtual void Tick(float DeltaSeconds) override;
	UE_API virtual void SmoothTargetViewRotation(APawn* TargetPawn, float DeltaSeconds) override;
	
	virtual bool ShouldRecordClientReplay() override;

	// Callback for when the game state's RecorderPlayerState gets replicated during replay playback
	void RecorderPlayerStateUpdated(APlayerState* NewRecorderPlayerState);

	// Callback for when the followed player state changes pawn
	UFUNCTION()
	UE_API  void OnPlayerStatePawnSet(APlayerState* ChangedPlayerState, APawn* NewPlayerPawn, APawn* OldPlayerPawn);

	// The player state we are currently following */
	UPROPERTY(Transient)
	TObjectPtr<APlayerState> FollowedPlayerState;
};
#undef UE_API
