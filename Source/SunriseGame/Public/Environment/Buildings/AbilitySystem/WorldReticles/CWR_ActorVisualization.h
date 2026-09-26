// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityWorldReticle.h"
#include "CoreMinimal.h"

#include "CWR_ActorVisualization.generated.h"

class AGameplayAbilityTargetActor;
class UMaterialInterface;
class USceneComponent;

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API ACWR_ActorVisualization : public AGameplayAbilityWorldReticle
{
	GENERATED_BODY()

public:
	ACWR_ActorVisualization(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void InitializeReticleVisualizationInformation(
		AGameplayAbilityTargetActor* InTargetingActor, AActor* VisualizationActor, UMaterialInterface* VisualizationMaterial);

	UPROPERTY()
	TArray<TObjectPtr<UActorComponent>> VisualizationComponents;
};
