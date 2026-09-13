// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ControllableEntities/IControllableEntity.h"
#include "CoreMinimal.h"
#include "Player/ModularPlayerController.h"
#include "Teams/System/ModularTeamAgentInterface.h"

#include "SunrisePlayerController.generated.h"

class ASunriseUnit;
class USunriseSelectionAbility;
class USunriseHeroSquadAbility;
class UControllableEntitiesManager;
class USunrisePauseMenuWidget;
class USunriseOverloadInfoWidget;
class USunriseTouchControls;

/** Player identity, control authority and UI. Pawn/GAS own camera and unit interaction. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunrisePlayerController : public AModularPlayerController,
												 public IIControllableEntity,
												 public IModularTeamAgentInterface
{
	GENERATED_BODY()
public:
	ASunrisePlayerController();

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual TScriptInterface<IIControllableEntity> GetControllingAgent() override;

	virtual void SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent) override;

	virtual FOnControllingAgentChanged* GetOnControllingAgentChangedDelegate() override { return &OnControllingAgentChanged; }

#pragma region IModularTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return IntegerToGenericTeamId(ControlledTeamId); }
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	int32 GetControlledTeamId() const { return ControlledTeamId; }

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Sunrise|Team")
	int32 ControlledTeamId = 0;

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChanged;
#pragma endregion IModularTeamAgentInterface

	void SetCommandsEnabled(bool bEnabled);
	bool AreCommandsEnabled() const { return bCommandsEnabled; }
	void RefreshTouchControls();

protected:
	void TogglePauseMenu();
	UFUNCTION()
	void OnRep_CommandsEnabled();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Control")
	TObjectPtr<UControllableEntitiesManager> ControllableEntitiesManager;

	UPROPERTY(EditAnywhere, Category = "Input")
	TSubclassOf<USunriseTouchControls> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<USunriseTouchControls> MobileControlsWidget;

	UPROPERTY(Transient)
	TObjectPtr<USunrisePauseMenuWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<USunriseOverloadInfoWidget> OverloadInfoWidget;

	UPROPERTY(ReplicatedUsing = OnRep_CommandsEnabled)
	bool bCommandsEnabled = true;
	UPROPERTY()
	FOnControllingAgentChanged OnControllingAgentChanged;
};
