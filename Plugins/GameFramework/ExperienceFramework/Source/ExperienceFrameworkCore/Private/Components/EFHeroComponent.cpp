// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/EFHeroComponent.h"

#include "Components/GameFrameworkComponentDelegates.h"
#include "EFLogChannels.h"
#include "EnhancedInputSubsystems.h"
#include "Logging/MessageLog.h"
/*#include "Player/EFPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Player/EFPlayerState.h"*/
/*#include "Character/EFPawnExtensionComponent.h"
#include "AbilitySystem/EFAbilitySystemComponent.h"*/
/*#include "Character/EFCharacter.h"
#include "Character/EFPawnData.h"*/
#include "Input/EFInputConfig.h"
/*#include "Input/EFInputComponent.h"*/
#include "Camera/EFCameraComponent.h"
#include "Camera/EFCameraMode.h"
#include "Components/GameFrameworkComponentManager.h"
#include "EFGameplayTags.h"
#include "InputMappingContext.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EFHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif // WITH_EDITOR

namespace EFHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
}; // namespace EFHero

const FName UEFHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UEFHeroComponent::NAME_ActorFeatureName("Hero");

UEFHeroComponent::UEFHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;
}

void UEFHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogExperienceFramework, Error,
			TEXT(
				"[UEFHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("EFHeroComponent", "NotOnPawnError",
				"has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("EFHeroComponent");

			FMessageLog(HeroMessageLogName)
				.Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));

			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool UEFHeroComponent::CanChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == EFGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == EFGameplayTags::InitState_Spawned && DesiredState == EFGameplayTags::InitState_DataAvailable)
	{
		/*// The player state is required.
		if (!GetPlayerState<AEFPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AEFPlayerController* EFPC = GetController<AEFPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !EFPC || !EFPC->GetLocalPlayer())
			{
				return false;
			}
		}*/

		return true;
	}
	else if (CurrentState == EFGameplayTags::InitState_DataAvailable && DesiredState == EFGameplayTags::InitState_DataInitialized)
	{
		/*// Wait for player state and extension component
		AEFPlayerState* EFPS = GetPlayerState<AEFPlayerState>();

		return EFPS && Manager->HasFeatureReachedInitState(Pawn, UEFPawnExtensionComponent::NAME_ActorFeatureName, EFGameplayTags::InitState_DataInitialized);*/
		return true;
	}
	else if (CurrentState == EFGameplayTags::InitState_DataInitialized && DesiredState == EFGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UEFHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == EFGameplayTags::InitState_DataAvailable && DesiredState == EFGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		/*AEFPlayerState* EFPS = GetPlayerState<AEFPlayerState>();
		if (!ensure(Pawn && EFPS))
		{
			return;
		}

		const UEFPawnData* PawnData = nullptr;

		if (UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UEFPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(EFPS->GetEFAbilitySystemComponent(), EFPS);
		}

		if (AEFPlayerController* EFPC = GetController<AEFPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}*/

		// Hook up the delegate for all pawns, in case we spectate later
		/*if (PawnData)
		{
			if (UEFCameraComponent* CameraComponent = UEFCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}*/
	}
}

void UEFHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	/*if (Params.FeatureName == UEFPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == EFGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}*/
}

void UEFHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {EFGameplayTags::InitState_Spawned, EFGameplayTags::InitState_DataAvailable,
		EFGameplayTags::InitState_DataInitialized, EFGameplayTags::InitState_GameplayReady};

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UEFHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	/*BindOnActorInitStateChanged(UEFPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);*/

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(EFGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UEFHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UEFHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	/*if (const UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UEFPawnData* PawnData = PawnExtComp->GetPawnData<UEFPawnData>())
		{
			if (const UEFInputConfig* InputConfig = PawnData->InputConfig)
			{
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);
							}
							
							FModifyContextOptions Options = {};
							Options.bIgnoreAllPressedKeysUntilRelease = false;
							// Actually add the config to the local player							
							Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
						}
					}
				}

				// The EF Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the UEFInputComponent or modify this component accordingly.
				UEFInputComponent* EFIC = Cast<UEFInputComponent>(PlayerInputComponent);
				if (ensureMsgf(EFIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UEFInputComponent or a subclass of it.")))
				{
					// Add the key mappings that may have been set by the player
					EFIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events. 
					TArray<uint32> BindHandles;
					EFIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out#1# BindHandles);

					EFIC->BindNativeAction(InputConfig, EFGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=#1# false);
					EFIC->BindNativeAction(InputConfig, EFGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=#1# false);
					EFIC->BindNativeAction(InputConfig, EFGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=#1# false);
					EFIC->BindNativeAction(InputConfig, EFGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=#1# false);
					EFIC->BindNativeAction(InputConfig, EFGameplayTags::InputTag_AutoRun, ETriggerEvent::Triggered, this, &ThisClass::Input_AutoRun, /*bLogIfNotFound=#1# false);
				}
			}
		}
	}*/

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UEFHeroComponent::AddAdditionalInputConfig(const UEFInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	/*if (const UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UEFInputComponent* EFIC = Pawn->FindComponentByClass<UEFInputComponent>();
		if (ensureMsgf(EFIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UEFInputComponent or a subclass of it.")))
		{
			EFIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out#1# BindHandles);
		}
	}*/
}

void UEFHeroComponent::RemoveAdditionalInputConfig(const UEFInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UEFHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UEFHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		/*if (const UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UEFAbilitySystemComponent* EFASC = PawnExtComp->GetEFAbilitySystemComponent())
			{
				EFASC->AbilityInputTagPressed(InputTag);
			}
		}	*/
	}
}

void UEFHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	/*if (const UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UEFAbilitySystemComponent* EFASC = PawnExtComp->GetEFAbilitySystemComponent())
		{
			EFASC->AbilityInputTagReleased(InputTag);
		}
	}*/
}

void UEFHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	/*if (AEFPlayerController* EFController = Cast<AEFPlayerController>(Controller))
	{
		EFController->SetIsAutoRunning(false);
	}*/

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UEFHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UEFHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * EFHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * EFHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UEFHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	/*if (AEFCharacter* Character = GetPawn<AEFCharacter>())
	{
		Character->ToggleCrouch();
	}*/
}

void UEFHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		/*if (AEFPlayerController* Controller = Cast<AEFPlayerController>(Pawn->GetController()))
		{
			// Toggle auto running
			Controller->SetIsAutoRunning(!Controller->GetIsAutoRunning());
		}	*/
	}
}

TSubclassOf<UEFCameraMode> UEFHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	/*if (UEFPawnExtensionComponent* PawnExtComp = UEFPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UEFPawnData* PawnData = PawnExtComp->GetPawnData<UEFPawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}*/

	return nullptr;
}

void UEFHeroComponent::SetAbilityCameraMode(TSubclassOf<UEFCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void UEFHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}
