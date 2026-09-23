// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/TargetActors/CTA_PlacingActorUnderCursor.h"

#include <Abilities/GameplayAbility.h>

#include "AbilitySystem/WorldReticles/CWR_ActorVisualization.h"
#include "BuildingTypes.h"

ACTA_PlacingActorUnderCursor::ACTA_PlacingActorUnderCursor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FHitResult ACTA_PlacingActorUnderCursor::PerformTrace(AActor* InSourceActor)
{
	if (!bUsePositionUnderCursor)
	{
		return Super::PerformTrace(InSourceActor);
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ACTA_PlacingActorUnderCursor));
	Params.bReturnPhysicalMaterial = true;

	Params.AddIgnoredActor(InSourceActor);

	if (const auto ReticlePtr = ReticleActor.Get())
	{
		Params.AddIgnoredActor(ReticleActor.Get());

		TArray<AActor*> IgnoreActors;
		ReticlePtr->GetAttachedActors(IgnoreActors);
		Params.AddIgnoredActors(IgnoreActors);
	}

	FVector TraceStart = StartLocation.GetTargetingTransform().GetLocation();
	FVector TraceEnd;
	GetWorldPositionUnderCursor(InSourceActor, Params, TraceStart, TraceEnd); //Effective on server and launching client only

	// ------------------------------------------------------

	FHitResult ReturnHitResult;
	//Use a line trace initially to see where the player is actually pointing
	LineTraceWithFilter(ReturnHitResult, InSourceActor->GetWorld(), Filter, TraceStart, TraceEnd, TraceProfile.Name, Params);
	//Default to end of trace line if we don't hit anything.
	if (!ReturnHitResult.bBlockingHit)
	{
		ReturnHitResult.Location = TraceEnd;
	}

	//Second trace, straight down. Consider using InSourceActor->GetWorld()->NavigationSystem->ProjectPointToNavigation() instead of just going straight down in the case of movement abilities (flag/bool).
	TraceStart = ReturnHitResult.Location - (TraceEnd - TraceStart).GetSafeNormal(); //Pull back very slightly to avoid scraping down walls
	TraceEnd = TraceStart;
	TraceStart.Z += CollisionHeightOffset;
	TraceEnd.Z -= 99999.0f;
	LineTraceWithFilter(ReturnHitResult, InSourceActor->GetWorld(), Filter, TraceStart, TraceEnd, TraceProfile.Name, Params);
	//if (!ReturnHitResult.bBlockingHit) then our endpoint may be off the map. Hopefully this is only possible in debug maps.

	bLastTraceWasGood = true; //So far, we're good. If we need a ground spot and can't find one, we'll come back.

	//Use collision shape to find a valid ground spot, if appropriate
	if (CollisionShape.ShapeType != ECollisionShape::Line)
	{
		ReturnHitResult.Location.Z += CollisionHeightOffset; //Rise up out of the ground
		TraceStart = InSourceActor->GetActorLocation();
		TraceEnd = ReturnHitResult.Location;
		TraceStart.Z += CollisionHeightOffset;
		bLastTraceWasGood = AdjustCollisionResultForShape(TraceStart, TraceEnd, Params, ReturnHitResult);

		if (bLastTraceWasGood)
		{
			ReturnHitResult.Location.Z -= CollisionHeightOffset; //Undo the artificial height adjustment

			if (AGameplayAbilityWorldReticle* LocalReticleActor = ReticleActor.Get())
			{
				LocalReticleActor->SetIsTargetValid(bLastTraceWasGood);
				LocalReticleActor->SetActorLocation(ReturnHitResult.Location);
			}
		}
	}

	// Reset the trace start so the target data uses the correct origin
	ReturnHitResult.TraceStart = StartLocation.GetTargetingTransform().GetLocation();

	return ReturnHitResult;
}
void ACTA_PlacingActorUnderCursor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}
void ACTA_PlacingActorUnderCursor::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData());

	if (SourceActor)
	{
		bDebug = false;

		FGameplayAbilityTargetData_ActorPlacement* ReturnData = new FGameplayAbilityTargetData_ActorPlacement();
		ReturnData->HitResult = PerformTrace(SourceActor);
		ReturnData->PlacementActorClass = PlacedActorClass;

		TargetDataReadyDelegate.Broadcast(ReturnData);
	}
}

void ACTA_PlacingActorUnderCursor::StartTargeting(UGameplayAbility* InAbility)
{
	Super::StartTargeting(InAbility);

	AsyncLoad<AActor>(PlacedActorClass,
		TFunction<void(TSubclassOf<AActor>)>(
			[this](const TSubclassOf<AActor>& LoadedClass)
			{
				UWorld* World = GetWorld();
				if (!World || World->bIsTearingDown)
				{
					return;
				}

				ACWR_ActorVisualization* ActorVisualizationReticle = nullptr;

				if (AActor* VisualizationActor = World->SpawnActor(LoadedClass))
				{
					ActorVisualizationReticle = World->SpawnActor<ACWR_ActorVisualization>();
					ActorVisualizationReticle->InitializeReticleVisualizationInformation(this, VisualizationActor, PlacedActorMaterial);
					World->DestroyActor(VisualizationActor);
				}

				if (AGameplayAbilityWorldReticle* CachedReticleActor = ReticleActor.Get())
				{
					ActorVisualizationReticle->AttachToActor(CachedReticleActor, FAttachmentTransformRules::KeepRelativeTransform);
				}
				else
				{
					ReticleActor = ActorVisualizationReticle;
				}
			}));
}

void ACTA_PlacingActorUnderCursor::GetWorldPositionUnderCursor(
	const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd)
{
	if (!OwningAbility) // Server and launching client only
	{
		return;
	}

	APlayerController* PC = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
	check(PC);

	FVector ViewStart;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewStart, ViewRot);

	float MouseX;
	float MouseY;
	FVector Location;
	FVector ViewDir;
	PC->GetMousePosition(MouseX, MouseY);
	PC->DeprojectScreenPositionToWorld(MouseX, MouseY, Location, ViewDir);

	FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	FHitResult HitResult;
	LineTraceWithFilter(HitResult, InSourceActor->GetWorld(), Filter, ViewStart, ViewEnd, TraceProfile.Name, Params);

	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (MaxRange * MaxRange));

	const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero())
	{
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult)
	{
		FVector OriginalAimDir = (ViewEnd - TraceStart).GetSafeNormal();

		if (!OriginalAimDir.IsZero())
		{
			// Convert to angles and use original pitch
			const FRotator OriginalAimRot = OriginalAimDir.Rotation();

			FRotator AdjustedAimRot = AdjustedAimDir.Rotation();
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}

	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}