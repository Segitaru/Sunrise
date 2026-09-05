// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/ModularGameplayEffectContext.h"

#include "AbilitySystem/ModularAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serialization/GameplayEffectContextNetSerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularGameplayEffectContext)

class FArchive;

FModularGameplayEffectContext* FModularGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FModularGameplayEffectContext::StaticStruct()))
	{
		return (FModularGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FModularGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

namespace UE::Net
{
	// Forward to FGameplayEffectContextNetSerializer
	// Note: If FModularGameplayEffectContext::NetSerialize() is modified, a custom NetSerializer must be implemented as the current fallback will no longer be sufficient.
	UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(ModularGameplayEffectContext, FGameplayEffectContextNetSerializer);
}

void FModularGameplayEffectContext::SetAbilitySource(const IModularAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IModularAbilitySourceInterface* FModularGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IModularAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FModularGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

