// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/WorldReticles/CWR_ActorVisualization.h"

#include <Abilities/GameplayAbilityTargetActor.h>
#include <Components/CapsuleComponent.h>
#include <Components/MeshComponent.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(CWR_ActorVisualization)

ACWR_ActorVisualization::ACWR_ActorVisualization(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent->SetUsingAbsoluteScale(true);
	RootComponent->SetCanEverAffectNavigation(false);
}


void ACWR_ActorVisualization::InitializeReticleVisualizationInformation(
	AGameplayAbilityTargetActor* InTargetingActor, AActor* VisualizationActor, UMaterialInterface* VisualizationMaterial)
{
	if (VisualizationActor)
	{
		//Get components
		TInlineComponentArray<UMeshComponent*> MeshComps;
		USceneComponent* MyRoot = GetRootComponent();
		VisualizationActor->GetComponents(MeshComps);
		check(MyRoot);

		TargetingActor = InTargetingActor;
		AddTickPrerequisiteActor(
			TargetingActor); //We want the reticle to tick after the targeting actor so that designers have the final say on the position

		for (UMeshComponent* MeshComp : MeshComps)
		{
			//Special case: If we don't clear the root component explicitly, the component will be destroyed along with the original visualization actor.
			if (MeshComp == VisualizationActor->GetRootComponent())
			{
				VisualizationActor->SetRootComponent(nullptr);
			}

			//Disable collision on visualization mesh parts so it doesn't interfere with aiming or any other client-side collision/prediction/physics stuff
			MeshComp->SetCollisionEnabled(
				ECollisionEnabled::NoCollision); //All mesh components are primitive components, so no cast is needed

			//Move components from one actor to the other, attaching as needed. Hierarchy should not be important, but we can do fixups if it becomes important later.
			MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			MeshComp->AttachToComponent(MyRoot, FAttachmentTransformRules::KeepRelativeTransform);
			MeshComp->Rename(nullptr, this);
			if (VisualizationMaterial)
			{
				MeshComp->SetMaterial(0, VisualizationMaterial);
			}
		}
	}
}
