// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ControllableEntities/Data/ControllableEntityDefinition.h"
#include "ControllableEntities/IControllableEntity.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "System/TFTeamAgentInterface.h"

#include "SunrisePlayerController.generated.h"

class ASunriseHUD;
class ASunriseUnit;
class UInputAction;
class UInputMappingContext;
class USunrisePauseMenuWidget;
class USunriseOverloadInfoWidget;
class USunriseTouchControls;
class USunriseTopDownCameraComponent;
class USunriseHeroSquadAbility;
class UControllableEntityDefinition;
struct FInputActionInstance;
struct FInputActionValue;

/** Native RTS input layer: camera, edge scroll, selection, formation movement and target orders. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunrisePlayerController : public APlayerController, public IIControllableEntity, public ITFTeamAgentInterface
{
	GENERATED_BODY()

public:
	ASunrisePlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual TScriptInterface<IIControllableEntity> GetControllingAgent() override;
	virtual void SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent) override;
	virtual FOnControllingAgentChanged* GetOnControllingAgentChangedDelegate() override { return &OnControllingAgentChanged; }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return IntegerToTFTeamId(ControlledTeamId); }
	virtual FOnTFTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }

	void DragSelectUnits(const TArray<ASunriseUnit*>& Units);

	const TArray<ASunriseUnit*>& GetSelectedUnits();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetMidPointFromSelectedUnits();

	float GetDefaultZoomPercentage() const;
	void SetCommandsEnabled(bool bEnabled);

	UFUNCTION(Server, Reliable)
	void ServerActivateHeroSquad();

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	int32 GetControlledTeamId() const { return ControlledTeamId; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Hero Ability")
	float GetHeroSquadCooldownRemaining() const;
	TSubclassOf<USunriseHeroSquadAbility> GetHeroSquadAbilityClass() const { return HeroSquadAbilityClass; }

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	bool DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoSelectAllUnitsOnScreenCommand();

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoDeselectAllUnitsCommand();

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoToggleSelectAllUnitsCommand();

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Orders")
	void DoMoveUnitsCommand(const FVector& GoalLocation);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Orders")
	void DoTargetUnitsCommand(ASunriseUnit* Target);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraModifyZoomCommand(float ZoomDelta);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraResetZoomCommand();

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraSetZoomPercentageCommand(float Percentage);

	/** Requests a one-time initial camera focus when the local player's hero appears. */
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void FocusCameraOnHero(ASunriseUnit* Hero);

