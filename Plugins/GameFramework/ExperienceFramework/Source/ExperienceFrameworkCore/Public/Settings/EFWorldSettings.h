// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/WorldSettings.h"

#include "EFWorldSettings.generated.h"

#define UE_API EXPERIENCEFRAMEWORKCORE_API

class UEFExperienceDefinition;

/**
 * The default world settings object, used primarily to set the default gameplay experience to use when playing on this map
 */
UCLASS(MinimalAPI)
class AEFWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	UE_API AEFWorldSettings(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	UE_API virtual void CheckForErrors() override;
#endif

public:
	// Returns the default experience to use when a server opens this map if it is not overridden by the user-facing experience
	UE_API FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	// The default experience to use when a server opens this map if it is not overridden by the user-facing experience
	UPROPERTY(EditDefaultsOnly, Category = GameMode)
	TSoftClassPtr<UEFExperienceDefinition> DefaultGameplayExperience;

public:
#if WITH_EDITORONLY_DATA
	// Is this level part of a front-end or other standalone experience?
	// When set, the net mode will be forced to Standalone when you hit Play in the editor
	UPROPERTY(EditDefaultsOnly, Category = PIE)
	bool ForceStandaloneNetMode = false;
#endif
};

#undef UE_API
