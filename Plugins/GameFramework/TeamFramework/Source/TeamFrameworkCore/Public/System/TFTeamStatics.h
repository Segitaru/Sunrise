#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "TFTeamStatics.generated.h"

class UTFTeamDisplayAsset;
class UTexture;

UCLASS()
class TEAMFRAMEWORKCORE_API UTFTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team", meta = (DefaultToSelf = "Agent", AdvancedDisplay = "bLogIfNotSet"))
	static void FindTeamFromObject(
		const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, UTFTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet = false);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team", meta = (WorldContext = "WorldContextObject"))
	static UTFTeamDisplayAsset* GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static float GetTeamScalarWithFallback(const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static FLinearColor GetTeamColorWithFallback(const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static UTexture* GetTeamTextureWithFallback(const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue);
};