protected:
	void RefreshPawnFeatures();
	bool ShouldUseTouchControls() const;
	void MoveCamera(const FInputActionValue& Value);
	void ZoomCamera(const FInputActionValue& Value);
	void ResetCamera(const FInputActionValue& Value);
	void SelectHoldStarted(const FInputActionValue& Value);
	void SelectHoldTriggered(const FInputActionValue& Value);
	void SelectHoldCompleted(const FInputActionValue& Value);
	void SelectClick(const FInputActionValue& Value);
	void SelectClickAdditive(const FInputActionValue& Value);
	void SelectAllDoubleClick(const FInputActionValue& Value);
	void SelectionModifierStarted(const FInputActionValue& Value);
	void SelectionModifierCompleted(const FInputActionValue& Value);
	void InteractHoldStarted(const FInputActionValue& Value);
	void InteractHoldTriggered(const FInputActionValue& Value);
	void InteractHoldCompleted(const FInputActionValue& Value);
	void InteractClick(const FInputActionValue& Value);
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);
	void TouchPrimaryHoldTriggered(const FInputActionInstance& Instance);
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);
	void TouchSecondaryTriggered(const FInputActionValue& Value);
	void TouchSecondaryCompleted(const FInputActionValue& Value);
	void ActivateHeroSquadAbility(const FInputActionValue& Value);
	void TogglePauseMenu();
	void StopSelectedUnits();
	void ApplyEdgeScroll(float DeltaSeconds) const;
	void ApplyPendingHeroFocus();
	void GetCameraGroundBasis(FVector& OutForward, FVector& OutRight) const;
	void ClampCameraToBounds() const;
	void PruneSelection();
	FVector2D GetMouseLocationForPlayer() const;
	bool TryGetMouseLocationInsideViewport(FVector2D& OutPosition) const;
	bool GetLocationUnderCursor(FVector& Location);
	bool GetLocationUnderFinger(FVector& Location);
	bool GetHitUnderCursor(FHitResult& Hit) const;
	FVector ProjectTouchPointToWorldSpace();

	UFUNCTION(BlueprintImplementableEvent, Category = "Cursor", meta = (DisplayName = "Cursor Feedback"))
	void BP_CursorFeedback(FVector Location, bool bPositive);

	UPROPERTY(Transient)
	TObjectPtr<APawn> ControlledCameraPawn;

	UPROPERTY(Transient)
	TObjectPtr<USunriseTopDownCameraComponent> TopDownCameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<ASunriseHUD> SunriseHUD;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> MouseMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> RTSMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> TouchMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveCameraAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ZoomCameraAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ResetCameraAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectClickAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectClickAdditiveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectAllDoubleClickAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectHoldAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractClickAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractHoldAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectionModifierAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> TouchPrimaryHoldAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> TouchSecondaryAction;

	/** Configure an authored InputAction that summons the configured mixed squad around the controlled hero. */
	UPROPERTY(EditAnywhere, Category = "Sunrise|Hero Ability")
	TObjectPtr<UInputAction> HeroSquadAction;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Hero Ability")
	TSubclassOf<USunriseHeroSquadAbility> HeroSquadAbilityClass;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSubclassOf<USunriseTouchControls> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USunriseTouchControls> MobileControlsWidget;

	UPROPERTY(Transient)
	TObjectPtr<USunrisePauseMenuWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<USunriseOverloadInfoWidget> OverloadInfoWidget;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bForceTouchControls = false;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "0", Units = "cm"))
	float SelectionRadius = 180.0f;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Sunrise|Team")
	int32 ControlledTeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Control")
	TObjectPtr<class UControllableEntitiesManager> ControllableEntitiesManager;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "50", Units = "cm"))
	float FormationSpacing = 170.0f;

	UPROPERTY(EditAnywhere, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel = TraceTypeQuery1;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float EdgeScrollBorder = 72.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 2200.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bConstrainCamera = false;

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D CameraBoundsMin = FVector2D(-10000.0f, -10000.0f);

	UPROPERTY(EditAnywhere, Category = "Camera")
	FVector2D CameraBoundsMax = FVector2D(10000.0f, 10000.0f);

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float MinZoomLevel = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0"))
	float MaxZoomLevel = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float ZoomScaling = 120.0f;

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float DragMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.0", Units = "s"))
	float TouchDragScrollHoldTime = 0.15f;

	TArray<ASunriseUnit*> ControlledUnits;
	FVector2D StartingDragScrollPosition = FVector2D::ZeroVector;
	FVector2D StartingBoxSelectionPosition = FVector2D::ZeroVector;
	FVector CameraDragStartLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ASunriseUnit> DraggedCommandUnit;

	UPROPERTY(Transient)
	TObjectPtr<ASunriseUnit> PendingHeroFocus;

	float LastTouchDragScrollTime = 0.0f;
	float LastCameraDragTime = -1000.0f;
	float LastBoxSelectionTime = -1000.0f;
	float CameraZoom = 0.0f;
	float DefaultZoom = 1500.0f;
	bool bSelectionModifier = false;
	bool bAllowInteraction = true;
	bool bCameraDragActive = false;
	bool bCameraDragInputSuspended = false;
	bool bDidCameraDrag = false;
	bool bSuppressNextSelectClick = false;
	bool bHeroFocusConsumed = false;

	UPROPERTY(Replicated)
	float NextHeroSquadActivationTime = 0.0f;
	UPROPERTY()
	FOnControllingAgentChanged OnControllingAgentChanged;
	UPROPERTY()
	FOnTFTeamIndexChangedDelegate OnTeamChanged;
};
