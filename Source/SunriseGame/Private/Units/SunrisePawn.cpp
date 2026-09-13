// Copyright Epic Games, Inc. All Rights Reserved.
#include "Units/SunrisePawn.h"

#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Camera/ModularCameraComponent.h"
#include "Components/ModularHeroComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Pawn/Components/ModularPawnExtensionComponent.h"
#include "Player/Camera/SunriseTopDownCameraMode.h"
#include "Player/SunrisePlayerController.h"
#include "Teams/Components/ModularTeamActorComponent.h"
#include "Units/SunriseUnit.h"
#include "Widgets/Input/SVirtualJoystick.h"

ASunrisePawn::ASunrisePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	TeamComponent = CreateDefaultSubobject<UModularTeamActorComponent>(TEXT("Team"));
	TeamComponent->OnTeamChanged.AddDynamic(this, &ThisClass::HandleTeamChanged);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	ModularAbilitySystemComponent = CreateDefaultSubobject<UModularAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	HeroComponent = CreateDefaultSubobject<UModularHeroComponent>(TEXT("HeroComponent"));
	PawnExtensionComponent = CreateDefaultSubobject<UModularPawnExtensionComponent>(TEXT("ExtensionComponent"));
	CameraComponent = CreateDefaultSubobject<UModularCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->DetermineCameraModeDelegate.BindLambda(
		[]()
		{
			return TSubclassOf<UModularCameraMode>(USunriseTopDownCameraMode::StaticClass());
		});
	CameraMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("CameraMovement"));
	CameraMovement->bConstrainToPlane = true;
	CameraMovement->SetPlaneConstraintNormal(FVector::UpVector);
	CameraMovement->MaxSpeed = 2200.0f;
	CameraMovement->Acceleration = 8000.0f;
	CameraMovement->Deceleration = 8000.0f;
}

void ASunrisePawn::BeginPlay()
{
	Super::BeginPlay();

	CameraZoom = FMath::Clamp(DefaultZoom, MinZoomLevel, MaxZoomLevel);
	PawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtensionComponent->OnAbilitySystemUninitialized_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
}

void ASunrisePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelInteraction();
	ReleaseRTSInput();

	PawnExtensionComponent->UninitializeAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ASunrisePawn::GetAbilitySystemComponent() const
{
	return PawnExtensionComponent->GetModularAbilitySystemComponent();
}

void ASunrisePawn::OnAbilitySystemInitialized()
{
	// TODO: Refresh info inside components
}

void ASunrisePawn::OnAbilitySystemUninitialized()
{
	// TODO: Refresh info inside components
}

void ASunrisePawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	PawnExtensionComponent->HandleControllerChanged();
}

void ASunrisePawn::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	PawnExtensionComponent->HandlePlayerStateReplicated();
}

void ASunrisePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PawnExtensionComponent->SetupPlayerInputComponent();
	InitializeRTSInput(PlayerInputComponent);
}

void ASunrisePawn::ReleaseRTSInput()
{
	bRTSInputReady = false;
	if (UEnhancedInputComponent* Input = BoundInput.Get())
	{
		for (uint32 Handle : InputBindingHandles)
		{
			Input->RemoveBindingByHandle(Handle);
		}
	}
	InputBindingHandles.Reset();
	BoundInput.Reset();
}

