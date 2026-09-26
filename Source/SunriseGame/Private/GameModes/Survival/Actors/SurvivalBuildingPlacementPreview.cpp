#include "GameModes/Survival/Actors/SurvivalBuildingPlacementPreview.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalBuildComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalBuildingPlacementPreview)

ASurvivalBuildingPlacementPreview::ASurvivalBuildingPlacementPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(SceneRoot);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetGenerateOverlapEvents(false);
	PreviewMesh->SetCanEverAffectNavigation(false);
	PreviewMesh->SetCastShadow(false);
	SetActorEnableCollision(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Indicator(
		TEXT("/Game/Sunrise/Effects/Indicators/M_Indicator.M_Indicator"));
	if (Indicator.Succeeded())
	{
		IndicatorMaterial = Indicator.Object;
	}
}

void ASurvivalBuildingPlacementPreview::Configure(const FSurvivalBuildOption& Option)
{
	const ASurvivalBuilding* Defaults = Option.BuildingClass ? Option.BuildingClass->GetDefaultObject<ASurvivalBuilding>() : nullptr;
	const UStaticMeshComponent* SourceMesh = Defaults ? Defaults->GetBuildingMeshComponent() : nullptr;
	if (!SourceMesh)
	{
		SetActorHiddenInGame(true);
		return;
	}
	PreviewMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	PreviewMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
	const int32 MaterialCount = FMath::Max(1, PreviewMesh->GetNumMaterials());
	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		PreviewMesh->SetMaterial(Index, IndicatorMaterial);
	}
	SetActorHiddenInGame(false);
}
