// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFeatureStateChangeObserver.h"
#include "GameFeaturesProjectPolicies.h"

#include "ModularGameFeaturePolicy.generated.h"

class FName;
class UGameFeatureData;
struct FPrimaryAssetId;

/**
 * Manager to keep track of the state machines that bring a game feature plugin into memory and active
 * This class discovers plugins either that are built-in and distributed with the game or are reported externally (i.e. by a web service or other endpoint)
 */
UCLASS(MinimalAPI, Config = Game)
class UModularGameFeaturePolicy : public UDefaultGameFeaturesProjectPolicies
{
	GENERATED_BODY()

public:
	MODULARGAMEPLAYACTORS_API static UModularGameFeaturePolicy& Get();

	UModularGameFeaturePolicy(const FObjectInitializer& ObjectInitializer);

	//~UGameFeaturesProjectPolicies interface
	virtual void InitGameFeatureManager() override;
	virtual void ShutdownGameFeatureManager() override;
	virtual TArray<FPrimaryAssetId> GetPreloadAssetListForGameFeature(
		const UGameFeatureData* GameFeatureToLoad, bool bIncludeLoadedAssets = false) const override;
#if UE_VERSION_5_8_x
	virtual bool IsPluginAllowed(const FString& PluginURL, FString* OutReason) const override;
#else
	virtual bool IsPluginAllowed(const FString& PluginURL) const override;
#endif
	virtual const TArray<FName> GetPreloadBundleStateForGameFeature() const override;
	virtual void GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const override;
	//~End of UGameFeaturesProjectPolicies interface

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Observers;
};



// checked
UCLASS()
class UModularGameFeatureHotfixManager : public UObject, public IGameFeatureStateChangeObserver
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureLoading(const UGameFeatureData* GameFeatureData, const FString& PluginURL) override;
};

// checked
UCLASS()
class UModularGameFeatureAddGameplayCuePaths : public UObject, public IGameFeatureStateChangeObserver
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureRegistering(
		const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
	virtual void OnGameFeatureUnregistering(
		const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
};
