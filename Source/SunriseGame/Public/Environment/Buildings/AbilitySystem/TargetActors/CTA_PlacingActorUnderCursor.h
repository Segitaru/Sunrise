// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetActor_GroundTrace.h"
#include "AsyncMixin.h"
#include "CoreMinimal.h"

#include "CTA_PlacingActorUnderCursor.generated.h"

class AWR_ActorVisualization;
class APlaceableActor;
class UGameplayAbility;
class UMaterialInterface;

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API ACTA_PlacingActorUnderCursor : public AGameplayAbilityTargetActor_GroundTrace, public FAsyncMixin
{
	GENERATED_BODY()

public:
	ACTA_PlacingActorUnderCursor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual FHitResult PerformTrace(AActor* InSourceActor) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ConfirmTargetingAndContinue() override;

public:
	virtual void StartTargeting(UGameplayAbility* InAbility) override;

	virtual void GetWorldPositionUnderCursor(
		const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd);
	/** Actor we intend to place. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
	TSoftClassPtr<AActor> PlacedActorClass; //Using a special class for replication purposes. (Not implemented yet)

	/** Override Material 0 on our placed actor's meshes with this material for visualization. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Projectile)
	TObjectPtr<UMaterialInterface> PlacedActorMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Targeting)
	bool bUsePositionUnderCursor = true;
};
