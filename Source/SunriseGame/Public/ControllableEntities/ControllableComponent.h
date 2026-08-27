#pragma once

#include "Components/ActorComponent.h"

#include "ControllableComponent.generated.h"

class AController;
class UControllableEntityDefinition;

/** Actor-scoped authority deciding whether a player may select and command this entity. */
UCLASS(ClassGroup = (Sunrise), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UControllableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UControllableComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	static UControllableComponent* FindControllableComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UControllableComponent>() : nullptr;
	}
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	bool IsPlayerControllable() const { return bPlayerControllable; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Control")
	void SetPlayerControllable(bool bNewControllable);
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	bool CanBeControlledBy(const AController* Controller) const;
	void SetEntityDefinition(UControllableEntityDefinition* NewDefinition);
	UFUNCTION(BlueprintPure, Category = "Sunrise|Control")
	UControllableEntityDefinition* GetEntityDefinition() const { return EntityDefinition; }

private:
	UPROPERTY(EditAnywhere, Replicated, Category = "Sunrise|Control")
	bool bPlayerControllable = false;
	UPROPERTY(Replicated)
	TObjectPtr<UControllableEntityDefinition> EntityDefinition;
};
