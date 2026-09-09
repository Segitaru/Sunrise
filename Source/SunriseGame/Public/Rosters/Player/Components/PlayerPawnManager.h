// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Components/GameFrameworkInitStateInterface.h>
#include <Components/PawnComponent.h>
#include <NativeGameplayTags.h>

#include "AbilitySystem/ModularAbilitySet.h"
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

class UPlayerPawnManager : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	FModularAbilitySet_GrantedHandles CurrentGrantedAbility;

	UPROPERTY()
	FGrantedPawnComponents CurrentGrantedComponents;

	UPROPERTY()
	TSubclassOf<UModularCameraMode> PawnCameraMode = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedPawnDefinition)
	TObjectPtr<UModularPawnData> SelectedPawnDefinition;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bCharacterConfirmed)
	bool bCharacterConfirmed = false;

	UPROPERTY(BlueprintAssignable)
	FOnPawnDefinitionUpdated OnPawnDefinitionUpdated;

public:
	UPlayerPawnManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure)
	UModularPawnData* GetSelectedPawnDefinition() const;

	UFUNCTION(BlueprintCallable)
	void SetSelectedPawnDefinition(const UModularPawnData* NewPawnDefinition);

	UFUNCTION(BlueprintCallable)
	void TryTakePawn(const UModularPawnData* TakingPawn);

	UFUNCTION(BlueprintCallable)
	void ConfirmSelectedPawn();

	UFUNCTION(BlueprintCallable)
	void ResetSelectedPawnConfirmation();

	void SendMessagePlayerPawnConfirmed(bool bIsAutoSelect) const;
	void SendMessagePlayerPawnSelected() const;

	UFUNCTION(BlueprintCallable)
	void SwapRandom();

	TSubclassOf<UModularCameraMode> GetPawnCameraMode() const;

	// TODO: Try Swap/take Pawn random/choised;
	// TODO: Try Swap Pawn between players;

	void RemoveGrantedAbility(UModularAbilitySystemComponent* FromASC);
	void AddGrantedAbilities(UModularAbilitySystemComponent* IntoASC, TArray<TSoftObjectPtr<UModularAbilitySet>> AbilitiesToGrand);

	void SetDefaultAbilities();

	void CommitRandomPawn();

	void TryTakeRandomPawnAfterReconnect();

	UFUNCTION(Server, Reliable)
	virtual void TryTakeRandomPawn_OnServer();

	void RegisterOrCallOnPawnDataLoaded();
	void ClearForceTakeTimer();

protected:
	virtual void BeginPlay() override;

	/** The name of this component-implemented feature */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(
		UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(
		UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	void TryTakePawnOnGameStarted();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPawnDataLoaded();
	virtual void OnExperienceLoadedForBot(const UExperienceDefinition* CurrentExperience);
	virtual void OnExperienceLoadedForPlayer(const UExperienceDefinition* CurrentExperience);

	UFUNCTION(Server, Reliable)
	void ConfirmSelectedPawn_OnServer();

	UFUNCTION(Server, Reliable)
	void TryTakePawn_OnServer(const UModularPawnData* TakingPawn);

	UFUNCTION(Server, Reliable)
	void ResetSelectedPawnConfirmation_OnServer();

	virtual void TryAddPawnPartComponent();
	virtual void TryAddPawnAbilities();
	virtual void TryAddPawnComponents();
	virtual void TryActivateFragments();
	UFUNCTION()
	virtual void OnRosterReady();

	UFUNCTION()
	virtual void OnTeamChanged(UObject* ObjectChangingTeam, int32 OldTeamID, int32 NewTeamID);

	UFUNCTION()
	void OnRep_bCharacterConfirmed();

	UFUNCTION()
	void OnRep_SelectedPawnDefinition();

	UFUNCTION()
	void OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);

	void OnAbilitySystemInitialized();

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
