// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/SunrisePlayerController.h"

#include "Abilities/SunriseHeroSquadAbility.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/Components/SunriseTopDownCameraComponent.h"
#include "UI/SunriseHUD.h"
#include "UI/SunriseTouchControls.h"
#include "UI/SunriseWidgets.h"
#include "Units/SunriseUnit.h"
#include "Units/SunriseUnitInterfaces.h"
#include "Widgets/Input/SVirtualJoystick.h"

ASunrisePlayerController::ASunrisePlayerController()
{
	ControllableEntitiesManager = CreateDefaultSubobject<UControllableEntitiesManager>(TEXT("ControllableEntitiesManager"));
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ASunrisePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASunrisePlayerController, ControlledTeamId);
	DOREPLIFETIME(ASunrisePlayerController, NextHeroSquadActivationTime);
}

TScriptInterface<IIControllableEntity> ASunrisePlayerController::GetControllingAgent()
{
	TScriptInterface<IIControllableEntity> Self;
	Self.SetObject(this);
	Self.SetInterface(this);
	return Self;
}

void ASunrisePlayerController::SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent)
{
	// A player controller is the root controlling agent and cannot be reassigned.
	(void)NewAgent;
}

void ASunrisePlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	const int32 NewValue = TFTeamIdToInteger(NewTeamId);
	if (!HasAuthority() || ControlledTeamId == NewValue)
	{
		return;
	}
	const int32 OldValue = ControlledTeamId;
	ControlledTeamId = NewValue;
	OnTeamChanged.Broadcast(this, OldValue, ControlledTeamId);
}

float ASunrisePlayerController::GetHeroSquadCooldownRemaining() const
{
	return GetWorld() ? FMath::Max(0.0f, NextHeroSquadActivationTime - GetWorld()->GetTimeSeconds()) : 0.0f;
}

void ASunrisePlayerController::ServerActivateHeroSquad_Implementation()
{
	if (!bAllowInteraction || !HeroSquadAbilityClass || !ControllableEntitiesManager)
	{
		return;
	}
	for (AActor* Entity : ControllableEntitiesManager->GetControlledEntities())
	{
		ASunriseUnit* Hero = Cast<ASunriseUnit>(Entity);
		if (Hero && Hero->IsHero() && Hero->IsAlive())
		{
			if (USunriseHeroSquadAbility::ActivateForHero(Hero, HeroSquadAbilityClass))
			{
				NextHeroSquadActivationTime = GetWorld()->GetTimeSeconds() + 30.0;
			}
			return;
		}
	}
}
void ASunrisePlayerController::BeginPlay()
{
	Super::BeginPlay();
	SunriseHUD = Cast<ASunriseHUD>(GetHUD());
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, nullptr, EMouseLockMode::DoNotLock, false);
	if (IsLocalPlayerController())
	{
		OverloadInfoWidget = CreateWidget<USunriseOverloadInfoWidget>(this, USunriseOverloadInfoWidget::StaticClass());
		if (OverloadInfoWidget)
		{
			OverloadInfoWidget->SetAlignmentInViewport(FVector2D(0.0f, 1.0f));

			OverloadInfoWidget->SetPositionInViewport(FVector2D(24.0f, -24.0f));

			OverloadInfoWidget->AddToViewport(5);
		}
	}
	if (IsLocalPlayerController() && ShouldUseTouchControls() && MobileControlsWidgetClass)
	{
		MobileControlsWidget = CreateWidget<USunriseTouchControls>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(10);
			MobileControlsWidget->SetPlayerController(this);
		}
	}
}

void ASunrisePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetPawn() != ControlledCameraPawn || (!TopDownCameraComponent && ControlledCameraPawn))
	{
		RefreshPawnFeatures();
	}
	ApplyPendingHeroFocus();
	PruneSelection();
	if (bCameraDragActive)
	{
		FVector2D CursorPosition;
		if (!TryGetMouseLocationInsideViewport(CursorPosition))
		{
			bCameraDragInputSuspended = true;
		}
	}
	if (bAllowInteraction && !IsPaused())
	{
		ApplyEdgeScroll(DeltaSeconds);
		ClampCameraToBounds();
	}
}

void ASunrisePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsLocalPlayerController())
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		UInputMappingContext* Context = ShouldUseTouchControls() ? TouchMappingContext : MouseMappingContext;
		if (Context)
		{
			Subsystem->AddMappingContext(Context, 0);
		}
	}

	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (SelectClickAction)
		{
			Enhanced->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::SelectClick);
		}
		if (SelectClickAdditiveAction)
		{
			Enhanced->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::SelectClickAdditive);
		}
		if (SelectAllDoubleClickAction)
		{
			Enhanced->BindAction(
				SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::SelectAllDoubleClick);
		}
		if (SelectionModifierAction)
		{
			Enhanced->BindAction(
				SelectionModifierAction, ETriggerEvent::Started, this, &ASunrisePlayerController::SelectionModifierStarted);
			Enhanced->BindAction(
				SelectionModifierAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::SelectionModifierCompleted);
			Enhanced->BindAction(
				SelectionModifierAction, ETriggerEvent::Canceled, this, &ASunrisePlayerController::SelectionModifierCompleted);
		}
		if (InteractHoldAction)
		{
			Enhanced->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &ASunrisePlayerController::InteractHoldStarted);
			Enhanced->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::InteractHoldTriggered);
			Enhanced->BindAction(InteractHoldAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::InteractHoldCompleted);
			Enhanced->BindAction(InteractHoldAction, ETriggerEvent::Canceled, this, &ASunrisePlayerController::InteractHoldCompleted);
		}
		if (MoveCameraAction)
		{
			Enhanced->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::MoveCamera);
		}
		if (ZoomCameraAction)
		{
			Enhanced->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::ZoomCamera);
		}
		if (ResetCameraAction)
		{
			Enhanced->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::ResetCamera);
		}
		if (SelectHoldAction)
		{
			Enhanced->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &ASunrisePlayerController::SelectHoldStarted);
			Enhanced->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::SelectHoldTriggered);
			Enhanced->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::SelectHoldCompleted);
			Enhanced->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &ASunrisePlayerController::SelectHoldCompleted);
		}
		if (InteractClickAction)
		{
			Enhanced->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::InteractClick);
		}
		if (TouchPrimaryHoldAction)
		{
			Enhanced->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &ASunrisePlayerController::TouchPrimaryHoldStarted);
			Enhanced->BindAction(
				TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::TouchPrimaryHoldTriggered);
			Enhanced->BindAction(
				TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::TouchPrimaryHoldCompleted);
		}
		if (TouchSecondaryAction)
		{
			Enhanced->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &ASunrisePlayerController::TouchSecondaryTriggered);
			Enhanced->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &ASunrisePlayerController::TouchSecondaryCompleted);
		}
		if (HeroSquadAction)
		{
			Enhanced->BindAction(HeroSquadAction, ETriggerEvent::Started, this, &ASunrisePlayerController::ActivateHeroSquadAbility);
		}
	}
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASunrisePlayerController::TogglePauseMenu).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ASunrisePlayerController::StopSelectedUnits);
}

void ASunrisePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RefreshPawnFeatures();
	ApplyPendingHeroFocus();
}

void ASunrisePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshPawnFeatures();
	ApplyPendingHeroFocus();
}

void ASunrisePlayerController::DragSelectUnits(const TArray<ASunriseUnit*>& Units)
{
	DoDeselectAllUnitsCommand();
	for (ASunriseUnit* Unit : Units)
	{
		if (Unit && ISunriseSelectable::Execute_CanBeSelectedBy(Unit, this))
		{
			ControlledUnits.AddUnique(Unit);
			ISunriseSelectable::Execute_SetSunriseSelected(Unit, true);
		}
	}
}

const TArray<ASunriseUnit*>& ASunrisePlayerController::GetSelectedUnits()
{
	PruneSelection();
	return ControlledUnits;
}

FVector ASunrisePlayerController::GetMidPointFromSelectedUnits()
{
	FVector Destination;

	const auto& UnitArray = GetSelectedUnits();
	for (const auto& Unit : UnitArray)
	{
		Destination += Unit->GetActorLocation();
	}
	return Destination / UnitArray.Num();
}

float ASunrisePlayerController::GetDefaultZoomPercentage() const
{
	return FMath::Clamp((DefaultZoom - MinZoomLevel) / FMath::Max(1.0f, MaxZoomLevel - MinZoomLevel), 0.0f, 1.0f);
}

void ASunrisePlayerController::SetCommandsEnabled(bool bEnabled)
{
	bAllowInteraction = bEnabled;
	if (!bEnabled)
	{
		DoDeselectAllUnitsCommand();
	}
}

