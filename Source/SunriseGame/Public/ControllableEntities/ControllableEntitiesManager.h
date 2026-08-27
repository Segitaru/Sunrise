#pragma once

#include "Components/ActorComponent.h"

#include "ControllableEntitiesManager.generated.h"

class UControllableEntityDefinition;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnControlledEntityChanged, AActor*, Entity);

/** Player-side registry and spawn entry point for heroes and summoned RTS squads. */
UCLASS(ClassGroup = (Sunrise), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UControllableEntitiesManager : public UActorComponent
{
	GENERATED_BODY()
public:
	UControllableEntitiesManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	static UControllableEntitiesManager* FindControllableEntitiesManager(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UControllableEntitiesManager>() : nullptr;
	}
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	bool CanControlEntity(const AActor* Entity) const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	void RegisterControlledEntity(AActor* Entity);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	void UnregisterControlledEntity(AActor* Entity);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	void ClearSummonedUnits();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	TArray<APawn*> SpawnControlledUnitsAtLocations(
		TSoftObjectPtr<UControllableEntityDefinition> RequiredEntity, const TArray<FVector>& TargetLocations);
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	TArray<AActor*> GetControlledEntities() const;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntityRegistered;
	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntityUnregistered;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> ControlledEntities;
};
