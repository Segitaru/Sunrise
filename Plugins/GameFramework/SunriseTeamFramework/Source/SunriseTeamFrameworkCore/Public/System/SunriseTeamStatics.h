#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SunriseTeamStatics.generated.h"

class USunriseTeamDisplayAsset;
class UTexture;

UCLASS()
class SUNRISETEAMFRAMEWORKCORE_API USunriseTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team", meta = (DefaultToSelf = "Agent", AdvancedDisplay = "bLogIfNotSet"))
	static void FindTeamFromObject(
		const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, USunriseTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet = false);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team", meta = (WorldContext = "WorldContextObject"))
	static USunriseTeamDisplayAsset* GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static float GetTeamScalarWithFallback(const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static FLinearColor GetTeamColorWithFallback(const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	static UTexture* GetTeamTextureWithFallback(const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue);
};
