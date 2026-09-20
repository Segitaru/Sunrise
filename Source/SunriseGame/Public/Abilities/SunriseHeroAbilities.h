#pragma once

#include "Abilities/ModularGameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

#include "SunriseHeroAbilities.generated.h"

class ASunriseUnit;

UCLASS()
class SUNRISEGAME_API USunriseHeroAbilityCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	USunriseHeroAbilityCooldownEffect();
};

UCLASS(Abstract, Blueprintable)
class SUNRISEGAME_API USunriseHeroAbility : public UModularGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseHeroAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	float GetCooldownRemaining(const ASunriseUnit* Hero) const;

	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	bool GetHero(const FGameplayAbilityActorInfo* ActorInfo, ASunriseUnit*& OutHero) const;

	bool StartHeroAbility(
		FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo);

	void ApplyHeroCooldown(const FGameplayAbilityActorInfo* ActorInfo, float Duration, const FGameplayTag& CooldownTag) const;

	virtual FGameplayTag GetCooldownTag() const { return FGameplayTag(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Ability", meta = (ClampMin = "1.0", Units = "s"))
	float Cooldown = 10.0f;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseHeroHealingAuraAbility : public USunriseHeroAbility
{
	GENERATED_BODY()
public:
	USunriseHeroHealingAuraAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual FGameplayTag GetCooldownTag() const override;

	UFUNCTION()
	void ApplyAuraPulse();

	UFUNCTION()
	void FinishAura();

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.1", Units = "s"))
	float AuraDuration = 3.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "50.0", Units = "cm"))
	float AuraRadius = 600.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.0"))
	float HealPerPulse = 12.0f;
	FTimerHandle AuraPulseTimer;
	FTimerHandle AuraFinishTimer;
	TWeakObjectPtr<ASunriseUnit> AuraHero;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseHeroBombAbility : public USunriseHeroAbility
{
	GENERATED_BODY()
public:
	USunriseHeroBombAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual FGameplayTag GetCooldownTag() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "50.0", Units = "cm"))
	float ExplosionRadius = 450.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 120.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.1", Units = "s"))
	float FuseDuration = 5.0f;
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseHeroBlackHoleAbility : public USunriseHeroAbility
{
	GENERATED_BODY()
public:
	USunriseHeroBlackHoleAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual FGameplayTag GetCooldownTag() const override;

	UFUNCTION()
	void PullAndDamage();

	UFUNCTION()
	void FinishBlackHole();

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "50.0", Units = "cm"))
	float Radius = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.1", Units = "s"))
	float Duration = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.0", ForseUnits = "cm/s"))
	float PullSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Ability", meta = (ClampMin = "0.0"))
	float DamagePerPulse = 80.0f;

	FTimerHandle BlackHolePulseTimer;
	FTimerHandle BlackHoleFinishTimer;
	TWeakObjectPtr<ASunriseUnit> BlackHoleHero;
	FVector BlackHoleLocation = FVector::ZeroVector;
};