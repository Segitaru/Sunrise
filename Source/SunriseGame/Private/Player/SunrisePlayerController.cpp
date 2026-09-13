// Copyright Epic Games, Inc. All Rights Reserved.
#include "Player/SunrisePlayerController.h"

#include "Abilities/SunriseSelectionAbility.h"
#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Net/UnrealNetwork.h"
#include "UI/SunriseTouchControls.h"
#include "UI/SunriseWidgets.h"
#include "Units/SunrisePawn.h"
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
	const int32 NewValue = GenericTeamIdToInteger(NewTeamId);
	if (!HasAuthority() || ControlledTeamId == NewValue)
	{
		return;
	}
	const int32 OldValue = ControlledTeamId;
	ControlledTeamId = NewValue;
	OnTeamChanged.Broadcast(this, OldValue, ControlledTeamId);
}

void ASunrisePlayerController::BeginPlay()
{
	Super::BeginPlay();
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
	RefreshTouchControls();
}

void ASunrisePlayerController::RefreshTouchControls()
{
	const ASunrisePawn* SunrisePawn = GetPawn<ASunrisePawn>();
	const bool bTouchControls = SunrisePawn ? SunrisePawn->ShouldUseTouchControls() : SVirtualJoystick::ShouldDisplayTouchInterface();
	if (IsLocalPlayerController() && bTouchControls && MobileControlsWidgetClass && !MobileControlsWidget)
	{
		MobileControlsWidget = CreateWidget<USunriseTouchControls>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(10);
			MobileControlsWidget->SetPlayerController(this);
		}
	}
}

void ASunrisePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ControlledTeamId);
	DOREPLIFETIME(ThisClass, bCommandsEnabled);
}

void ASunrisePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::TogglePauseMenu).bExecuteWhenPaused = true;
}

void ASunrisePlayerController::PostProcessInput(float DeltaTime, bool bGamePaused)
{
	if (const ASunrisePawn* SunrisePawn = GetPawn<ASunrisePawn>())
	{
		if (UModularAbilitySystemComponent* ASC = Cast<UModularAbilitySystemComponent>(SunrisePawn->GetAbilitySystemComponent()))
		{
			if (bCommandsEnabled && !bGamePaused)
			{
				ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
			}
			else
			{
				ASC->ClearAbilityInput();
			}
		}
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void ASunrisePlayerController::SetCommandsEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		bCommandsEnabled = bEnabled;
		OnRep_CommandsEnabled();
		ForceNetUpdate();
	}
}

void ASunrisePlayerController::OnRep_CommandsEnabled()
{
	if (!bCommandsEnabled)
	{
		if (ASunrisePawn* SunrisePawn = GetPawn<ASunrisePawn>())
		{
			SunrisePawn->CancelInteraction();
		}
	}
}

void ASunrisePlayerController::TogglePauseMenu()
{
	if (!bCommandsEnabled)
	{
		return;
	}
	const bool bPause = !IsPaused();
	SetPause(bPause);
	if (bPause)
	{
		if (ASunrisePawn* SunrisePawn = GetPawn<ASunrisePawn>())
		{
			SunrisePawn->CancelInteraction();
		}
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
	}
}
