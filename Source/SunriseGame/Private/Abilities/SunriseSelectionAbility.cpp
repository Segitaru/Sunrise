// Copyright Epic Games, Inc. All Rights Reserved.
#include "Abilities/SunriseSelectionAbility.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/SunriseUnitOrderAbility.h"
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

	if (AvatarPawn->IsRTSInputReady())
	{
		BindInput(Cast<UEnhancedInputComponent>(AvatarPawn->InputComponent));
	}
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
		BindingHandles.Add(Input->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &ThisClass::SelectClick).GetHandle());
	}
	if (SelectClickAdditiveAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &ThisClass::SelectClickAdditive).GetHandle());
	}
	if (SelectAllDoubleClickAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &ThisClass::SelectAllDoubleClick).GetHandle());
	}
	if (SelectHoldAction)
	{
		BindingHandles.Add(Input->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &ThisClass::SelectHoldStarted).GetHandle());
	}
	if (SelectHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &ThisClass::SelectHoldTriggered).GetHandle());
	}
	if (SelectHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &ThisClass::SelectHoldCompleted).GetHandle());
	}
	if (SelectHoldAction)
	{
		BindingHandles.Add(Input->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &ThisClass::CancelSelectHold).GetHandle());
	}
	if (InteractClickAction)
	{
		BindingHandles.Add(Input->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &ThisClass::InteractClick).GetHandle());
	}
	if (SelectionModifierAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectionModifierAction, ETriggerEvent::Started, this, &ThisClass::SelectionModifierStarted).GetHandle());
	}
	if (SelectionModifierAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectionModifierAction, ETriggerEvent::Completed, this, &ThisClass::SelectionModifierCompleted).GetHandle());
	}
	if (SelectionModifierAction)
	{
		BindingHandles.Add(
			Input->BindAction(SelectionModifierAction, ETriggerEvent::Canceled, this, &ThisClass::SelectionModifierCompleted).GetHandle());
	}
	if (TouchPrimaryHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &ThisClass::TouchPrimaryHoldStarted).GetHandle());
	}
	if (TouchPrimaryHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &ThisClass::TouchPrimaryHoldTriggered).GetHandle());
	}
	if (TouchPrimaryHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &ThisClass::TouchPrimaryHoldCompleted).GetHandle());
	}
	if (TouchPrimaryHoldAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Canceled, this, &ThisClass::CancelSelectHold).GetHandle());
	}
	if (TouchSecondaryAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &ThisClass::TouchSecondaryTriggered).GetHandle());
	}
	if (TouchSecondaryAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &ThisClass::TouchSecondaryCompleted).GetHandle());
	}
	if (TouchSecondaryAction)
	{
		BindingHandles.Add(
			Input->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &ThisClass::CancelSelectHold).GetHandle());
	}
	if (HeroSquadAction)
	{
		BindingHandles.Add(
			Input->BindAction(HeroSquadAction, ETriggerEvent::Started, this, &ThisClass::ActivateHeroSquadAbility).GetHandle());
	}
	if (StopActions)
	{
		BindingHandles.Add(Input->BindAction(StopActions, ETriggerEvent::Started, this, &ThisClass::StopSelectedUnits).GetHandle());
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

void USunriseSelectionAbility::SubmitOrder(FGameplayTag Tag, const FVector& Location, AActor* Target, ASunriseUnit* SingleUnit)
{
	if (!CanInteract())
	{
		return;
	}
	PruneSelection();
	FGameplayEventData Event;
	Event.EventTag = Tag;
	Event.Target = Target;
	auto* Actors = new FGameplayAbilityTargetData_ActorArray();
	if (SingleUnit)
	{
		Actors->TargetActorArray.Add(SingleUnit);
	}
	else
	{
		for (ASunriseUnit* Unit : ControlledUnits)
		{
			Actors->TargetActorArray.Add(Unit);
		}
	}
	Event.TargetData.Add(Actors);
	FHitResult Hit;
	Hit.ImpactPoint = Location;
	Hit.Location = Location;
	Event.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));
	GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(Tag, &Event);
}

void USunriseSelectionAbility::OrderFromHit(const FHitResult& Hit, ASunriseUnit* SingleUnit)
{
	AActor* Target = Hit.GetActor();
	if (Cast<ASunriseUnit>(Target) && Target != SingleUnit)
	{
		SubmitOrder(SunriseOrders::Target, Hit.ImpactPoint, Target, SingleUnit);
	}
	else if (IsValid(Target) && Target->Implements<UOverloadHackable>())
	{
		SubmitOrder(SunriseOrders::Hack, Hit.ImpactPoint, Target, SingleUnit);
	}
	else
	{
		SubmitOrder(SunriseOrders::Move, Hit.ImpactPoint, nullptr, SingleUnit);
	}
}

void USunriseSelectionAbility::StopSelectedUnits(const FInputActionValue& Value)
{
	SubmitOrder(SunriseOrders::Stop, FVector::ZeroVector);
}

