#pragma once

#include <GenericTeamAgentInterface.h>

#include "AbilitySystemInterface.h"
#include "ControllableEntities/IControllableEntity.h"
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "System/TFTeamAgentInterface.h"
#include "Vitality/VitalityComponent.h"

#include "PlaceableActor.generated.h"

class UAbilitySystemComponent;
class USceneComponent;
class USunriseCombatSet;
class USunriseHealthSet;

/** GAS-backed base for placeable buildings, objectives and destructible props. */
UCLASS(Blueprintable)
class SUNRISEGAME_API APlaceableActor : public AActor,
										public IIControllableEntity,
										public ITFTeamAgentInterface,
										public IAbilitySystemInterface,
										public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	APlaceableActor();
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Placement")
	USceneComponent* GetPlacementComponent() const { return PlacementComponent; }

protected:
	UFUNCTION()
	void HandleVitalityStateChanged(AActor* OwningActor, EVitalityState OldState, EVitalityState NewState);
	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation")
	void BP_VitalityStateChanged(EVitalityState OldState, EVitalityState NewState);
	UFUNCTION()
	void OnRep_TeamId(FGenericTeamId OldTeamId);
	UFUNCTION()
	void OnRep_ControllingAgent(AActor* OldAgent);

#pragma region IIControllableEntity
	virtual TScriptInterface<IIControllableEntity> GetControllingAgent() override;
	virtual void SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent) override;
	virtual FOnControllingAgentChanged* GetOnControllingAgentChangedDelegate() override { return &OnControllingAgentChanged; }
#pragma endregion IIControllableEntity

#pragma region ITFTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual FOnTFTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }
#pragma endregion ITFTeamAgentInterface

#pragma region IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }
#pragma endregion IAbilitySystemInterface

#pragma region IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
#pragma endregion IGameplayTagAssetInterface

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> PlacementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Vitality")
	TObjectPtr<UVitalityComponent> VitalityComponent;

	UPROPERTY()
	TObjectPtr<USunriseHealthSet> HealthSet;

	UPROPERTY()
	TObjectPtr<USunriseCombatSet> CombatSet;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Vitality", meta = (ClampMin = "1"))
	float InitialMaxHealth = 500.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_ControllingAgent)
	TObjectPtr<AActor> ControllingAgentActor;

	UPROPERTY()
	FOnControllingAgentChanged OnControllingAgentChanged;

	UPROPERTY(ReplicatedUsing = OnRep_TeamId)
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;

	UPROPERTY()
	FOnTFTeamIndexChangedDelegate OnTeamChanged;
};
