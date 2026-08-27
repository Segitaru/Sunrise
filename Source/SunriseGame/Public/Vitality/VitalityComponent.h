#pragma once

#include "AttributeSet.h"
#include "Components/ActorComponent.h"

#include "VitalityComponent.generated.h"

class UAbilitySystemComponent;
class USunriseHealthSet;
struct FGameplayEffectSpec;

UENUM(BlueprintType)
enum class EVitalityState : uint8
{
	Healthy = 0,
	Dying,
	Knockdowned,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnVitalityStateChanged, AActor*, OwningActor, EVitalityState, OldState, EVitalityState, CurrentState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnVitalityAttributeChanged, UVitalityComponent*, VitalityComponent,
	const FGameplayAttribute&, Attribute, float, OldValue, float, NewValue, AActor*, Instigator);

/** Shared health/death facade for every actor backed by USunriseHealthSet. */
UCLASS(ClassGroup = (Sunrise), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UVitalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVitalityComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnUnregister() override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	static UVitalityComponent* FindVitalityComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UVitalityComponent>() : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Vitality")
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InASC);
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Vitality")
	void UninitializeFromAbilitySystem();
	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	float GetHealth() const;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	float GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	float GetHealthNormalized() const;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	EVitalityState GetVitalityState() const { return VitalityState; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Vitality")
	bool IsHealthy() const { return VitalityState == EVitalityState::Healthy; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Vitality")
	void StartDeath();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Vitality")
	void FinishDeath();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Vitality")
	void RestoreVitality();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Vitality")
	void DamageSelfDestruct();

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Vitality")
	FOnVitalityAttributeChanged OnAttributeChanged;
	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Vitality")
	FOnVitalityStateChanged OnVitalityStateChanged;

private:
	void SetVitalityState(EVitalityState NewState);
	void ClearGameplayTags();
	void HandleHealthChanged(
		AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
	void HandleMaxHealthChanged(
		AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
	void HandleOutOfHealth(
		AActor* Instigator, AActor* Causer, const FGameplayEffectSpec* Spec, float Magnitude, float OldValue, float NewValue);
	UFUNCTION()
	void OnRep_VitalityState(EVitalityState OldState);

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY()
	TObjectPtr<const USunriseHealthSet> HealthSet;
	UPROPERTY(ReplicatedUsing = OnRep_VitalityState)
	EVitalityState VitalityState = EVitalityState::Healthy;
};
