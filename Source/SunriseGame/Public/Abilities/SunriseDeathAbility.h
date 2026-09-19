#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"

#include "SunriseDeathAbility.generated.h"

class ASunriseUnit;

/** Consumes Vitality's out-of-health event on the Pawn ASC. The Pawn is retained after death. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseDeathAbility : public UModularGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseDeathAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnDeathStarted(ASunriseUnit* Unit);
};

/** Restores the same Pawn; an instanced ability retains the number of successful respawns. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseRespawnAbility : public USunriseDeathAbility
{
	GENERATED_BODY()
public:
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual void OnDeathStarted(ASunriseUnit* Unit) override;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn", meta = (ClampMin = "0.0", Units = "s"))
	float RespawnDelay = 12.0f;

	/** -1 means unlimited; 0 disables respawning. */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn", meta = (ClampMin = "-1"))
	int32 MaxRespawns = -1;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	bool bRespawnAtInitialSpawn = true;

private:
	void WaitForRespawn(float Delay);

	UFUNCTION()
	void TryRespawn();

	FTransform RespawnTransform;
	int32 CompletedRespawns = 0;
};
