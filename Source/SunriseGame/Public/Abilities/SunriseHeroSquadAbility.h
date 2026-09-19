#pragma once

#include "Abilities/ModularGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "SunriseHeroSquadAbility.generated.h"

class UModularPawnData;
class ASunriseUnit;
struct FPawnFormation;

UCLASS()
class SUNRISEGAME_API USunriseHeroSquadCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	USunriseHeroSquadCooldownEffect();
};

UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseHeroSquadAbility : public UModularGameplayAbility
{
	GENERATED_BODY()

public:
	USunriseHeroSquadAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Sunrise")
	static bool ActivateForHero(ASunriseUnit* Hero, TSubclassOf<USunriseHeroSquadAbility> AbilityClass);

	static float GetCooldownRemaining(const ASunriseUnit* Hero);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise")
	TArray<FPawnFormation> SquadFormations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise", meta = (ClampMin = "1.0", Units = "s"))
	float Cooldown = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise", meta = (ClampMin = "50.0", Units = "cm"))
	float FormationSpacing = 170.0f;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASunriseUnit>> SpawnedUnits;
	bool SpawnSquad(ASunriseUnit* Hero);
};
