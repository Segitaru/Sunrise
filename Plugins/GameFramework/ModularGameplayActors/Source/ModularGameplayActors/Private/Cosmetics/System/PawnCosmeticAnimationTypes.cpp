// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cosmetics/System/PawnCosmeticAnimationTypes.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PawnCosmeticAnimationTypes)

TSubclassOf<UAnimInstance> FPawnAnimLayerSelectionSet::SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FPawnAnimLayerSelectionEntry& Rule : LayerRules)
	{
		if ((Rule.Layer != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Layer.LoadSynchronous();
		}
	}

	return DefaultLayer.LoadSynchronous();
}

USkeletalMesh* FPawnAnimBodyStyleSelectionSet::SelectBestBodyStyle(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FPawnAnimBodyStyleSelectionEntry& Rule : MeshRules)
	{
		if ((Rule.Mesh != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Mesh.LoadSynchronous();
		}
	}

	return DefaultMesh.LoadSynchronous();
}

