// Copyright Epic Games, Inc. All Rights Reserved.
#include "Abilities/SunriseSelectionAbility.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/SunriseUnitOrderAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Player/SunrisePlayerController.h"
#include "UI/SunriseHUD.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"
#include "Units/SunriseUnitInterfaces.h"

USunriseSelectionAbility::USunriseSelectionAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	ActivationPolicy = EModularAbilityActivationPolicy::OnSpawn;
}

void USunriseSelectionAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AvatarPawn = ActorInfo ? Cast<ASunrisePawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!AvatarPawn.IsValid() || !ActorInfo->IsLocallyControlled())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	BindInput(Cast<UEnhancedInputComponent>(AvatarPawn->InputComponent));
}

void USunriseSelectionAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CancelInteraction();
	DoDeselectAllUnitsCommand();
	UnbindInput();

	AvatarPawn.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

ASunrisePlayerController* USunriseSelectionAbility::GetSunriseController() const
{
	return AvatarPawn.IsValid() ? AvatarPawn->GetController<ASunrisePlayerController>() : nullptr;
}

bool USunriseSelectionAbility::CanInteract() const
{
	const ASunrisePlayerController* PC = GetSunriseController();
	return IsActive() && PC && PC->AreCommandsEnabled() && !PC->IsPaused();
}

void USunriseSelectionAbility::UnbindInput()
{
	if (UEnhancedInputComponent* Input = BoundInput.Get())
	{
		for (uint32 Handle : BindingHandles)
		{
			Input->RemoveBindingByHandle(Handle);
		}
		Input->KeyBindings.RemoveAll(
			[this](const FInputKeyBinding& Binding)
			{
				return Binding.KeyDelegate.IsBoundToObject(this);
			});
	}
	BindingHandles.Reset();
	BoundInput.Reset();
}

void USunriseSelectionAbility::BindInput(UEnhancedInputComponent* Input)
{
	if (!Input || !IsActive())
	{
		return;
	}
	UnbindInput();
	BoundInput = Input;
	if (SelectClickAction)
	{
		BindingHandles.Add(Input->BindAction(SelectClickAction, ETriggerEvent::Triggered, this, &ThisClass::SelectClick).GetHandle());
	}
	if (SelectAllDoubleClickAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Triggered, this, &ThisClass::SelectAllDoubleClick).GetHandle());
	}
	if (SelectHoldAction)
	{
		BindingHandles.Add(Input->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &ThisClass::SelectHoldStarted).GetHandle());
		BindingHandles.Add(Input->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &ThisClass::SelectHoldStarted).GetHandle());
		BindingHandles.Add(
			Input->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &ThisClass::SelectHoldCompleted).GetHandle());
		BindingHandles.Add(Input->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &ThisClass::CancelSelectHold).GetHandle());
	}
	if (SelectMoveAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectMoveAction, ETriggerEvent::Triggered, this, &ThisClass::SelectHoldTriggered).GetHandle());
		BindingHandles.Add(Input->BindAction(SelectMoveAction, ETriggerEvent::Ongoing, this, &ThisClass::SelectHoldTriggered).GetHandle());
	}
}

void USunriseSelectionAbility::PruneSelection()
{
	for (int32 Index = ControlledUnits.Num() - 1; Index >= 0; --Index)
	{
		ASunriseUnit* Unit = ControlledUnits[Index];
		if (!IsValid(Unit) || !Unit->IsAlive() || !ISunriseSelectable::Execute_CanBeSelectedBy(Unit, GetSunriseController()))
		{
			if (IsValid(Unit))
			{
				ISunriseSelectable::Execute_SetSunriseSelected(Unit, false);
			}
			ControlledUnits.RemoveAtSwap(Index);
		}
	}
}

const TArray<ASunriseUnit*>& USunriseSelectionAbility::GetSelectedUnits()
{
	PruneSelection();
	return ControlledUnits;
}

FVector USunriseSelectionAbility::GetMidPointFromSelectedUnits()
{
	PruneSelection();
	FVector Result = FVector::ZeroVector;
	for (const ASunriseUnit* Unit : ControlledUnits)
	{
		Result += Unit->GetActorLocation();
	}
	return ControlledUnits.IsEmpty() ? Result : Result / ControlledUnits.Num();
}