void ASunrisePawn::InitializeRTSInput(UInputComponent* PlayerInputComponent)
{
	ReleaseRTSInput();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	APlayerController* PC = GetController<APlayerController>();
	if (!Input || !PC || !PC->IsLocalController() || !PC->GetLocalPlayer())
	{
		return;
	}
	BoundInput = Input;
	if (ASunrisePlayerController* SunrisePC = Cast<ASunrisePlayerController>(PC))
	{
		SunrisePC->RefreshTouchControls();
	}

	if (MoveCameraAction)
	{
		InputBindingHandles.Add(Input->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &ThisClass::MoveCamera).GetHandle());
	}
	if (ZoomCameraAction)
	{
		InputBindingHandles.Add(Input->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &ThisClass::ZoomCamera).GetHandle());
	}
	if (ResetCameraAction)
	{
		InputBindingHandles.Add(Input->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &ThisClass::ResetCamera).GetHandle());
	}
	if (InteractHoldAction)
	{
		InputBindingHandles.Add(
			Input->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &ThisClass::InteractHoldStarted).GetHandle());
	}
	if (InteractHoldAction)
	{
		InputBindingHandles.Add(
			Input->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &ThisClass::InteractHoldTriggered).GetHandle());
	}
	if (InteractHoldAction)
	{
		InputBindingHandles.Add(
			Input->BindAction(InteractHoldAction, ETriggerEvent::Completed, this, &ThisClass::InteractHoldCompleted).GetHandle());
	}
	if (InteractHoldAction)
	{
		InputBindingHandles.Add(
			Input->BindAction(InteractHoldAction, ETriggerEvent::Canceled, this, &ThisClass::InteractHoldCompleted).GetHandle());
	}

	bRTSInputReady = true;
}

bool ASunrisePawn::ShouldUseTouchControls() const
{
	return bForceTouchControls || SVirtualJoystick::ShouldDisplayTouchInterface();
}

bool ASunrisePawn::CanControlCamera() const
{
	const ASunrisePlayerController* PC = GetController<ASunrisePlayerController>();
	return bRTSInputReady && PC && PC->IsLocalController() && PC->AreCommandsEnabled() && !PC->IsPaused();
}

void ASunrisePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsLocallyControlled())
	{
		return;
	}
	TryFocusControlledHero();
	if (!CanControlCamera())
	{
		return;
	}
	if (bCameraDragActive && !bTouchCameraDrag)
	{
		FVector2D Cursor;
		if (!TryGetMouseLocationInsideViewport(Cursor))
		{
			bCameraDragInputSuspended = true;
		}
	}
	ApplyEdgeScroll(DeltaSeconds);
	ClampCameraToBounds();
}

void ASunrisePawn::CancelInteraction()
{
	EndCameraDrag();
	CameraMovement->StopMovementImmediately();
}

void ASunrisePawn::MoveCamera(const FInputActionValue& Value)
{
	if (!CanControlCamera())
	{
		return;
	}
	const FVector Axis =
		Value.GetValueType() == EInputActionValueType::Axis3D ? Value.Get<FVector>() : FVector(Value.Get<FVector2D>(), 0.0f);
	FVector Forward, Right;
	GetCameraGroundBasis(Forward, Right);
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
	AddActorWorldRotation(FRotator(0.0f, Axis.Z * YawRotationSpeed * GetWorld()->GetDeltaSeconds(), 0.0f));
}

void ASunrisePawn::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}
void ASunrisePawn::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
}

void ASunrisePawn::DoCameraModifyZoomCommand(float Delta)
{
	if (CanControlCamera() && FMath::IsFinite(Delta))
	{
		CameraZoom = FMath::Clamp(CameraZoom + Delta, MinZoomLevel, MaxZoomLevel);
	}
}

void ASunrisePawn::DoCameraResetZoomCommand()
{
	if (CanControlCamera())
	{
		CameraZoom = FMath::Clamp(DefaultZoom, MinZoomLevel, MaxZoomLevel);
	}
}

void ASunrisePawn::DoCameraSetZoomPercentageCommand(float Percentage)
{
	if (CanControlCamera() && FMath::IsFinite(Percentage))
	{
		CameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));
	}
}

float ASunrisePawn::GetDefaultZoomPercentage() const
{
	return FMath::Clamp((DefaultZoom - MinZoomLevel) / FMath::Max(1.0f, MaxZoomLevel - MinZoomLevel), 0.0f, 1.0f);
}

float ASunrisePawn::GetZoomPercentage() const
{
	return FMath::Clamp((CameraZoom - MinZoomLevel) / FMath::Max(1.0f, MaxZoomLevel - MinZoomLevel), 0.0f, 1.0f);
}

