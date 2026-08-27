#include "System/TFTeamStatics.h"

#include "Data/TFTeamDisplayAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "System/TFTeamSubsystem.h"

void UTFTeamStatics::FindTeamFromObject(
	const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, UTFTeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet)
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
		if (UTFTeamSubsystem* Teams = World->GetSubsystem<UTFTeamSubsystem>())
		{
			TeamId = Teams->FindTeamFromObject(Agent);
			bIsPartOfTeam = TeamId != INDEX_NONE;
			DisplayAsset = Teams->GetTeamDisplayAsset(TeamId);
		}
	}
	if (bLogIfNotSet && !bIsPartOfTeam)
	{
		UE_LOG(LogTemp, Warning, TEXT("No TF team is assigned to %s"), *GetPathNameSafe(Agent));
	}
}

UTFTeamDisplayAsset* UTFTeamStatics::GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UTFTeamSubsystem* Teams = World ? World->GetSubsystem<UTFTeamSubsystem>() : nullptr;
	return Teams ? Teams->GetTeamDisplayAsset(TeamId) : nullptr;
}

float UTFTeamStatics::GetTeamScalarWithFallback(const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue)
{
	const float* Value = DisplayAsset ? DisplayAsset->ScalarParameters.Find(ParameterName) : nullptr;
	return Value ? *Value : DefaultValue;
}

FLinearColor UTFTeamStatics::GetTeamColorWithFallback(
	const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue)
{
	const FLinearColor* Value = DisplayAsset ? DisplayAsset->ColorParameters.Find(ParameterName) : nullptr;
	return Value ? *Value : DefaultValue;
}

UTexture* UTFTeamStatics::GetTeamTextureWithFallback(const UTFTeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue)
{
	const TObjectPtr<UTexture>* Value = DisplayAsset ? DisplayAsset->TextureParameters.Find(ParameterName) : nullptr;
	return Value ? Value->Get() : DefaultValue;
}