bool ASunrisePlayerController::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
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
		if (!Unit || !ISunriseSelectable::Execute_CanBeSelectedBy(Unit, this))
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

void ASunrisePlayerController::DoSelectAllUnitsOnScreenCommand()
{
	TArray<AActor*> Units;
	UGameplayStatics::GetAllActorsOfClass(this, ASunriseUnit::StaticClass(), Units);
	for (AActor* Actor : Units)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Actor);
		if (Unit && Unit->WasRecentlyRendered(0.25f) && ISunriseSelectable::Execute_CanBeSelectedBy(Unit, this))
		{
			ControlledUnits.AddUnique(Unit);
			ISunriseSelectable::Execute_SetSunriseSelected(Unit, true);
		}
	}
}

void ASunrisePlayerController::DoDeselectAllUnitsCommand()
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

void ASunrisePlayerController::DoToggleSelectAllUnitsCommand()
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

void ASunrisePlayerController::DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition)
{
	if (!ControlledCameraPawn || !bCameraDragActive)
	{
		return;
	}
	int32 Width = 0, Height = 0;
	GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0 || CurrentCursorPosition.X < 0.0f || CurrentCursorPosition.X >= Width || CurrentCursorPosition.Y < 0.0f ||
		CurrentCursorPosition.Y >= Height)
	{
		bCameraDragInputSuspended = true;
		return;
	}
	if (bCameraDragInputSuspended)
	{
		// Resume from the first valid sample instead of applying the unobservable outside delta.
		StartingDragScrollPosition = CurrentCursorPosition;
		CameraDragStartLocation = ControlledCameraPawn->GetActorLocation();
		bCameraDragInputSuspended = false;
		return;
	}
	const FVector2D PixelDelta = StartingDragScrollPosition - CurrentCursorPosition;
	if (PixelDelta.SizeSquared() < 16.0f)
	{
		return;
	}
	bDidCameraDrag = true;
	const float WorldUnitsPerPixel = CameraZoom / FMath::Max(1, Height);
	FVector Forward, Right;
	GetCameraGroundBasis(Forward, Right);
	const FVector WorldOffset = (Right * PixelDelta.X - Forward * PixelDelta.Y) * WorldUnitsPerPixel * DragMultiplier;
	ControlledCameraPawn->SetActorLocation(CameraDragStartLocation + WorldOffset, true);
}

void ASunrisePlayerController::DoMoveUnitsCommand(const FVector& GoalLocation)
{
	PruneSelection();
	const int32 Count = ControlledUnits.Num();
	if (Count == 0)
	{
		return;
	}
	const int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
	const int32 Rows = FMath::CeilToInt(static_cast<float>(Count) / Columns);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 Row = Index / Columns;
		const int32 Column = Index % Columns;
		const FVector Destination =
			GoalLocation + FVector((Row - (Rows - 1) * 0.5f) * FormationSpacing, (Column - (Columns - 1) * 0.5f) * FormationSpacing, 0.0f);
		ISunriseOrderReceiver::Execute_IssueMoveOrder(ControlledUnits[Index], Destination);
	}
	BP_CursorFeedback(GoalLocation, true);
}

void ASunrisePlayerController::DoTargetUnitsCommand(ASunriseUnit* Target)
{
	if (!Target || !Target->IsAlive())
	{
		return;
	}
	for (int32 Index = 0; Index < ControlledUnits.Num(); ++Index)
	{
		ASunriseUnit* Unit = ControlledUnits[Index];
		if (!IsValid(Unit))
		{
			continue;
		}
		if (Unit->GetUnitRole() != ESunriseUnitRole::Healer || Unit->CanTargetWithWeapon(Target))
		{
			ISunriseOrderReceiver::Execute_IssueTargetOrder(Unit, Target);
		}
		else
		{
			const FVector EscortOffset(0.0f, (Index - ControlledUnits.Num() * 0.5f) * FormationSpacing, 0.0f);
			ISunriseOrderReceiver::Execute_IssueMoveOrder(Unit, Target->GetActorLocation() + EscortOffset);
		}
	}
	BP_CursorFeedback(Target->GetActorLocation(), true);
}

void ASunrisePlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	if (!ControlledCameraPawn)
	{
		return;
	}
	CameraZoom = FMath::Clamp(CameraZoom + ZoomDelta, MinZoomLevel, MaxZoomLevel);
	if (TopDownCameraComponent)
	{
		TopDownCameraComponent->SetZoom(CameraZoom);
	}
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
}

void ASunrisePlayerController::DoCameraResetZoomCommand()
{
	CameraZoom = DefaultZoom;
	if (TopDownCameraComponent)
	{
		TopDownCameraComponent->SetZoom(CameraZoom);
	}
}

void ASunrisePlayerController::DoCameraSetZoomPercentageCommand(float Percentage)
{
	CameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));
	if (TopDownCameraComponent)
	{
		TopDownCameraComponent->SetZoom(CameraZoom);
	}
}
void ASunrisePlayerController::FocusCameraOnHero(ASunriseUnit* Hero)
{
	if (bHeroFocusConsumed || !IsLocalPlayerController() || !IsValid(Hero) || !Hero->IsAlive())
	{
		return;
	}

	PendingHeroFocus = Hero;
	ApplyPendingHeroFocus();
}
void ASunrisePlayerController::RefreshPawnFeatures()
{
	ControlledCameraPawn = GetPawn();
	TopDownCameraComponent = USunriseTopDownCameraComponent::Find(ControlledCameraPawn);
	SunriseHUD = Cast<ASunriseHUD>(GetHUD());
	if (TopDownCameraComponent && TopDownCameraComponent->GetCamera())
	{
		DefaultZoom = CameraZoom = TopDownCameraComponent->GetCamera()->OrthoWidth;
	}
}

bool ASunrisePlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ASunrisePlayerController::MoveCamera(const FInputActionValue& Value)
{
	if (!bAllowInteraction || !ControlledCameraPawn)
	{
		return;
	}
	const FVector Axis = Value.Get<FVector>();
	FVector Forward;
	FVector Right;
	GetCameraGroundBasis(Forward, Right);
	ControlledCameraPawn->AddMovementInput(Forward, Axis.Y);
	ControlledCameraPawn->AddMovementInput(Right, Axis.X);
	if (TopDownCameraComponent)
	{
		TopDownCameraComponent->RotateYaw(Axis.Z, GetWorld()->GetDeltaSeconds());
	}
}

void ASunrisePlayerController::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}

void ASunrisePlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
}

void ASunrisePlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();
	DraggedCommandUnit = nullptr;
	FHitResult Hit;
	if (bAllowInteraction && GetHitUnderCursor(Hit))
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Hit.GetActor());
		if (Unit && ISunriseSelectable::Execute_CanBeSelectedBy(Unit, this))
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
			if (SunriseHUD)
			{
				SunriseHUD->CommandDragUpdate(Unit, StartingBoxSelectionPosition, true);
			}
		}
	}
}

void ASunrisePlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{
	if (!bAllowInteraction)
	{
		return;
	}
	const FVector2D Current = GetMouseLocationForPlayer();
	if (DraggedCommandUnit)
	{
		if (SunriseHUD)
		{
			SunriseHUD->CommandDragUpdate(DraggedCommandUnit, Current, true);
		}
		return;
	}
	if (SunriseHUD && FVector2D::Distance(Current, StartingBoxSelectionPosition) > 5.0f)
	{
		SunriseHUD->DragSelectUpdate(StartingBoxSelectionPosition, Current - StartingBoxSelectionPosition, Current, true);
	}
}

void ASunrisePlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	const bool bCompletedBoxSelection =
		!DraggedCommandUnit && FVector2D::Distance(GetMouseLocationForPlayer(), StartingBoxSelectionPosition) > 5.0f;
	if (DraggedCommandUnit)
	{
		const FVector2D Current = GetMouseLocationForPlayer();
		if (FVector2D::Distance(Current, StartingBoxSelectionPosition) > 8.0f)
		{
			FHitResult Hit;
			if (GetHitUnderCursor(Hit))
			{
				if (ASunriseUnit* Target = Cast<ASunriseUnit>(Hit.GetActor()); Target && Target != DraggedCommandUnit)
				{
					ISunriseOrderReceiver::Execute_IssueTargetOrder(DraggedCommandUnit, Target);
					BP_CursorFeedback(Target->GetActorLocation(), true);
				}
				else if (AActor* TargetActor = Hit.GetActor(); TargetActor && TargetActor->Implements<UOverloadHackable>())
				{
					if (UOverloadInteractorComponent* Interactor = DraggedCommandUnit->FindComponentByClass<UOverloadInteractorComponent>())
					{
						Interactor->RequestHack(TargetActor, true);
						BP_CursorFeedback(IOverloadHackable::Execute_GetHackLocation(TargetActor), true);
					}
				}
				else
				{
					ISunriseOrderReceiver::Execute_IssueMoveOrder(DraggedCommandUnit, Hit.ImpactPoint);
					BP_CursorFeedback(Hit.ImpactPoint, true);
				}
			}
		}
		if (SunriseHUD)
		{
			SunriseHUD->CommandDragUpdate(nullptr, FVector2D::ZeroVector, false);
		}
		DraggedCommandUnit = nullptr;
	}
	if (bCompletedBoxSelection && GetWorld())
	{
		LastBoxSelectionTime = GetWorld()->GetTimeSeconds();
	}
	if (SunriseHUD)
	{
		SunriseHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void ASunrisePlayerController::SelectClick(const FInputActionValue& Value)
{
	if (GetWorld() && GetWorld()->GetTimeSeconds() - LastBoxSelectionTime <= 0.15f)
	{
		return;
	}
	if (bSuppressNextSelectClick)
	{
		bSuppressNextSelectClick = false;
		return;
	}
	FVector Location;
	if (bAllowInteraction && GetLocationUnderCursor(Location))
	{
		DoSelectCommand(Location, bSelectionModifier);
	}
}

void ASunrisePlayerController::SelectClickAdditive(const FInputActionValue& Value)
{
	FVector Location;
	if (bAllowInteraction && GetLocationUnderCursor(Location))
	{
		DoSelectCommand(Location, true);
	}
}

void ASunrisePlayerController::SelectAllDoubleClick(const FInputActionValue& Value)
{
	if (bAllowInteraction)
	{
		DoSelectAllUnitsOnScreenCommand();
	}
}

void ASunrisePlayerController::SelectionModifierStarted(const FInputActionValue& Value)
{
	bSelectionModifier = true;
}

void ASunrisePlayerController::SelectionModifierCompleted(const FInputActionValue& Value)
{
	bSelectionModifier = false;
}

void ASunrisePlayerController::InteractHoldStarted(const FInputActionValue& Value)
{
	bDidCameraDrag = false;
	FVector2D CursorPosition;
	if (!bAllowInteraction || !TryGetMouseLocationInsideViewport(CursorPosition))
	{
		bCameraDragActive = false;
		bCameraDragInputSuspended = true;
		return;
	}
	StartingDragScrollPosition = CursorPosition;
	CameraDragStartLocation = ControlledCameraPawn ? ControlledCameraPawn->GetActorLocation() : FVector::ZeroVector;
	bCameraDragActive = true;
	bCameraDragInputSuspended = false;
}

void ASunrisePlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{
	if (!bAllowInteraction || !bCameraDragActive)
	{
		return;
	}
	FVector2D CursorPosition;
	if (!TryGetMouseLocationInsideViewport(CursorPosition))
	{
		bCameraDragInputSuspended = true;
		return;
	}
	DoCameraDragScrollCommand(CursorPosition);
}

void ASunrisePlayerController::InteractHoldCompleted(const FInputActionValue& Value)
{
	if (bDidCameraDrag)
	{
		LastCameraDragTime = GetWorld()->GetTimeSeconds();
	}
	bCameraDragActive = false;
	bCameraDragInputSuspended = false;
}
void ASunrisePlayerController::InteractClick(const FInputActionValue& Value)
{
	if (bDidCameraDrag || GetWorld()->GetTimeSeconds() - LastCameraDragTime < 0.15f)
	{
		bDidCameraDrag = false;
		return;
	}
	if (!bAllowInteraction || ControlledUnits.IsEmpty())
	{
		return;
	}
	FHitResult Hit;
	if (!GetHitUnderCursor(Hit))
	{
		return;
	}
	ASunriseUnit* Target = Cast<ASunriseUnit>(Hit.GetActor());
	if (!Target && Hit.GetComponent())
	{
		Target = Cast<ASunriseUnit>(Hit.GetComponent()->GetOwner());
	}
	if (Target)
	{
		DoTargetUnitsCommand(Target);
		return;
	}
	if (AActor* TargetActor = Hit.GetActor(); TargetActor && TargetActor->Implements<UOverloadHackable>())
	{
		bool bIssuedHack = false;
		for (ASunriseUnit* Unit : ControlledUnits)
		{
			if (UOverloadInteractorComponent* Interactor =
					IsValid(Unit) ? Unit->FindComponentByClass<UOverloadInteractorComponent>() : nullptr)
			{
				bIssuedHack |= Interactor->RequestHack(TargetActor, true);
			}
		}
		BP_CursorFeedback(IOverloadHackable::Execute_GetHackLocation(TargetActor), bIssuedHack);
		return;
	}
	DoMoveUnitsCommand(Hit.ImpactPoint);
}
void ASunrisePlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	StartingDragScrollPosition = Value.Get<FVector2D>();
}

