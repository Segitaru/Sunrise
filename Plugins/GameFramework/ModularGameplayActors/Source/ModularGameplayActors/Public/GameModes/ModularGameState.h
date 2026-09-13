// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <AbilitySystemInterface.h>

#include "GameFramework/GameState.h"

#include "ModularGameState.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class UExperienceManagerComponent;
class UModularAbilitySystemComponent;
class UAbilitySystemComponent;

/** Pair this with a ModularGameModeBase */
UCLASS(MinimalAPI, Blueprintable)
class AModularGameStateBase : public AGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UE_API AModularGameStateBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void PostInitializeComponents() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	//~AGameStateBase interface
	UE_API virtual void AddPlayerState(APlayerState* PlayerState) override;
	UE_API virtual void RemovePlayerState(APlayerState* PlayerState) override;
	UE_API virtual void SeamlessTravelTransitionCheckpoint(bool bToTransitionMap) override;
	//~End of AGameStateBase interface

	//~IAbilitySystemInterface
	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface

	// Gets the ability system component used for game wide things
	UFUNCTION(BlueprintCallable, Category = "Modular|GameState", meta = (DeterminesOutputType = "ByClass"))
	UAbilitySystemComponent* K2_GetAbilitySystemComponent(const TSubclassOf<UAbilitySystemComponent> ByClass) const
	{
		return AbilitySystemComponent;
	}

	// Send a message that all clients will (probably) get
	// (use only for client notifications like eliminations, server join messages, etc... that can handle being lost)
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Modular|GameState")
	UE_API void MulticastMessageToClients(const FModularVerbMessage Message);

	// Send a message that all clients will be guaranteed to get
	// (use only for client notifications that cannot handle being lost)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Modular|GameState")
	UE_API void MulticastReliableMessageToClients(const FModularVerbMessage Message);

	// Gets the server's FPS, replicated to clients
	UE_API float GetServerFPS() const;

	// Indicate the local player state is recording a replay
	UE_API void SetRecorderPlayerState(APlayerState* NewPlayerState);

	// Gets the player state that recorded the replay, if valid
	UE_API APlayerState* GetRecorderPlayerState() const;

	// Delegate called when the replay player state changes
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRecorderPlayerStateChanged, APlayerState*);
	FOnRecorderPlayerStateChanged OnRecorderPlayerStateChangedEvent;

private:
	// Handles loading and managing the current gameplay experience
	UPROPERTY()
	TObjectPtr<UExperienceManagerComponent> ExperienceManagerComponent;

	// The ability system component subobject for game-wide things (primarily gameplay cues)
	UPROPERTY(VisibleAnywhere, Category = "Modular|GameState")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

protected:
	UPROPERTY(Replicated)
	float ServerFPS;

	// The player state that recorded a replay, it is used to select the right pawn to follow
	// This is only set in replay streams and is not replicated normally
	UPROPERTY(Transient, ReplicatedUsing = OnRep_RecorderPlayerState)
	TObjectPtr<APlayerState> RecorderPlayerState;

	UFUNCTION()
	UE_API void OnRep_RecorderPlayerState();
};


/** Pair this with a ModularGameState */
UCLASS(MinimalAPI, Blueprintable)
class AModularGameState : public AGameState
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

protected:
	//~ Begin AGameState interface
	UE_API virtual void HandleMatchHasStarted() override;
	//~ Begin AGameState interface
};

#undef UE_API
