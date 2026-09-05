// Copyright Epic Games, Inc. All Rights Reserved.

#include "Hotfix/ModularRuntimeOptions.h"
#include "UObject/Class.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularRuntimeOptions)

UModularRuntimeOptions::UModularRuntimeOptions()
{
	OptionCommandPrefix = TEXT("ro");
}

UModularRuntimeOptions* UModularRuntimeOptions::GetRuntimeOptions()
{
	return GetMutableDefault<UModularRuntimeOptions>();
}

const UModularRuntimeOptions& UModularRuntimeOptions::Get()
{
	const UModularRuntimeOptions& RuntimeOptions = *GetDefault<UModularRuntimeOptions>();
	return RuntimeOptions;
}