void USunriseSelectionAbility::SelectHoldStarted(const FInputActionValue& Value)
{
	if (!CanInteract())
	{
		return;
	}
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();
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
				if (!bSelectionModifier)
				{
					DoDeselectAllUnitsCommand();
				}
				ControlledUnits.AddUnique(Unit);
				ISunriseSelectable::Execute_SetSunriseSelected(Unit, true);
			}
			bSuppressNextSelectClick = true;
		}
	}
}

void USunriseSelectionAbility::SelectHoldTriggered(const FInputActionValue& Value)
{
	if (!CanInteract())
	{
		return;
	}
	const FVector2D Current = GetMouseLocationForPlayer();
	ASunriseHUD* HUD = Cast<ASunriseHUD>(GetSunriseController()->GetHUD());
	if (DraggedCommandUnit.IsValid())
	{
		if (HUD)
		{
			HUD->CommandDragUpdate(DraggedCommandUnit.Get(), Current, true);
		}
	}
	else if (FVector2D::Distance(Current, StartingBoxSelectionPosition) > 5.0f)
	{
		SelectBox(StartingBoxSelectionPosition, Current);
		if (HUD)
		{
			HUD->DragSelectUpdate(StartingBoxSelectionPosition, Current - StartingBoxSelectionPosition, Current, true);
		}
	}
}

void USunriseSelectionAbility::SelectHoldCompleted(const FInputActionValue& Value)
{
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
	DraggedCommandUnit.Reset();
	bTouchDragging = false;
	if (AvatarPawn.IsValid())
	{
		AvatarPawn->EndCameraDrag();
	}
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
	bSelectionModifier = false;
	bSuppressNextSelectClick = false;
	DoDeselectAllUnitsCommand();
}

void USunriseSelectionAbility::SelectClick(const FInputActionValue& Value)
{
	if (GetWorld()->GetTimeSeconds() - LastBoxSelectionTime <= 0.15f)
	{
		return;
	}
	if (bSuppressNextSelectClick)
	{
		bSuppressNextSelectClick = false;
		return;
	}
	FHitResult Hit;
	if (CanInteract() && GetHitUnderCursor(Hit))
	{
		BP_CursorFeedback(Hit.ImpactPoint, DoSelectCommand(Hit.ImpactPoint, bSelectionModifier));
	}
}

void USunriseSelectionAbility::SelectClickAdditive(const FInputActionValue& Value)
{
	FHitResult Hit;
	if (CanInteract() && GetHitUnderCursor(Hit))
	{
		DoSelectCommand(Hit.ImpactPoint, true);
	}
}

void USunriseSelectionAbility::SelectAllDoubleClick(const FInputActionValue& Value)
{
	DoSelectAllUnitsOnScreenCommand();
}

void USunriseSelectionAbility::SelectionModifierStarted(const FInputActionValue& Value)
{
	bSelectionModifier = true;
}
void USunriseSelectionAbility::SelectionModifierCompleted(const FInputActionValue& Value)
{
	bSelectionModifier = false;
}

void USunriseSelectionAbility::InteractClick(const FInputActionValue& Value)
{
	if (!CanInteract() || AvatarPawn->ConsumeCameraDragClick())
	{
		return;
	}
	FHitResult Hit;
	if (GetHitUnderCursor(Hit))
	{
		OrderFromHit(Hit);
	}
}

void USunriseSelectionAbility::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	TouchStartPosition = Value.Get<FVector2D>();
	StartingBoxSelectionPosition = TouchStartPosition;
	bTouchDragging = false;
}

void USunriseSelectionAbility::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	if (!CanInteract() || Instance.GetElapsedTime() <= TouchDragScrollHoldTime)
	{
		return;
	}
	if (!bTouchDragging)
	{
		AvatarPawn->BeginCameraDrag(TouchStartPosition, true);
		bTouchDragging = true;
	}
	AvatarPawn->DoCameraDragScrollCommand(Instance.GetValue().Get<FVector2D>());
}

void USunriseSelectionAbility::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	const bool bWasDragging = bTouchDragging;
	bTouchDragging = false;
	if (AvatarPawn.IsValid())
	{
		AvatarPawn->EndCameraDrag();
	}
	if (!CanInteract() || bWasDragging)
	{
		return;
	}
	FHitResult Hit;
	if (GetSunriseController()->GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, true, Hit))
	{
		if (!DoSelectCommand(Hit.ImpactPoint, true))
		{
			OrderFromHit(Hit);
		}
	}
}

void USunriseSelectionAbility::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	if (!CanInteract())
	{
		return;
	}
	const FVector2D Position = Value.Get<FVector2D>();
	SelectBox(StartingBoxSelectionPosition, Position);
	if (ASunriseHUD* HUD = Cast<ASunriseHUD>(GetSunriseController()->GetHUD()))
	{
		HUD->DragSelectUpdate(StartingBoxSelectionPosition, Position - StartingBoxSelectionPosition, Position, true);
	}
}

void USunriseSelectionAbility::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	CancelSelectHold(Value);
}

void USunriseSelectionAbility::ActivateHeroSquadAbility(const FInputActionValue& Value)
{
	SubmitOrder(SunriseOrders::Squad, FVector::ZeroVector);
}
