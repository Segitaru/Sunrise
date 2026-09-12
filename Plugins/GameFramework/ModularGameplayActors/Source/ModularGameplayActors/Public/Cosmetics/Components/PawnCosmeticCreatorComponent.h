// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/PawnComponent.h"
#include "Cosmetics/System/PawnCosmeticAnimationTypes.h"
#include "Cosmetics/System/PawnCosmeticPartTypes.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "PawnCosmeticCreatorComponent.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

class UPawnCosmeticCreatorComponent;

namespace EEndPlayReason
{
	enum Type : int;
}
struct FGameplayTag;
struct FPawnCosmeticPartList;

class AActor;
class UChildActorComponent;
class UObject;
class USceneComponent;
class USkeletalMeshComponent;
struct FFrame;
struct FNetDeltaSerializeInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPawnSpawnedCosmeticPartsChanged, UPawnCosmeticCreatorComponent*, ComponentWithChangedParts);

//////////////////////////////////////////////////////////////////////

// A single applied character part
USTRUCT()
struct FPawnAppliedCosmeticPartEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UE_API FPawnAppliedCosmeticPartEntry() {}

	UE_API FString GetDebugString() const;

private:
	friend FPawnCosmeticPartList;
	friend UPawnCosmeticCreatorComponent;

private:
	// The character part being represented
	UPROPERTY()
	FPawnCosmeticPart Part;

	// Handle index we returned to the user (server only)
	UPROPERTY(NotReplicated)
	int32 PartHandle = INDEX_NONE;

	// The spawned actor instance (client only)
	UPROPERTY(NotReplicated)
	TObjectPtr<UChildActorComponent> SpawnedComponent = nullptr;
};

//////////////////////////////////////////////////////////////////////

// Replicated list of applied character parts
USTRUCT(BlueprintType)
struct FPawnCosmeticPartList : public FFastArraySerializer
{
	GENERATED_BODY()

	FPawnCosmeticPartList()
		: OwnerComponent(nullptr)
	{
	}

public:
	//~FFastArraySerializer contract
	UE_API void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	UE_API void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	UE_API void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	UE_API bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPawnAppliedCosmeticPartEntry, FPawnCosmeticPartList>(
			Entries, DeltaParms, *this);
	}

	FPawnCosmeticPartHandle AddEntry(FPawnCosmeticPart NewPart);
	UE_API void RemoveEntry(FPawnCosmeticPartHandle Handle);
	UE_API void ClearAllEntries(bool bBroadcastChangeDelegate);

	FGameplayTagContainer CollectCombinedTags() const;

	void SetOwnerComponent(UPawnCosmeticCreatorComponent* InOwnerComponent) { OwnerComponent = InOwnerComponent; }

private:
	friend UPawnCosmeticCreatorComponent;

	bool SpawnActorForEntry(FPawnAppliedCosmeticPartEntry& Entry);
	bool DestroyActorForEntry(FPawnAppliedCosmeticPartEntry& Entry);

private:
	// Replicated list of equipment entries
	UPROPERTY()
	TArray<FPawnAppliedCosmeticPartEntry> Entries;

	// The component that contains this list
	UPROPERTY(NotReplicated)
	TObjectPtr<UPawnCosmeticCreatorComponent> OwnerComponent;

	// Upcounter for handles
	int32 PartHandleCounter = 0;
};

template <>
struct TStructOpsTypeTraits<FPawnCosmeticPartList> : public TStructOpsTypeTraitsBase2<FPawnCosmeticPartList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

//////////////////////////////////////////////////////////////////////

// A component that handles spawning cosmetic actors attached to the owner pawn on all clients
UCLASS(meta = (BlueprintSpawnableComponent), MinimalAPI)
class UPawnCosmeticCreatorComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UE_API UPawnCosmeticCreatorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void OnRegister() override;
	//~End of UActorComponent interface

	// Adds a character part to the actor that owns this customization component, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Cosmetics)
	UE_API FPawnCosmeticPartHandle AddCosmeticPart(const FPawnCosmeticPart& NewPart);

	// Removes a previously added character part from the actor that owns this customization component, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Cosmetics)
	UE_API void RemoveCharacterPart(FPawnCosmeticPartHandle Handle);

	// Removes all added character parts, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Cosmetics)
	UE_API void RemoveAllCharacterParts();

	// Gets the list of all spawned character parts from this component
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = Cosmetics)
	UE_API TArray<AActor*> GetCharacterPartActors() const;

	// If the parent actor is derived from ACharacter, returns the Mesh component, otherwise nullptr
	UE_API USkeletalMeshComponent* GetParentMeshComponent() const;

	// Returns the scene component to attach the spawned actors to
	// If the parent actor is derived from ACharacter, we'll use the Mesh component, otherwise the root component
	UE_API USceneComponent* GetSceneComponentToAttachTo() const;

	// Returns the set of combined gameplay tags from attached character parts, optionally filtered to only tags that start with the specified root
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = Cosmetics)
	UE_API FGameplayTagContainer GetCombinedTags(FGameplayTag RequiredPrefix) const;

	UE_API void BroadcastChanged();

public:
	// Delegate that will be called when the list of spawned character parts has changed
	UPROPERTY(BlueprintAssignable, Category = Cosmetics, BlueprintCallable)
	FPawnSpawnedCosmeticPartsChanged OnCharacterPartsChanged;

private:
	// List of character parts
	UPROPERTY(Replicated, Transient)
	FPawnCosmeticPartList CosmeticPartList;

	// Rules for how to pick a body style mesh for animation to play on, based on character part cosmetics tags
	UPROPERTY(EditAnywhere, Category = Cosmetics)
	FPawnAnimBodyStyleSelectionSet BodyMeshes;
};

#undef UE_API