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

	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Actions")
	TObjectPtr<UInputAction> SelectClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Actions")
	TObjectPtr<UInputAction> SelectAllDoubleClickAction;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Actions")
	TObjectPtr<UInputAction> SelectHoldAction;

	/** Pointer/mouse movement action used while SelectHoldAction is active. */
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Actions")
	TObjectPtr<UInputAction> SelectMoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Actions")
	TObjectPtr<UInputAction> SelectHeroAction;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Selection", meta = (ClampMin = "0", Units = "cm"))
	float SelectionRadius = 180.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel = TraceTypeQuery1;


private:
	TArray<ASunriseUnit*> GetSelectedUnits() const;
	ASunrisePlayerController* GetSunriseController() const;

	bool CanInteract() const;

	bool GetHitUnderCursor(FHitResult& Hit) const;
	FVector2D GetMouseLocationForPlayer() const;
	bool GetWorldLocationForPlayer(FVector& OutWorldLocation) const;

	void PruneSelection();

	void UnbindInput();

	void SelectBox(const FVector2D& Start, const FVector2D& End);
	void SelectBoxWorld(const FVector& StartWorldLocation, const FVector& EndWorldLocation);
	void OrderFromHit(const FHitResult& Hit, ASunriseUnit* SingleUnit = nullptr);
	void SelectHoldStarted(const FInputActionValue& Value);
	void SelectHoldTriggered(const FInputActionValue& Value);
	void SelectHoldCompleted(const FInputActionValue& Value);
	void CancelSelectHold(const FInputActionValue& Value);
	void SelectClick(const FInputActionValue& Value);
	void SelectAllDoubleClick(const FInputActionValue& Value);
	void SelectHero(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TWeakObjectPtr<ASunriseUnit> DraggedCommandUnit;

	UPROPERTY(Transient)
	TWeakObjectPtr<ASunrisePawn> AvatarPawn;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;

	TArray<uint32> BindingHandles;
	FVector2D StartingBoxSelectionPosition = FVector2D::ZeroVector;
	FVector StartingBoxSelectionWorldLocation = FVector::ZeroVector;

	bool bSelectionGestureActive = false;
	float LastBoxSelectionTime = -1000.0f;
};
