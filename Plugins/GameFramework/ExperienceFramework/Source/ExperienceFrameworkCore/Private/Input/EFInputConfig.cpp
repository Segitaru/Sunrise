// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/EFInputConfig.h"

#include "EFLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFInputConfig)

UEFInputConfig::UEFInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

const UInputAction* UEFInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FEFInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && (Action.InputTags.HasTag(InputTag)))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogExperienceFramework, Error, TEXT("Can't find NativeInputAction for InputTags [%s] on InputConfig [%s]."),
			*InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}

const UInputAction* UEFInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FEFInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTags.HasTag(InputTag)))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogExperienceFramework, Error, TEXT("Can't find AbilityInputAction for InputTags [%s] on InputConfig [%s]."),
			*InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}
