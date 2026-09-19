#pragma once

#include "Components/ActorComponent.h"

#include "ControllableEntitiesManager.generated.h"


class UModularPawnData;
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

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Control")
	void SelectControlledEntity(AActor* Entity);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Control")
	void UnselectControlledEntity(AActor* Entity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	TArray<APawn*> SpawnControlledUnitsAtLocations(TSoftObjectPtr<UModularPawnData> RequiredEntity, const TArray<FVector>& TargetLocations);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	TArray<AActor*> GetControlledEntities() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	TArray<AActor*> GetSelectedEntities() const;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntityRegistered;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntityUnregistered;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntitySelected;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Control")
	FOnControlledEntityChanged OnEntityUnselected;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SelectedEntities;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> ControlledEntities;
};
