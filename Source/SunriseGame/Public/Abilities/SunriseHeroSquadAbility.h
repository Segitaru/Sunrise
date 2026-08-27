#pragma once
#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"

#include "SunriseHeroSquadAbility.generated.h"
class UControllableEntityDefinition;
class ASunriseUnit;
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseHeroSquadAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseHeroSquadAbility();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Hero Ability")
	static bool ActivateForHero(ASunriseUnit* Hero, TSubclassOf<USunriseHeroSquadAbility> AbilityClass);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Hero Ability")
	TArray<TSoftObjectPtr<UControllableEntityDefinition>> SquadDefinitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Hero Ability", meta = (ClampMin = "1.0", Units = "s"))
	float Cooldown = 30.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Hero Ability", meta = (ClampMin = "50.0", Units = "cm"))
	float FormationSpacing = 170.0f;
	float NextActivationTime = 0.0f;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASunriseUnit>> SpawnedUnits;
	bool SpawnSquad(ASunriseUnit* Hero);
};
