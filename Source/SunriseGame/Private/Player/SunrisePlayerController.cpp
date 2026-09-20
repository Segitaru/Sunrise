// Copyright Epic Games, Inc. All Rights Reserved.
#include "Player/SunrisePlayerController.h"

#include "Abilities/SunriseSelectionAbility.h"
#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameFramework/PlayerState.h"
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
	if (!HasAuthority())
	{
		return;
	}
	ObservePlayerStateTeam(PlayerState);
	if (IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState))
	{
		TeamAgent->SetGenericTeamId(NewTeamId);
	}
}

FGenericTeamId ASunrisePlayerController::GetGenericTeamId() const
{
	const IModularTeamAgentInterface* TeamAgent = IsValid(PlayerState) ? Cast<IModularTeamAgentInterface>(PlayerState) : nullptr;
	return TeamAgent ? TeamAgent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

void ASunrisePlayerController::ObservePlayerStateTeam(APlayerState* NewPlayerState)
{
	if (ObservedTeamPlayerState.Get() != NewPlayerState)
	{
		if (IModularTeamAgentInterface* Previous = Cast<IModularTeamAgentInterface>(ObservedTeamPlayerState.Get()))
		{
			Previous->GetTeamChangedDelegateChecked().RemoveDynamic(this, &ThisClass::HandlePlayerStateTeamChanged);
		}
		ObservedTeamPlayerState = NewPlayerState;
		if (IModularTeamAgentInterface* Current = Cast<IModularTeamAgentInterface>(NewPlayerState))
		{
			Current->GetTeamChangedDelegateChecked().AddUniqueDynamic(this, &ThisClass::HandlePlayerStateTeamChanged);
		}
	}
	const IModularTeamAgentInterface* TeamAgent = Cast<IModularTeamAgentInterface>(NewPlayerState);
	const int32 NewTeamId = TeamAgent ? GenericTeamIdToInteger(TeamAgent->GetGenericTeamId()) : INDEX_NONE;
	HandlePlayerStateTeamChanged(NewPlayerState, ControlledTeamId, NewTeamId);
}

void ASunrisePlayerController::HandlePlayerStateTeamChanged(UObject* TeamAgent, int32 OldTeamId, int32 NewTeamId)
{
	if (TeamAgent != ObservedTeamPlayerState.Get() || ControlledTeamId == NewTeamId)
	{
		return;
	}
	const int32 PreviousTeamId = ControlledTeamId;
	ControlledTeamId = NewTeamId;
	OnTeamChanged.Broadcast(this, PreviousTeamId, NewTeamId);
}

void ASunrisePlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	ObservePlayerStateTeam(PlayerState);
}

void ASunrisePlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ObservePlayerStateTeam(PlayerState);
}

void ASunrisePlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	ObservePlayerStateTeam(PlayerState);
}

void ASunrisePlayerController::CleanupPlayerState()
{
	ObservePlayerStateTeam(nullptr);
	Super::CleanupPlayerState();
}

void ASunrisePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ObservePlayerStateTeam(nullptr);
	Super::EndPlay(EndPlayReason);
}

void ASunrisePlayerController::BeginPlay()
{
	Super::BeginPlay();
	ObservePlayerStateTeam(PlayerState);
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