bool USunriseSelectionAbility::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
	if (!CanInteract() || SelectLocation.ContainsNaN())
	{
		return false;
	}
	PruneSelection();
	if (!bAdditiveSelection)
	{
		DoDeselectAllUnitsCommand();
	}
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SunriseSelect), false);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps, SelectLocation, FQuat::Identity, Objects, FCollisionShape::MakeSphere(SelectionRadius), Params);

	ASunriseUnit* Closest = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Overlap.GetActor());
		if (!Unit || !ISunriseSelectable::Execute_CanBeSelectedBy(Unit, GetSunriseController()))
		{
			continue;
		}
		const float Distance = FVector::DistSquared(Unit->GetActorLocation(), SelectLocation);
		if (Distance < BestDistance)
		{
			Closest = Unit;
			BestDistance = Distance;
		}
	}
	if (!Closest)
	{
		return false;
	}
	if (ControlledUnits.Contains(Closest))
	{
		ControlledUnits.Remove(Closest);
		ISunriseSelectable::Execute_SetSunriseSelected(Closest, false);
	}
	else
	{
		ControlledUnits.Add(Closest);
		ISunriseSelectable::Execute_SetSunriseSelected(Closest, true);
	}
	return true;
}

void USunriseSelectionAbility::DoDeselectAllUnitsCommand()
{
	for (ASunriseUnit* Unit : ControlledUnits)
	{
		if (IsValid(Unit))
		{
			ISunriseSelectable::Execute_SetSunriseSelected(Unit, false);
		}
	}
	ControlledUnits.Reset();
}

void USunriseSelectionAbility::DoToggleSelectAllUnitsCommand()
{
	if (ControlledUnits.IsEmpty())
	{
		DoSelectAllUnitsOnScreenCommand();
	}
	else
	{
		DoDeselectAllUnitsCommand();
	}
}

void USunriseSelectionAbility::DoSelectAllUnitsOnScreenCommand()
{
	if (!CanInteract())
	{
		return;
	}
	ASunrisePlayerController* PC = GetSunriseController();
	int32 Width = 0, Height = 0;
	PC->GetViewportSize(Width, Height);
	SelectBox(FVector2D::ZeroVector, FVector2D(Width, Height));
}

void USunriseSelectionAbility::SelectBox(const FVector2D& Start, const FVector2D& End)
{
	if (!CanInteract())
	{
		return;
	}
	DoDeselectAllUnitsCommand();
	const FVector2D Min(FMath::Min(Start.X, End.X), FMath::Min(Start.Y, End.Y));
	const FVector2D Max(FMath::Max(Start.X, End.X), FMath::Max(Start.Y, End.Y));
	ASunrisePlayerController* PC = GetSunriseController();
	for (TActorIterator<ASunriseUnit> It(GetWorld()); It; ++It)
	{
		ASunriseUnit* Unit = *It;
		FVector2D Screen;
		if (ISunriseSelectable::Execute_CanBeSelectedBy(Unit, PC) && PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(), Screen) &&
			Screen.X >= Min.X && Screen.X <= Max.X && Screen.Y >= Min.Y && Screen.Y <= Max.Y)
		{
			ControlledUnits.Add(Unit);
			ISunriseSelectable::Execute_SetSunriseSelected(Unit, true);
		}
	}
}

bool USunriseSelectionAbility::GetHitUnderCursor(FHitResult& Hit) const
{
	const ASunrisePlayerController* PC = GetSunriseController();
	return PC && PC->GetHitResultUnderCursorByChannel(SelectionTraceChannel, true, Hit);
}

FVector2D USunriseSelectionAbility::GetMouseLocationForPlayer() const
{
	float X = 0.0f, Y = 0.0f;
	if (const ASunrisePlayerController* PC = GetSunriseController())
	{
		PC->GetMousePosition(X, Y);
	}
	return FVector2D(X, Y);
}

void USunriseSelectionAbility::OrderFromHit(const FHitResult& Hit, ASunriseUnit* SingleUnit)
{
	AActor* Target = Hit.GetActor();
	if (Cast<ASunriseUnit>(Target) && Target != SingleUnit)
	{
		USunriseUnitOrderAbility::SendOrderEvent(
			AvatarPawn.Get(), SunriseOrders::Target, ControlledUnits, SingleUnit, Hit.ImpactPoint, Target);
	}
	else if (IsValid(Target) && Target->Implements<UOverloadHackable>())
	{
		USunriseUnitOrderAbility::SendOrderEvent(
			AvatarPawn.Get(), SunriseOrders::Hack, ControlledUnits, SingleUnit, Hit.ImpactPoint, Target);
	}
	else
	{
		USunriseUnitOrderAbility::SendOrderEvent(AvatarPawn.Get(), SunriseOrders::Move, ControlledUnits, SingleUnit, Hit.ImpactPoint);
	}
}

