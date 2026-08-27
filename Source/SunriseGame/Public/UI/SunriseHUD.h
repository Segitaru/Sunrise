// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "SunriseHUD.generated.h"

class USunriseUI;
class USunriseHUDComponent;
class ASunriseUnit;
class UCanvas;

/** Canvas HUD supplies a complete native fallback and can still host Epic's UMG widget. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;
	void DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw);
	void CommandDragUpdate(ASunriseUnit* SourceUnit, FVector2D CursorPosition, bool bDraw);
	UCanvas* GetDrawingCanvas() const { return Canvas; }

protected:
	void DrawUnitOverlays();
	void DrawMatchPanel();

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
	FVector2D BoxStart = FVector2D::ZeroVector;
	FVector2D BoxSize = FVector2D::ZeroVector;
	FVector2D BoxCurrentPosition = FVector2D::ZeroVector;
	TWeakObjectPtr<ASunriseUnit> CommandDragUnit;
	FVector2D CommandDragCursor = FVector2D::ZeroVector;
	bool bDrawCommandDrag = false;
};