bool ASunrisePawn::TryGetMouseLocationInsideViewport(FVector2D& Position) const
{
	const APlayerController* PC = GetController<APlayerController>();
	float X = 0.0f, Y = 0.0f;
	int32 Width = 0, Height = 0;
	if (!PC || !PC->GetMousePosition(X, Y))
	{
		return false;
	}
	PC->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0 || X < 0.0f || Y < 0.0f || X >= Width || Y >= Height)
	{
		return false;
	}
	Position = FVector2D(X, Y);
	return true;
}

void ASunrisePawn::BeginCameraDrag(const FVector2D& Position, bool bTouch)
{
	if (!CanControlCamera())
	{
		return;
	}
	bDidCameraDrag = false;
	bCameraDragActive = true;
	bCameraDragInputSuspended = false;
	bTouchCameraDrag = bTouch;
	StartingDragScrollPosition = Position;
	CameraDragStartLocation = GetActorLocation();
}

void ASunrisePawn::EndCameraDrag()
{
	if (bCameraDragActive && bDidCameraDrag && GetWorld())
	{
		LastCameraDragTime = GetWorld()->GetTimeSeconds();
	}
	bCameraDragActive = false;
	bCameraDragInputSuspended = false;
	bTouchCameraDrag = false;
}

bool ASunrisePawn::ConsumeCameraDragClick()
{
	const bool bSuppress = bDidCameraDrag || GetWorld()->GetTimeSeconds() - LastCameraDragTime < 0.15f;
	bDidCameraDrag = false;
	return bSuppress;
}

void ASunrisePawn::InteractHoldStarted(const FInputActionValue& Value)
{
	FVector2D Position;
	if (TryGetMouseLocationInsideViewport(Position))
	{
		BeginCameraDrag(Position);
	}
}

void ASunrisePawn::InteractHoldTriggered(const FInputActionValue& Value)
{
	FVector2D Position;
	if (TryGetMouseLocationInsideViewport(Position))
	{
		DoCameraDragScrollCommand(Position);
	}
	else
	{
		bCameraDragInputSuspended = true;
	}
}

void ASunrisePawn::InteractHoldCompleted(const FInputActionValue& Value)
{
	EndCameraDrag();
}

void ASunrisePawn::DoCameraDragScrollCommand(const FVector2D& Position)
{
	const APlayerController* PC = GetController<APlayerController>();
	if (!CanControlCamera() || !bCameraDragActive || !PC || Position.ContainsNaN())
	{
		return;
	}
	int32 Width = 0, Height = 0;
	PC->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0 || Position.X < 0.0f || Position.Y < 0.0f || Position.X >= Width || Position.Y >= Height)
	{
		bCameraDragInputSuspended = true;
		return;
	}
	if (bCameraDragInputSuspended)
	{
		StartingDragScrollPosition = Position;
		CameraDragStartLocation = GetActorLocation();
		bCameraDragInputSuspended = false;
		return;
	}
	const FVector2D Delta = StartingDragScrollPosition - Position;
	if (Delta.SizeSquared() < 16.0f)
	{
		return;
	}
	bDidCameraDrag = true;
	FVector Forward, Right;
	GetCameraGroundBasis(Forward, Right);
	// OrthoWidth is horizontal world width, so horizontal pixels determine the scale.
	const float WorldUnitsPerPixel = CameraZoom / Width;
	SetActorLocation(CameraDragStartLocation + (Right * Delta.X - Forward * Delta.Y) * WorldUnitsPerPixel * DragMultiplier, true);
}

void ASunrisePawn::GetCameraGroundBasis(FVector& Forward, FVector& Right) const
{
	const FRotator Yaw(0.0f, GetActorRotation().Yaw, 0.0f);
	Forward = Yaw.RotateVector(FVector::ForwardVector);
	Right = Yaw.RotateVector(FVector::RightVector);
}

