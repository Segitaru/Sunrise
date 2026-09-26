#pragma once

#include "GameFramework/Actor.h"

#include "SurvivalBuildingPlacementPreview.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
struct FSurvivalBuildOption;

/** Local, non-replicated visual used while the player chooses a building location. */
UCLASS(NotBlueprintable, Transient)
class SUNRISEGAME_API ASurvivalBuildingPlacementPreview : public AActor
{
	GENERATED_BODY()

public:
	ASurvivalBuildingPlacementPreview();
	void Configure(const FSurvivalBuildOption& Option);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> IndicatorMaterial;
};
