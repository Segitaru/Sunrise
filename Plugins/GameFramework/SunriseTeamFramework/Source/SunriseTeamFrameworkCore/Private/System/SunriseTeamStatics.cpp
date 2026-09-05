#include "System/SunriseTeamStatics.h"

#include "Data/SunriseTeamDisplayAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "System/SunriseTeamSubsystem.h"

void USunriseTeamStatics::FindTeamFromObject(
	const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, USunriseTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet)
{
	bIsPartOfTeam = false;
	TeamId = INDEX_NONE;
	DisplayAsset = nullptr;
	if (!Agent)
	{
		return;
	}
	if (const UWorld* World = GEngine->GetWorldFromContextObject(Agent, EGetWorldErrorMode::ReturnNull))
	{
		if (USunriseTeamSubsystem* Teams = World->GetSubsystem<USunriseTeamSubsystem>())
		{
			TeamId = Teams->FindTeamFromObject(Agent);
			bIsPartOfTeam = TeamId != INDEX_NONE;
			DisplayAsset = Teams->GetTeamDisplayAsset(TeamId);
		}
	}
	if (bLogIfNotSet && !bIsPartOfTeam)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Sunrise team is assigned to %s"), *GetPathNameSafe(Agent));
	}
}

USunriseTeamDisplayAsset* USunriseTeamStatics::GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	USunriseTeamSubsystem* Teams = World ? World->GetSubsystem<USunriseTeamSubsystem>() : nullptr;
	return Teams ? Teams->GetTeamDisplayAsset(TeamId) : nullptr;
}

float USunriseTeamStatics::GetTeamScalarWithFallback(const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue)
{
	const float* Value = DisplayAsset ? DisplayAsset->ScalarParameters.Find(ParameterName) : nullptr;
	return Value ? *Value : DefaultValue;
}

FLinearColor USunriseTeamStatics::GetTeamColorWithFallback(
	const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue)
{
	const FLinearColor* Value = DisplayAsset ? DisplayAsset->ColorParameters.Find(ParameterName) : nullptr;
	return Value ? *Value : DefaultValue;
}

UTexture* USunriseTeamStatics::GetTeamTextureWithFallback(const USunriseTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue)
{
	const TObjectPtr<UTexture>* Value = DisplayAsset ? DisplayAsset->TextureParameters.Find(ParameterName) : nullptr;
	return Value ? Value->Get() : DefaultValue;
}
