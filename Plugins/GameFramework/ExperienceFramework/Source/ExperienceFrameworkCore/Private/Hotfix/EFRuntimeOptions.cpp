// Copyright Epic Games, Inc. All Rights Reserved.

#include "Hotfix/EFRuntimeOptions.h"

#include "UObject/Class.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFRuntimeOptions)

UEFRuntimeOptions::UEFRuntimeOptions()
{
	OptionCommandPrefix = TEXT("ro");
}

UEFRuntimeOptions* UEFRuntimeOptions::GetRuntimeOptions()
{
	return GetMutableDefault<UEFRuntimeOptions>();
}

const UEFRuntimeOptions& UEFRuntimeOptions::Get()
{
	const UEFRuntimeOptions& RuntimeOptions = *GetDefault<UEFRuntimeOptions>();
	return RuntimeOptions;
}
