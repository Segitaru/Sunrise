#pragma once

#include "GameplayTagContainer.h"
#include "UI/Components/SunriseHUDComponent.h"

#include "SurvivalHUDComponent.generated.h"

class ASunriseHUD;
class ASunrisePlayerController;
class ASunriseUnit;
class ASurvivalBuilding;
class ASurvivalBuildingPlacementPreview;
class USunriseEndScreenWidget;
struct FSurvivalBuildOption;

/** Asset-independent Survival HUD, building selection and fallback command panel. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalHUDComponent : public USunriseHUDComponent
{
	GENERATED_BODY()

public:
	USurvivalHUDComponent();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DrawHUD(ASunriseHUD* HUD) override;

	UFUNCTION(BlueprintPure, Category = "Survival|Selection")
	ASurvivalBuilding* GetSelectedBuilding() const { return SelectedBuilding.Get(); }

	/** Returns true when Survival UI or placement consumed the current primary click. */
	bool HandlePrimaryClick();

private:
	ASunrisePlayerController* GetController() const;
	ASunriseUnit* FindSelectedWorker() const;
	bool HandleCommandPanelClick(const FVector2D& MousePosition, const FVector2D& ViewportSize);
	void BeginPlacement(const FSurvivalBuildOption& Option, ASunriseUnit* Builder);
	void UpdatePlacementPreview();
	void EndPlacement();
	void ShowEndScreen();
	void SelectBuilding(ASurvivalBuilding* Building);

	TWeakObjectPtr<ASurvivalBuilding> SelectedBuilding;
	TWeakObjectPtr<ASunriseUnit> PendingBuilder;
	FGameplayTag PendingBuildingId;
	FVector PendingPlacementExtent = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ASurvivalBuildingPlacementPreview> PlacementPreview;

	UPROPERTY(Transient)
	TObjectPtr<USunriseEndScreenWidget> EndScreen;

	uint64 LastPrimaryClickFrame = MAX_uint64;
	bool bLastPrimaryClickHandled = false;
};