void USunriseSelectionAbility::SelectHoldStarted(const FInputActionValue& Value)
{
	if (!CanInteract())
	{
		return;
	}
	if (bSelectionGestureActive)
	{
		return;
	}
	bSelectionGestureActive = true;
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();
	if (ASunriseHUD* HUD = Cast<ASunriseHUD>(GetSunriseController()->GetHUD()))
	{
		HUD->DragSelectUpdate(StartingBoxSelectionPosition, FVector2D::ZeroVector, StartingBoxSelectionPosition, true);
	}
	DraggedCommandUnit.Reset();
	FHitResult Hit;
	if (GetHitUnderCursor(Hit))
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Hit.GetActor());
		if (Unit && ISunriseSelectable::Execute_CanBeSelectedBy(Unit, GetSunriseController()))
		{
			DraggedCommandUnit = Unit;
			if (!ControlledUnits.Contains(Unit))
			{
				DoDeselectAllUnitsCommand();
				ControlledUnits.Add(Unit);
				ISunriseSelectable::Execute_SetSunriseSelected(Unit, true);
			}
		}
	}
}

void USunriseSelectionAbility::SelectHoldTriggered(const FInputActionValue& Value)
{
	if (!CanInteract() || !bSelectionGestureActive)
	{
		return;
	}
	const FVector2D Current = GetMouseLocationForPlayer();
	if (DraggedCommandUnit.IsValid())
	{
		if (ASunriseHUD* HUD = Cast<ASunriseHUD>(GetSunriseController()->GetHUD()))
		{
			HUD->CommandDragUpdate(DraggedCommandUnit.Get(), Current, true);
		}
	}
	else if (FVector2D::Distance(Current, StartingBoxSelectionPosition) > 5.0f)
	{
		SelectBox(StartingBoxSelectionPosition, Current);
		if (ASunriseHUD* HUD = Cast<ASunriseHUD>(GetSunriseController()->GetHUD()))
		{
			HUD->DragSelectUpdate(StartingBoxSelectionPosition, Current - StartingBoxSelectionPosition, Current, true);
		}
	}
}

void USunriseSelectionAbility::SelectHoldCompleted(const FInputActionValue& Value)
{
	if (!bSelectionGestureActive)
	{
		return;
	}
	if (CanInteract() && DraggedCommandUnit.IsValid() &&
		FVector2D::Distance(GetMouseLocationForPlayer(), StartingBoxSelectionPosition) > 8.0f)
	{
		FHitResult Hit;
		if (GetHitUnderCursor(Hit))
		{
			OrderFromHit(Hit, DraggedCommandUnit.Get());
		}
	}
	else if (CanInteract() && !DraggedCommandUnit.IsValid() &&
			 FVector2D::Distance(GetMouseLocationForPlayer(), StartingBoxSelectionPosition) > 5.0f)
	{
		LastBoxSelectionTime = GetWorld()->GetTimeSeconds();
	}
	CancelSelectHold(Value);
}

void USunriseSelectionAbility::CancelSelectHold(const FInputActionValue& Value)
{
	bSelectionGestureActive = false;
	DraggedCommandUnit.Reset();
	if (ASunrisePlayerController* PC = GetSunriseController())
	{
		if (ASunriseHUD* HUD = Cast<ASunriseHUD>(PC->GetHUD()))
		{
			HUD->CommandDragUpdate(nullptr, FVector2D::ZeroVector, false);
			HUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
		}
	}
}

void USunriseSelectionAbility::CancelInteraction()
{
	CancelSelectHold(FInputActionValue());

	DoDeselectAllUnitsCommand();
}

void USunriseSelectionAbility::SelectClick(const FInputActionValue& Value)
{
	if (GetWorld()->GetTimeSeconds() - LastBoxSelectionTime <= 0.15f)
	{
		return;
	}
	FHitResult Hit;
	if (CanInteract() && GetHitUnderCursor(Hit))
	{
		BP_CursorFeedback(Hit.ImpactPoint, DoSelectCommand(Hit.ImpactPoint, false));
	}
}

void USunriseSelectionAbility::SelectAllDoubleClick(const FInputActionValue& Value)
{
	DoSelectAllUnitsOnScreenCommand();
}
