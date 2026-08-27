#pragma once

#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "TFTeamSubsystem.generated.h"

class AActor;
class UTFTeamDisplayAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTFTeamDisplayAssetChangedDelegate, const UTFTeamDisplayAsset*, DisplayAsset);

USTRUCT()
struct FTFTeamRuntimeData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTFTeamDisplayAsset> DisplayAsset = nullptr;
	UPROPERTY()
	TMap<FGameplayTag, int32> TagStacks;
	UPROPERTY()
	FOnTFTeamDisplayAssetChangedDelegate OnDisplayAssetChanged;
};

UENUM(BlueprintType)
enum class ETFTeamComparison : uint8
{
	OnSameTeam,
	DifferentTeams,
	InvalidArgument
};

/** World-level queries and presentation data for the component-driven Sunrise team model. */
UCLASS()
class TEAMFRAMEWORKCORE_API UTFTeamSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	bool ChangeTeamForActor(AActor* ActorToChange, int32 NewTeamId);

	int32 FindTeamFromObject(const UObject* TestObject) const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team", meta = (Keywords = "Get"))
	void FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, int32& TeamId) const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team", meta = (ExpandEnumAsExecs = ReturnValue))
	ETFTeamComparison CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA, int32& TeamIdB) const;

	ETFTeamComparison CompareTeams(const UObject* A, const UObject* B) const;
	bool CanCauseDamage(const UObject* Instigator, const UObject* Target, bool bAllowDamageToSelf = true) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	void AddTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	void RemoveTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	int32 GetTeamTagStackCount(int32 TeamId, FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	bool TeamHasTag(int32 TeamId, FGameplayTag Tag) const { return GetTeamTagStackCount(TeamId, Tag) > 0; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	bool DoesTeamExist(int32 TeamId) const { return TeamMap.Contains(TeamId); }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	void SetTeamDisplayAsset(int32 TeamId, UTFTeamDisplayAsset* DisplayAsset);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	UTFTeamDisplayAsset* GetTeamDisplayAsset(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	TArray<int32> GetTeamIds() const;

	void NotifyTeamDisplayAssetModified(UTFTeamDisplayAsset* ModifiedAsset);
	FOnTFTeamDisplayAssetChangedDelegate& GetTeamDisplayAssetChangedDelegate(int32 TeamId);

private:
	int32 FindTeamFromActor(const AActor* Actor, TSet<const UObject*>& VisitedObjects) const;

	UPROPERTY()
	TMap<int32, FTFTeamRuntimeData> TeamMap;
};
