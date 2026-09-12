// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "ModularAbilitySet.h"
#include "ModularPawnData.h"

#include "ModularPawnExtensionComponent.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

namespace EEndPlayReason
{
	enum Type : int;
}

class UGameFrameworkComponentManager;
class UModularAbilitySystemComponent;
class UModularPawnData;
class UObject;
struct FActorInitStateChangedParams;
struct FFrame;
struct FGameplayTag;

/**
 * Component that adds functionality to all Pawn classes so it can be used for characters/vehicles/etc.
 * This coordinates the initialization of other components.
 */
UCLASS(MinimalAPI)
class UModularPawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	UE_API UModularPawnExtensionComponent(const FObjectInitializer& ObjectInitializer);

	/** The name of this overall feature, this one depends on the other named component features */
	static UE_API const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	UE_API virtual bool CanChangeInitState(
		UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	UE_API virtual void HandleChangeInitState(
		UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	UE_API virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	UE_API virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

	/** Returns the pawn extension component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "Modular|Pawn")
	static UModularPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<UModularPawnExtensionComponent>() : nullptr);
	}

	/** Gets the pawn data, which is used to specify pawn properties in data */
	template <class T>
	const T* GetPawnData() const
	{
		return Cast<T>(PawnData);
	}

	/** Sets the current pawn data */
	UE_API void SetPawnData(const UModularPawnData* InPawnData);

	/** Gets the current ability system component, which may be owned by a different actor */
	UFUNCTION(BlueprintPure, Category = "Modular|Pawn")
	UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Should be called by the owning pawn to become the avatar of the ability system. */
	UE_API void InitializeAbilitySystem(UModularAbilitySystemComponent* InASC, AActor* InOwnerActor);

	UE_API void UpdateAbilitySystemOwner(AActor* InOwnerActor);

	/** Should be called by the owning pawn to remove itself as the avatar of the ability system. */
	UE_API void UninitializeAbilitySystem();

	/** Should be called by the owning pawn when the pawn's controller changes. */
	UE_API void HandleControllerChanged();

	/** Should be called by the owning pawn when the player state has been replicated. */
	UE_API void HandlePlayerStateReplicated();

	/** Should be called by the owning pawn when the input component is setup. */
	UE_API void SetupPlayerInputComponent();

	/** Register with the OnAbilitySystemInitialized delegate and broadcast if our pawn has been registered with the ability system component */
	UE_API void OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

	/** Register with the OnAbilitySystemUninitialized delegate fired when our pawn is removed as the ability system's avatar actor */
	UE_API void OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

protected:
	UE_API virtual void OnRegister() override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	UE_API void OnRep_PawnData();

	void TryAddPawnPartComponent();
	void TryAddPawnAbilities();
	void TryAddPawnComponents();
	void TryActivateFragments();

	void RemoveGrantedAbility();
	void AddGrantedAbilities();

	/** Delegate fired when our pawn becomes the ability system's avatar actor */
	FSimpleMulticastDelegate OnAbilitySystemInitialized;

	/** Delegate fired when our pawn is removed as the ability system's avatar actor */
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;

	/** Pawn data used to create the pawn. Specified from a spawn function or on a placed instance. */
	UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "Modular|Pawn")
	TObjectPtr<const UModularPawnData> PawnData;

	/** Pointer to the ability system component that is cached for convenience. */
	UPROPERTY(Transient)
	TObjectPtr<UModularAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	FModularAbilitySet_GrantedHandles CurrentGrantedAbility;

	UPROPERTY()
	FGrantedPawnComponents CurrentGrantedComponents;
};

#undef UE_API
