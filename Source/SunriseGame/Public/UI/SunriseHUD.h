// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Player/SunrisePlayerController.h"

#include "SunriseHUD.generated.h"

class USunriseUI;
class USunriseHUDComponent;
class USunriseHeroAbilityHUDComponent;
class ASunriseUnit;
class UCanvas;

/** Canvas HUD supplies a complete native fallback and can still host Epic's UMG widget. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseHUD : public AHUD
{
	GENERATED_BODY()

public:
	ASunriseHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void DrawSelectedUnitsCount(ASunrisePlayerController* PC);
	virtual void DrawHUD() override;
	void DragSelectUpdate(FVector StartWorldLocation, FVector CurrentWorldLocation, bool bDraw);
	void CommandDragUpdate(ASunriseUnit* SourceUnit, FVector2D CursorPosition, bool bDraw);
	UCanvas* GetDrawingCanvas() const { return Canvas; }
	FLinearColor GetTeamColor(int32 TeamId) const;

protected:
	void DrawUnitOverlays();
	void DrawMatchPanel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<USunriseHeroAbilityHUDComponent> HeroAbilityHUDComponent;

	UPROPERTY(Transient)
	TObjectPtr<USunriseUI> UIWidget;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<USunriseUI> UIWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	FLinearColor SelectionBoxColor = FLinearColor(0.1f, 0.8f, 1.0f, 0.22f);

	UPROPERTY(EditAnywhere, Category = "UI")
	FLinearColor FriendlyColor = FLinearColor(0.1f, 0.65f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "UI")
	FLinearColor EnemyColor = FLinearColor(0.95f, 0.12f, 0.08f, 1.0f);

	bool bDrawBox = false;
	FVector BoxStartWorldLocation = FVector::ZeroVector;
	FVector BoxCurrentWorldLocation = FVector::ZeroVector;
	TWeakObjectPtr<ASunriseUnit> CommandDragUnit;
	FVector2D CommandDragCursor = FVector2D::ZeroVector;
	bool bDrawCommandDrag = false;
};
