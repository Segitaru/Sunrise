// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"

#include "ModularPlayerSpawningManagerComponent.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class AController;
class APlayerController;
class APlayerState;
class APlayerStart;
class AModularPlayerStart;
class AActor;

/**
 * @class UModularPlayerSpawningManagerComponent
 */
UCLASS(MinimalAPI)
class UModularPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UE_API UModularPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer);

	/** UActorComponent */
	UE_API virtual void InitializeComponent() override;
	UE_API virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~UActorComponent */

	/** We proxy these calls from AModularGameMode, to this component so that each experience can more easily customize the respawn system they want. */
	UE_API AActor* ChoosePlayerStart(AController* Player);
	UE_API bool ControllerCanRestart(AController* Player);
	UE_API void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	
protected:
	// Utility
	UE_API APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AModularPlayerStart*>& FoundStartPoints) const;
	
	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<AModularPlayerStart*>& PlayerStarts) { return nullptr; }
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) { }

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnFinishRestartPlayer))
	UE_API void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);
	
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AModularPlayerStart>> CachedPlayerStarts;

private:
	UE_API void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);
	UE_API void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	UE_API APlayerStart* FindPlayFromHereStart(AController* Player);
#endif
};

#undef UE_API
