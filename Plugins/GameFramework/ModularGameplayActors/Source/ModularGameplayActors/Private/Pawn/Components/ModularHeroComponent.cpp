// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawn/Components/ModularHeroComponent.h"

#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Camera/ModularCameraComponent.h"
#include "Camera/ModularCameraMode.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Input/ModularInputComponent.h"
#include "Input/ModularInputConfig.h"
#include "InputMappingContext.h"
#include "Logging/MessageLog.h"
#include "ModularGameplayTags.h"
#include "ModularInputTypes.h"
#include "ModularLogChannels.h"
#include "Pawn/Components//ModularPawnExtensionComponent.h"
#include "Pawn/ModularCharacter.h"
#include "Pawn/ModularPawnData.h"
#include "Player/ModularPlayerController.h"
#include "Player/ModularPlayerState.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif // WITH_EDITOR

namespace ModularHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
}; // namespace ModularHero

const FName UModularHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UModularHeroComponent::NAME_ActorFeatureName("Hero");

UModularHeroComponent::UModularHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;
}

void UModularHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogModularGameplayActors, Error,
			TEXT(
				"[UModularHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("ModularHeroComponent", "NotOnPawnError",
				"has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("ModularHeroComponent");

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

bool UModularHeroComponent::CanChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == ModularGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == ModularGameplayTags::InitState_Spawned && DesiredState == ModularGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<AModularPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS =
				(Controller != nullptr) && (Controller->PlayerState != nullptr) && (Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AModularPlayerController* ModularPC = GetController<AModularPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !ModularPC || !ModularPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == ModularGameplayTags::InitState_DataAvailable && DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AModularPlayerState* ModularPS = GetPlayerState<AModularPlayerState>();

		return ModularPS && Manager->HasFeatureReachedInitState(Pawn, UModularPawnExtensionComponent::NAME_ActorFeatureName,
								ModularGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == ModularGameplayTags::InitState_DataInitialized && DesiredState == ModularGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UModularHeroComponent::HandleChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == ModularGameplayTags::InitState_DataAvailable && DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AModularPlayerState* ModularPS = GetPlayerState<AModularPlayerState>();
		if (!ensure(Pawn && ModularPS))
		{
			return;
		}

		const UModularPawnData* PawnData = nullptr;

		if (UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UModularPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(ModularPS->GetModularAbilitySystemComponent(), ModularPS);
		}

		if (AModularPlayerController* ModularPC = GetController<AModularPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		// Hook up the delegate for all pawns, in case we spectate later
		if (PawnData)
		{
			if (UModularCameraComponent* CameraComponent = UModularCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
	}
}

void UModularHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UModularPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == ModularGameplayTags::InitState_DataInitialized ||
			Params.FeatureState == ModularGameplayTags::InitState_GameplayReady)
		{
			// If the extension component says all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UModularHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = {ModularGameplayTags::InitState_Spawned, ModularGameplayTags::InitState_DataAvailable,
		ModularGameplayTags::InitState_DataInitialized, ModularGameplayTags::InitState_GameplayReady};

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UModularHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UModularPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(ModularGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UModularHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UModularHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

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

	Subsystem->ClearAllMappings();

	if (const UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UModularPawnData* PawnData = PawnExtComp->GetPawnData<UModularPawnData>())
		{
			if (const UModularInputConfig* InputConfig = PawnData->InputConfig)
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

				// The Modular Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the UModularInputComponent or modify this component accordingly.
				UModularInputComponent* ModularIC = Cast<UModularInputComponent>(PlayerInputComponent);
				if (ensureMsgf(ModularIC,
						TEXT(
							"Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UModularInputComponent or a subclass of it.")))
				{
					// Add the key mappings that may have been set by the player
					ModularIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events.
					TArray<uint32> BindHandles;
					ModularIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
						&ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					ModularIC->BindNativeAction(InputConfig, ModularGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this,
						&ThisClass::Input_Move, /*bLogIfNotFound=*/false);
					ModularIC->BindNativeAction(InputConfig, ModularGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this,
						&ThisClass::Input_LookMouse, /*bLogIfNotFound=*/false);
					ModularIC->BindNativeAction(InputConfig, ModularGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this,
						&ThisClass::Input_LookStick, /*bLogIfNotFound=*/false);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UModularHeroComponent::AddAdditionalInputConfig(const UModularInputConfig* InputConfig)
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

	if (const UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UModularInputComponent* ModularIC = Pawn->FindComponentByClass<UModularInputComponent>();
		if (ensureMsgf(ModularIC,
				TEXT(
					"Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UModularInputComponent or a subclass of it.")))
		{
			ModularIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
				&ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UModularHeroComponent::RemoveAdditionalInputConfig(const UModularInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UModularHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UModularHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UModularAbilitySystemComponent* ModularASC = PawnExtComp->GetModularAbilitySystemComponent())
			{
				ModularASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UModularHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UModularAbilitySystemComponent* ModularASC = PawnExtComp->GetModularAbilitySystemComponent())
		{
			ModularASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UModularHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (AController* Controller = Pawn ? Pawn->GetController() : nullptr)
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

void UModularHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
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

void UModularHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
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
		Pawn->AddControllerYawInput(Value.X * ModularHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * ModularHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

TSubclassOf<UModularCameraMode> UModularHeroComponent::DetermineCameraMode() const
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

	if (UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UModularPawnData* PawnData = PawnExtComp->GetPawnData<UModularPawnData>())
		{
			return PawnData->DefaultCameraMode.LoadSynchronous();
		}
	}

	return nullptr;
}

void UModularHeroComponent::SetAbilityCameraMode(
	TSubclassOf<UModularCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void UModularHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}
