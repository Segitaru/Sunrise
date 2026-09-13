// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPawn.h"
#include "SunriseUnitTypes.h"
#include "Teams/System/ModularTeamAgentInterface.h"

#include "SunrisePawn.generated.h"

class UModularAbilitySystemComponent;
class ASunriseUnit;
class UModularHeroComponent;
class UModularPawnExtensionComponent;
class UModularCameraComponent;
class UFloatingPawnMovement;
class UEnhancedInputComponent;
class UInputAction;
class UModularTeamActorComponent;

struct FInputActionValue;

/** Local RTS camera/input avatar; the PlayerState owns its modular ASC. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunrisePawn : public AModularPawn, public IAbilitySystemInterface, public IModularTeamAgentInterface
{
	GENERATED_BODY()
public:
	ASunrisePawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void OnAbilitySystemInitialized();
	void OnAbilitySystemUninitialized();

	void InitializeRTSInput(UInputComponent* PlayerInputComponent);
	bool IsRTSInputReady() const { return bRTSInputReady; }

	void MoveCamera(const FInputActionValue& Value);

	void BeginCameraDrag(const FVector2D& Position, bool bTouch = false);
	void EndCameraDrag();
	bool ConsumeCameraDragClick();
	void CancelInteraction();
	bool ShouldUseTouchControls() const;
	bool CanControlCamera() const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraDragScrollCommand(const FVector2D& Position);
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraModifyZoomCommand(float Delta);
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraResetZoomCommand();
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void DoCameraSetZoomPercentageCommand(float Percentage);
	UFUNCTION(BlueprintPure, Category = "Sunrise|Camera")
	float GetDefaultZoomPercentage() const;
	UFUNCTION(BlueprintPure, Category = "Sunrise|Camera")
	float GetZoomPercentage() const;
	float GetCameraZoom() const { return CameraZoom; }
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Camera")
	void FocusCameraOnHero(ASunriseUnit* Hero);


	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseTeam GetTeam() const;

	/** Numeric authority used by multi-team modes. 0=player, 1=legacy enemy, -1=neutral. */
	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	int32 GetTeamId() const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Unit")
	void SetTeam(ESunriseTeam NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Unit")
	void SetTeamId(int32 NewTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	UFUNCTION()
	void HandleTeamChanged(UObject* TeamAgent, int32 PreviousTeamId, int32 NewTeamId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	TObjectPtr<UModularTeamActorComponent> TeamComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UModularHeroComponent> HeroComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UModularPawnExtensionComponent> PawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UModularAbilitySystemComponent> ModularAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UModularCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> CameraMovement;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCameraAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ZoomCameraAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ResetCameraAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractHoldAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "1"))
	float EdgeScrollBorder = 72.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0"))
	float EdgeScrollSpeed = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0"))
	float YawRotationSpeed = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bConstrainCamera = false;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector2D CameraBoundsMin = FVector2D(-10000.0f, -10000.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FVector2D CameraBoundsMax = FVector2D(10000.0f, 10000.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "1"))
	float MinZoomLevel = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "1"))
	float MaxZoomLevel = 3500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0"))
	float ZoomScaling = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0"))
	float DragMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "1"))
	float DefaultZoom = 1500.0f;

private:
	void ReleaseRTSInput();
	void ZoomCamera(const FInputActionValue& Value);
	void ResetCamera(const FInputActionValue& Value);
	void InteractHoldStarted(const FInputActionValue& Value);
	void InteractHoldTriggered(const FInputActionValue& Value);
	void InteractHoldCompleted(const FInputActionValue& Value);
	void ApplyEdgeScroll(float DeltaSeconds);
	void GetCameraGroundBasis(FVector& Forward, FVector& Right) const;
	void ClampCameraToBounds();
	bool TryGetMouseLocationInsideViewport(FVector2D& Position) const;
	void TryFocusControlledHero();

	UPROPERTY(Transient)
	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TArray<uint32> InputBindingHandles;
	FVector2D StartingDragScrollPosition = FVector2D::ZeroVector;
	FVector CameraDragStartLocation = FVector::ZeroVector;

	bool bSelected = false;
	float CameraZoom = 1500.0f;
	float LastCameraDragTime = -1000.0f;
	bool bRTSInputReady = false;
	bool bCameraDragActive = false;
	bool bCameraDragInputSuspended = false;
	bool bDidCameraDrag = false;
	bool bTouchCameraDrag = false;
	bool bHeroFocusConsumed = false;
};
