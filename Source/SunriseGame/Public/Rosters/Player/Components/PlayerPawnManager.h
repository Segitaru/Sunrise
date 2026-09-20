// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/GameFrameworkInitStateInterface.h>
#include <NativeGameplayTags.h>

#include "Components/PlayerStateComponent.h"
#include "CoreMinimal.h"
#include "ModularPawnData.h"

#include "PlayerPawnManager.generated.h"

SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modular_Gameplay_PawnConfirmed);
SUNRISEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Modular_Gameplay_Selected);

class UModularCameraMode;
class UExperienceDefinition;
class UModularAbilitySystemComponent;
class UModularAbilitySet;
class UGamePawnSelectorComponent;
class AModularPlayerState;
class UModularPawnData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPawnDefinitionUpdated, const UModularPawnData*, NewDefinition);

UCLASS(BlueprintType, MinimalAPI)
class UPlayerPawnManager : public UPlayerStateComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnPawnDefinitionUpdated OnPawnDefinitionUpdated;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_SelectedPawnDefinition)
	TObjectPtr<UModularPawnData> SelectedPawnDefinition;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bCharacterConfirmed)
	bool bCharacterConfirmed = false;

public:
	UPlayerPawnManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure)
	UModularPawnData* GetSelectedPawnDefinition() const;

	UFUNCTION(BlueprintCallable)
	void SetSelectedPawnDefinition(const UModularPawnData* NewPawnDefinition);

	UFUNCTION(BlueprintCallable)
	void TryTakePawn(UModularPawnData* TakingPawn);

	UFUNCTION(BlueprintCallable)
	void ConfirmSelectedPawn();

	UFUNCTION(BlueprintCallable)
	void ResetSelectedPawnConfirmation();

	void SendMessagePlayerPawnConfirmed(bool bIsAutoSelect) const;
	void SendMessagePlayerPawnSelected() const;

	UFUNCTION(BlueprintCallable)
	void SwapRandom();

	// TODO: Try Swap/take Pawn random/choised;
	// TODO: Try Swap Pawn between players;

	void CommitRandomPawn();

	void TryTakeRandomPawnAfterReconnect();

	UFUNCTION(Server, Reliable)
	virtual void TryTakeRandomPawn_OnServer();

	void ClearForceTakeTimer();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	/** The name of this component-implemented feature */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(
		UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	void TryTakePawnOnGameStarted();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnExperienceLoadedForBot(const UExperienceDefinition* CurrentExperience);
	virtual void OnExperienceLoadedForPlayer(const UExperienceDefinition* CurrentExperience);
	void WaitForRosterReady();

	UFUNCTION(Client, Reliable)
	void RestoreSavedPawnSelection_OnClient();

	UFUNCTION(Server, Reliable)
	void RestoreSavedPawnSelection_OnServer(const FGameplayTag& PawnDeclaration);

	UFUNCTION(Server, Reliable)
	void ConfirmSelectedPawn_OnServer();

	UFUNCTION(Server, Reliable)
	void TryTakePawn_OnServer(const FGameplayTag& InPawnDeclaration);

	UFUNCTION(Server, Reliable)
	void ResetSelectedPawnConfirmation_OnServer();

	UFUNCTION()
	virtual void OnRosterReady();

	UFUNCTION()
	virtual void OnTeamChanged(UObject* ObjectChangingTeam, int32 OldTeamID, int32 NewTeamID);

	UFUNCTION()
	void OnRep_bCharacterConfirmed();

	UFUNCTION()
	void OnRep_SelectedPawnDefinition();

	UGamePawnSelectorComponent* GetPawnSelector() const;

	void ForceTakePawn();
	void ForceUpdate();

private:
	UPROPERTY()
	bool bChoseWasRandom = false;

	bool bForceTakePawn = false;

	FTimerHandle ForceTakePawnHandle;

	UPROPERTY()
	TObjectPtr<AModularPlayerState> OwnerPS;

	UPROPERTY()
	TSubclassOf<UObject> PayloadToSearch = nullptr;


	FDelegateHandle OnAbilitySystemInitializedHandle;
};
