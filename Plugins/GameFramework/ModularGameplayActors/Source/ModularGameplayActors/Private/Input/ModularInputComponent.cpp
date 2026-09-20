// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModularInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularInputComponent)

class UModularInputConfig;

UModularInputComponent::UModularInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UModularInputComponent::AddInputMappings(const UModularInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to add something from your input config if required
}

void UModularInputComponent::RemoveInputMappings(const UModularInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to remove input mappings that you may have added above
}

void UModularInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
