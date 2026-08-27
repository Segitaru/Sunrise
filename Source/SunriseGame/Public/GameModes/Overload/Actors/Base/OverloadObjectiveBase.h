// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"

#include "OverloadObjectiveBase.generated.h"

class UAbilitySystemComponent;
class UOverloadAttributeSet;
class UOverloadIntegritySet;
class UOverloadDefenseSet;
class UOverloadHackSet;
class UOverloadEnergySet;
class UTFTeamActorComponent;
class USceneComponent;
class UStaticMeshComponent;
class UGameplayEffect;

UCLASS(Abstract, Blueprintable)
class SUNRISEGAME_API AOverloadObjectiveBase : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AOverloadObjectiveBase();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Overload|Team")
	int32 GetTeamId() const;
	UFUNCTION(BlueprintPure, Category = "Overload|Team")
	int32 GetOriginalTeamId() const;
	UFUNCTION(BlueprintPure, Category = "Overload|GAS")
	UOverloadIntegritySet* GetIntegrityAttributes() const { return IntegrityAttributes; }
	UFUNCTION(BlueprintPure, Category = "Overload|GAS")
	UOverloadDefenseSet* GetDefenseAttributes() const { return DefenseAttributes; }
	UFUNCTION(BlueprintPure, Category = "Overload|GAS")
	UOverloadHackSet* GetHackAttributes() const { return HackAttributes; }
	UFUNCTION(BlueprintPure, Category = "Overload|GAS")
	UOverloadEnergySet* GetEnergyAttributes() const { return EnergyAttributes; }
	UFUNCTION(BlueprintPure, meta = (DeprecatedFunction, DeprecationMessage = "Use focused Overload attribute accessors"),
		Category = "Overload|GAS")
	UOverloadAttributeSet* GetOverloadAttributes() const { return LegacyAttributes; }
	UFUNCTION(BlueprintCallable, Category = "Overload|Team")
	void InitializeTeam(int32 TeamId);
	UFUNCTION(BlueprintCallable, Category = "Overload|GAS")
	void ApplyDynamicScaling(float AttackMultiplier, float ArmorMultiplier, float ResistanceMultiplier);

protected:
	void InitializeObjectiveAttributes(
		float MaxIntegrity, float AttackPower, float Armor, float HackResistance, float MaxOverloadEnergy = 100.0f);
	float ApplyInstantEffect(TSubclassOf<UGameplayEffect> EffectClass, FName MagnitudeName, float Magnitude);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadAttributeSet> LegacyAttributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadIntegritySet> IntegrityAttributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadDefenseSet> DefenseAttributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadHackSet> HackAttributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UOverloadEnergySet> EnergyAttributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overload|Components")
	TObjectPtr<UTFTeamActorComponent> TeamComponent;
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Overload|Team")
	int32 OriginalTeamId = INDEX_NONE;

private:
	FActiveGameplayEffectHandle ScalingEffectHandle;
};
