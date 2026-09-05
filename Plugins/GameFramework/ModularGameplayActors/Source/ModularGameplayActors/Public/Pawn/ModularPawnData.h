// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Cosmetics/System/PawnCosmeticPartTypes.h"
#include "ModularPawnData.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class UModularPawnDataFragment;
class UModularAbilitySet;
class UModularAbilityTagRelationshipMapping;
class UModularInputConfig;
class UModularCameraMode;
class UUserFacingModularPawnDefinition;
class APawn;

// This functionality is needed to issue specific components for a pawn.
USTRUCT(BlueprintType)
struct UE_API FGrantedPawnComponents
{
	GENERATED_BODY();

public:
	void AddComponent(UActorComponent* Component);
	void TakeFromActor(const AActor* OwnerActor);

protected:
	// Handles to the granted abilities.
	UPROPERTY()
	TArray<TObjectPtr<UActorComponent>> SpawnedComponents;
};


USTRUCT(BlueprintType)
struct UE_API FPawnComponentList
{
	GENERATED_BODY();

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<UActorComponent>> Components;

public:
	void GiveComponentsToActor(AActor* OwnerActor, FGrantedPawnComponents* IssuedComponents) const;
};

/**
 * UModularPawnData
 *
 * Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Modular Pawn Data", ShortTooltip = "Data asset used to define a Pawn."),
	CollapseCategories)
class UE_API UModularPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UModularPawnData(const FObjectInitializer& ObjectInitializer);

	bool ShowInGame() const;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	
	// Class to instantiate for this pawn (should usually derive from AModularPawn or AModularCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<APawn> PawnClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "System")
	FPrimaryAssetType ItemType;

	UPROPERTY(EditDefaultsOnly, Category = "System")
	bool bShowInFrontend = true;

	UPROPERTY(EditDefaultsOnly, Category = "System")
	bool bShowInEditor = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	TSoftObjectPtr<UUserFacingModularPawnDefinition> PawnUIDefinition;
	
	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSoftObjectPtr<UModularAbilitySet>> AbilitySets;
	
	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UModularAbilityTagRelationshipMapping> TagRelationshipMapping;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	TArray<FPawnComponentList> ComponentsSets;
	
	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UModularInputConfig> InputConfig;
	
	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TSoftClassPtr<UModularCameraMode> DefaultCameraMode;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Gameplay")
	TArray<TObjectPtr<UModularPawnDataFragment>> Fragments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	TArray<FPawnCosmeticPart> PawnMeshes;
};

#undef UE_API
