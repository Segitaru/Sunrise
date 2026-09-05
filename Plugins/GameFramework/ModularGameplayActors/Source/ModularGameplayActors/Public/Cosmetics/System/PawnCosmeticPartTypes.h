// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PawnCosmeticPartTypes.generated.h"

class UChildActorComponent;
class UPawnCosmeticCreatorComponent;
struct FPawnCosmeticPartList;

//////////////////////////////////////////////////////////////////////

// How should collision be configured on the spawned part actor
UENUM()
enum class ECosmeticCustomizationCollisionMode : uint8
{
	// Disable collision on spawned character parts
	NoCollision,

	// Leave the collision settings on character parts alone
	UseCollisionFromCharacterPart
};

//////////////////////////////////////////////////////////////////////

// A handle created by adding a character part entry, can be used to remove it later
USTRUCT(BlueprintType)
struct FPawnCosmeticPartHandle
{
	GENERATED_BODY()

	void Reset()
	{
		PartHandle = INDEX_NONE;
	}

	bool IsValid() const
	{
		return PartHandle != INDEX_NONE;
	}

private:
	UPROPERTY()
	int32 PartHandle = INDEX_NONE;

	friend FPawnCosmeticPartList;
};

//////////////////////////////////////////////////////////////////////
// A character part request

USTRUCT(BlueprintType)
struct FPawnCosmeticPart
{
	GENERATED_BODY()

	// The part to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AActor> PartClass;

	// The socket to attach the part to (if any)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	// How to handle collision for the primitive components in the part
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECosmeticCustomizationCollisionMode CollisionMode = ECosmeticCustomizationCollisionMode::NoCollision;

	// Compares against another part, ignoring the collision mode
	static bool AreEquivalentParts(const FPawnCosmeticPart& A, const FPawnCosmeticPart& B)
	{
		return (A.PartClass == B.PartClass) && (A.SocketName == B.SocketName);
	}
};

#undef UE_API