void ASunrisePawn::ApplyEdgeScroll(float DeltaSeconds)
{
	if (ShouldUseTouchControls() || bCameraDragActive)
	{
		return;
	}
	FVector2D Cursor;
	if (!TryGetMouseLocationInsideViewport(Cursor))
	{
		return;
	}
	int32 Width = 0, Height = 0;
	GetController<APlayerController>()->GetViewportSize(Width, Height);
	const float Border = FMath::Max(1.0f, EdgeScrollBorder);
	FVector Forward, Right;
	GetCameraGroundBasis(Forward, Right);
	FVector Direction = FVector::ZeroVector;
	if (Cursor.X <= Border)
	{
		Direction -= Right * (1.0f - Cursor.X / Border);
	}
	else if (Cursor.X >= Width - Border)
	{
		Direction += Right * (1.0f - (Width - Cursor.X) / Border);
	}
	if (Cursor.Y <= Border)
	{
		Direction += Forward * (1.0f - Cursor.Y / Border);
	}
	else if (Cursor.Y >= Height - Border)
	{
		Direction -= Forward * (1.0f - (Height - Cursor.Y) / Border);
	}
	const float ZoomScale = CameraZoom / FMath::Max(1.0f, DefaultZoom);
	AddActorWorldOffset(Direction.GetClampedToMaxSize(1.0f) * EdgeScrollSpeed * ZoomScale * DeltaSeconds, true);
}

void ASunrisePawn::ClampCameraToBounds()
{
	if (bConstrainCamera)
	{
		FVector Location = GetActorLocation();
		Location.X = FMath::Clamp(Location.X, CameraBoundsMin.X, CameraBoundsMax.X);
		Location.Y = FMath::Clamp(Location.Y, CameraBoundsMin.Y, CameraBoundsMax.Y);
		SetActorLocation(Location);
	}
}

void ASunrisePawn::FocusCameraOnHero(ASunriseUnit* Hero)
{
	const UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(GetController());
	if (bHeroFocusConsumed || !IsLocallyControlled() || !IsValid(Hero) || !Hero->IsAlive() || !Manager || !Manager->CanControlEntity(Hero))
	{
		return;
	}
	FVector Location = GetActorLocation();
	Location.X = Hero->GetActorLocation().X;
	Location.Y = Hero->GetActorLocation().Y;
	SetActorLocation(Location, true);
	ClampCameraToBounds();
	bHeroFocusConsumed = true;
}

ESunriseTeam ASunrisePawn::GetTeam() const
{
	return StaticCast<ESunriseTeam>(TeamComponent->GetTeamId());
}

int32 ASunrisePawn::GetTeamId() const
{
	return TeamComponent->GetTeamId();
}

void ASunrisePawn::SetTeam(ESunriseTeam NewTeam)
{
	SetTeamId(NewTeam == ESunriseTeam::Friendly ? 0 : NewTeam == ESunriseTeam::Enemy ? 1 : INDEX_NONE);
	if (NewTeam != ESunriseTeam::Friendly && bSelected)
	{
		ISunriseSelectable::Execute_SetSunriseSelected(this, false);
	}
}

void ASunrisePawn::SetTeamId(int32 NewTeamId)
{
	TeamComponent->SetTeamId(NewTeamId);
}

void ASunrisePawn::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamComponent->SetGenericTeamId(NewTeamId);
}

FGenericTeamId ASunrisePawn::GetGenericTeamId() const
{
	return TeamComponent->GetGenericTeamId();
}

FOnTeamIndexChangedDelegate* ASunrisePawn::GetOnTeamIndexChangedDelegate()
{
	return TeamComponent->GetOnTeamIndexChangedDelegate();
}

void ASunrisePawn::HandleTeamChanged(UObject* TeamAgent, int32 PreviousTeamId, int32 NewTeamId)
{
}

void ASunrisePawn::TryFocusControlledHero()
{
	if (bHeroFocusConsumed)
	{
		return;
	}
	if (const UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(GetController()))
	{
		for (AActor* Entity : Manager->GetControlledEntities())
		{
			if (ASunriseUnit* Hero = Cast<ASunriseUnit>(Entity); Hero && Hero->IsHero())
			{
				FocusCameraOnHero(Hero);
				if (bHeroFocusConsumed)
				{
					break;
				}
			}
		}
	}
}
