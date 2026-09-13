#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "ModularTeamStatics.generated.h"

class UModularTeamDisplayAsset;
class UTexture;

UCLASS()
class MODULARGAMEPLAYACTORS_API UModularTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Modular|Team", meta = (DefaultToSelf = "Agent", AdvancedDisplay = "bLogIfNotSet"))
	static void FindTeamFromObject(
		const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, UModularTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet = false);

	UFUNCTION(BlueprintPure, Category = "Modular|Team", meta = (WorldContext = "WorldContextObject"))
	static UModularTeamDisplayAsset* GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId);

	UFUNCTION(BlueprintPure, Category = "Modular|Team")
	static float GetTeamScalarWithFallback(UModularTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Modular|Team")
	static FLinearColor GetTeamColorWithFallback(UModularTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Modular|Team")
	static UTexture* GetTeamTextureWithFallback(UModularTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue);
};
