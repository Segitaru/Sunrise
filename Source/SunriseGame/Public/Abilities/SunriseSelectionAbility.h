// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AbilitySystem/Abilities/ModularGameplayAbility.h"

#include "SunriseSelectionAbility.generated.h"

class ASunrisePawn;
class ASunrisePlayerController;
class ASunriseUnit;
class UEnhancedInputComponent;
class UInputAction;
struct FInputActionValue;
struct FInputActionInstance;

/** Local selection and pointer gestures. Gameplay commands are submitted to the order ability. */
UCLASS(Blueprintable)
class SUNRISEGAME_API USunriseSelectionAbility : public UModularGameplayAbility
{
	GENERATED_BODY()
public:
	USunriseSelectionAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	void BindInput(UEnhancedInputComponent* Input);
	const TArray<ASunriseUnit*>& GetSelectedUnits();
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	bool DoSelectCommand(const FVector& Location, bool bAdditiveSelection);
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoSelectAllUnitsOnScreenCommand();
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoDeselectAllUnitsCommand();
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Selection")
	void DoToggleSelectAllUnitsCommand();
	UFUNCTION(BlueprintPure, Category = "Sunrise|Selection")
	FVector GetMidPointFromSelectedUnits();
	void CancelInteraction();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectClickAdditiveAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectAllDoubleClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectHoldAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectionModifierAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> TouchPrimaryHoldAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> TouchSecondaryAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HeroSquadAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> StopActions;
	UPROPERTY(EditDefaultsOnly, Category = "Selection", meta = (ClampMin = "0", Units = "cm"))
	float SelectionRadius = 180.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel = TraceTypeQuery1;
	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0", Units = "s"))
	float TouchDragScrollHoldTime = 0.15f;
	UFUNCTION(BlueprintImplementableEvent, Category = "Cursor")
	void BP_CursorFeedback(FVector Location, bool bPositive);

private:
	ASunrisePlayerController* GetSunriseController() const;
	bool CanInteract() const;
	bool GetHitUnderCursor(FHitResult& Hit) const;
	FVector2D GetMouseLocationForPlayer() const;
	void PruneSelection();
	void UnbindInput();
	void SelectBox(const FVector2D& Start, const FVector2D& End);
	void SubmitOrder(FGameplayTag Tag, const FVector& Location, AActor* Target = nullptr, ASunriseUnit* SingleUnit = nullptr);
	void OrderFromHit(const FHitResult& Hit, ASunriseUnit* SingleUnit = nullptr);
	void StopSelectedUnits(const FInputActionValue& Value);
	void SelectHoldStarted(const FInputActionValue& Value);
	void SelectHoldTriggered(const FInputActionValue& Value);
	void SelectHoldCompleted(const FInputActionValue& Value);
	void CancelSelectHold(const FInputActionValue& Value);
	void SelectClick(const FInputActionValue& Value);
	void SelectClickAdditive(const FInputActionValue& Value);
	void SelectAllDoubleClick(const FInputActionValue& Value);
	void SelectionModifierStarted(const FInputActionValue& Value);
	void SelectionModifierCompleted(const FInputActionValue& Value);
	void InteractClick(const FInputActionValue& Value);
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);
	void TouchPrimaryHoldTriggered(const FInputActionInstance& Instance);
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);
	void TouchSecondaryTriggered(const FInputActionValue& Value);
	void TouchSecondaryCompleted(const FInputActionValue& Value);
	void ActivateHeroSquadAbility(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TArray<ASunriseUnit*> ControlledUnits;
	UPROPERTY(Transient)
	TWeakObjectPtr<ASunriseUnit> DraggedCommandUnit;
	UPROPERTY(Transient)
	TWeakObjectPtr<ASunrisePawn> AvatarPawn;
	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TArray<uint32> BindingHandles;
	FVector2D StartingBoxSelectionPosition = FVector2D::ZeroVector;
	FVector2D TouchStartPosition = FVector2D::ZeroVector;
	float LastBoxSelectionTime = -1000.0f;
	bool bSelectionModifier = false;
	bool bSuppressNextSelectClick = false;
	bool bTouchDragging = false;
};