void ASunrisePlayerController::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	const FVector2D Position = Instance.GetValue().Get<FVector2D>();
	StartingBoxSelectionPosition = Position;
	if (Instance.GetElapsedTime() > TouchDragScrollHoldTime)
	{
		DoCameraDragScrollCommand(Position);
		LastTouchDragScrollTime = GetWorld()->GetTimeSeconds();
	}
}

void ASunrisePlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	if (GetWorld()->GetTimeSeconds() - LastTouchDragScrollTime <= 0.1f)
	{
		return;
	}
	const FVector Location = ProjectTouchPointToWorldSpace();
	if (!DoSelectCommand(Location, true))
	{
		DoMoveUnitsCommand(Location);
	}
}

void ASunrisePlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	const FVector2D Position = Value.Get<FVector2D>();
	if (SunriseHUD)
	{
		SunriseHUD->DragSelectUpdate(StartingBoxSelectionPosition, Position - StartingBoxSelectionPosition, Position, true);
	}
}

void ASunrisePlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	if (SunriseHUD)
	{
		SunriseHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void ASunrisePlayerController::ActivateHeroSquadAbility(const FInputActionValue& Value)
{
	if (!bAllowInteraction)
	{
		return;
	}
	if (HasAuthority())
	{
		ServerActivateHeroSquad_Implementation();
	}
	else
	{
		ServerActivateHeroSquad();
	}
}
void ASunrisePlayerController::TogglePauseMenu()
{
	if (!bAllowInteraction)
	{
		return;
	}
	const bool bPause = !IsPaused();
	SetPause(bPause);
	if (bPause)
	{
		PauseMenuWidget = CreateWidget<USunrisePauseMenuWidget>(this, USunrisePauseMenuWidget::StaticClass());
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport(90);
			UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, PauseMenuWidget, EMouseLockMode::DoNotLock, false);
		}
	}
	else if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
		UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, nullptr, EMouseLockMode::DoNotLock, false);
		if (IsLocalPlayerController())
		{
			OverloadInfoWidget = CreateWidget<USunriseOverloadInfoWidget>(this, USunriseOverloadInfoWidget::StaticClass());
			if (OverloadInfoWidget)
			{
				OverloadInfoWidget->SetAlignmentInViewport(FVector2D(0.0f, 1.0f));

				OverloadInfoWidget->SetPositionInViewport(FVector2D(24.0f, -24.0f));

				OverloadInfoWidget->AddToViewport(5);
			}
		}
	}
}

void ASunrisePlayerController::StopSelectedUnits()
{
	for (ASunriseUnit* Unit : ControlledUnits)
	{
		if (IsValid(Unit))
		{
			ISunriseOrderReceiver::Execute_StopOrder(Unit);
		}
	}
}

void ASunrisePlayerController::ApplyEdgeScroll(float DeltaSeconds) const
{
	if (!ControlledCameraPawn || ShouldUseTouchControls() || bCameraDragActive)
	{
		return;
	}
	FVector2D CursorPosition;
	if (!TryGetMouseLocationInsideViewport(CursorPosition))
	{
		return;
	}
	const float MouseX = CursorPosition.X;
	const float MouseY = CursorPosition.Y;
	int32 Width = 0, Height = 0;
	GetViewportSize(Width, Height);
	FVector Forward, Right;
	GetCameraGroundBasis(Forward, Right);
	FVector Direction = FVector::ZeroVector;
	if (MouseX <= EdgeScrollBorder)
	{
		Direction -= Right * (1.0f - MouseX / EdgeScrollBorder);
	}
	else if (MouseX >= Width - EdgeScrollBorder)
	{
		Direction += Right * (1.0f - (Width - MouseX) / EdgeScrollBorder);
	}
	if (MouseY <= EdgeScrollBorder)
	{
		Direction += Forward * (1.0f - MouseY / EdgeScrollBorder);
	}
	else if (MouseY >= Height - EdgeScrollBorder)
	{
		Direction -= Forward * (1.0f - (Height - MouseY) / EdgeScrollBorder);
	}
	const float ZoomScale = CameraZoom / FMath::Max(1.0f, DefaultZoom);
	ControlledCameraPawn->AddActorWorldOffset(Direction.GetClampedToMaxSize(1.0f) * EdgeScrollSpeed * ZoomScale * DeltaSeconds, true);
}

void ASunrisePlayerController::ApplyPendingHeroFocus()
{
	if (bHeroFocusConsumed || !IsValid(PendingHeroFocus) || !PendingHeroFocus->IsAlive() || !ControlledCameraPawn)
	{
		return;
	}

	FVector CameraLocation = ControlledCameraPawn->GetActorLocation();
	const FVector HeroLocation = PendingHeroFocus->GetActorLocation();
	CameraLocation.X = HeroLocation.X;
	CameraLocation.Y = HeroLocation.Y;
	ControlledCameraPawn->SetActorLocation(CameraLocation, true);
	ClampCameraToBounds();
	PendingHeroFocus = nullptr;
	bHeroFocusConsumed = true;
}

void ASunrisePlayerController::GetCameraGroundBasis(FVector& OutForward, FVector& OutRight) const
{
	const UCameraComponent* Camera = TopDownCameraComponent ? TopDownCameraComponent->GetCamera() : nullptr;
	OutForward = Camera ? Camera->GetForwardVector() : FVector::ForwardVector;
	OutRight = Camera ? Camera->GetRightVector() : FVector::RightVector;
	OutForward.Z = 0.0f;
	OutRight.Z = 0.0f;
	OutForward = OutForward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
	OutRight = OutRight.GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
}

void ASunrisePlayerController::ClampCameraToBounds() const
{
	if (!bConstrainCamera || !ControlledCameraPawn)
	{
		return;
	}
	FVector Location = ControlledCameraPawn->GetActorLocation();
	Location.X = FMath::Clamp(Location.X, CameraBoundsMin.X, CameraBoundsMax.X);
	Location.Y = FMath::Clamp(Location.Y, CameraBoundsMin.Y, CameraBoundsMax.Y);
	ControlledCameraPawn->SetActorLocation(Location);
}

void ASunrisePlayerController::PruneSelection()
{
	for (int32 Index = ControlledUnits.Num() - 1; Index >= 0; --Index)
	{
		if (!IsValid(ControlledUnits[Index]) || !ControlledUnits[Index]->IsAlive())
		{
			ControlledUnits.RemoveAtSwap(Index);
		}
	}
}

FVector2D ASunrisePlayerController::GetMouseLocationForPlayer() const
{
	float X = 0.0f, Y = 0.0f;
	GetMousePosition(X, Y);
	return FVector2D(X, Y);
}

bool ASunrisePlayerController::TryGetMouseLocationInsideViewport(FVector2D& OutPosition) const
{
	float X = 0.0f;
	float Y = 0.0f;
	int32 Width = 0;
	int32 Height = 0;
	if (!GetMousePosition(X, Y))
	{
		return false;
	}
	GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0 || X < 0.0f || Y < 0.0f || X >= Width || Y >= Height)
	{
		return false;
	}
	OutPosition = FVector2D(X, Y);
	return true;
}

bool ASunrisePlayerController::GetLocationUnderCursor(FVector& Location)
{
	FHitResult Hit;
	if (!GetHitUnderCursor(Hit))
	{
		return false;
	}
	Location = Hit.ImpactPoint;
	return true;
}

bool ASunrisePlayerController::GetLocationUnderFinger(FVector& Location)
{
	FHitResult Hit;
	if (!GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, true, Hit))
	{
		return false;
	}
	Location = Hit.ImpactPoint;
	return true;
}

bool ASunrisePlayerController::GetHitUnderCursor(FHitResult& Hit) const
{
	return GetHitResultUnderCursorByChannel(SelectionTraceChannel, true, Hit);
}

FVector ASunrisePlayerController::ProjectTouchPointToWorldSpace()
{
	FVector Location;
	return GetLocationUnderFinger(Location) ? Location : FVector::ZeroVector;
}
