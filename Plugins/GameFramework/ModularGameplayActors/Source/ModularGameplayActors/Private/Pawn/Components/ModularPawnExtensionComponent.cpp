// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pawn/Components/ModularPawnExtensionComponent.h"

#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/PawnCosmeticCreatorComponent.h"
#include "Fragments/ModularPawnDataFragment.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "ModularCharacter.h"
#include "ModularGameplayTags.h"
#include "ModularLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Pawn/ModularPawnData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularPawnExtensionComponent)

DEFINE_LOG_CATEGORY_STATIC(LogModularPawnExtensionComponent, All, All)

class FLifetimeProperty;
class UActorComponent;

const FName UModularPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UModularPawnExtensionComponent::UModularPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	PawnData = nullptr;
	AbilitySystemComponent = nullptr;
}

void UModularPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UModularPawnExtensionComponent, PawnData, COND_None, REPNOTIFY_Always);
}

void UModularPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf(
		(Pawn != nullptr), TEXT("ModularPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(UModularPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one ModularPawnExtensionComponent should exist on [%s]."),
		*GetNameSafe(GetOwner()));

	// Register with the init state system early, this will only work if this is a game world
	RegisterInitStateFeature();
}

void UModularPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for changes to all features
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

	// Notifies state manager that we have spawned, then try rest of default initialization
	ensure(TryToChangeInitState(ModularGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UModularPawnExtensionComponent::SetPawnData(const UModularPawnData* InPawnData)
{
	check(InPawnData);

	APawn* Pawn = GetPawnChecked<APawn>();

	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogModularGameplayActors, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."),
			*GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	Pawn->ForceNetUpdate();

	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::OnRep_PawnData()
{
	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::TryAddPawnPartComponent()
{
	if (!PawnData)
	{
		UE_LOG(LogModularPawnExtensionComponent, Display, TEXT("[Cosmetic]: Pawn definition doesn't existed yet"));
		return;
	}

	auto* const PawnPartComponent = GetOwner()->FindComponentByClass<UPawnCosmeticCreatorComponent>();

	if (!PawnPartComponent)
	{
		UE_LOG(LogModularPawnExtensionComponent, Error, TEXT("[Cosmetic]: Pawn Not have cosmetic component"));
		return;
	}

	PawnPartComponent->RemoveAllCharacterParts();

	for (const auto& PawnPart : PawnData->PawnMeshes)
	{
		PawnPartComponent->AddCosmeticPart(PawnPart);
	}

	if (AModularCharacter* const AsCharacter = Cast<AModularCharacter>(GetOwner()); IsValid(AsCharacter))
	{
		AsCharacter->OnCosmeticPartAddedExternal();
	}
}

void UModularPawnExtensionComponent::TryAddPawnAbilities()
{
	if (!PawnData)
	{
		UE_LOG(LogModularPawnExtensionComponent, Display, TEXT("[Abilities]: Pawn definition doesn't existed yet"));
		return;
	}

	if (!IsValid(GetModularAbilitySystemComponent()))
	{
		UE_LOG(LogModularPawnExtensionComponent, Display, TEXT("[Abilities]: Not valid ASC component"));
		return;
	}
	GetModularAbilitySystemComponent()->CancelAbilities();
	RemoveGrantedAbility();

	AddGrantedAbilities();
}

void UModularPawnExtensionComponent::TryAddPawnComponents()
{
	if (!PawnData)
	{
		UE_LOG(LogModularPawnExtensionComponent, Display, TEXT("[Components]: Pawn definition doesn't existed yet"));
		return;
	}

	AActor* const Pawn = GetOwner();

	CurrentGrantedComponents.TakeFromActor(Pawn);

	for (const auto& ComponentSet : PawnData->ComponentsSets)
	{
		ComponentSet.GiveComponentsToActor(Pawn, &CurrentGrantedComponents);
	}
}

void UModularPawnExtensionComponent::TryActivateFragments()
{
	if (!PawnData)
	{
		UE_LOG(LogModularPawnExtensionComponent, Display, TEXT("[Components]: Pawn definition doesn't existed yet"));
		return;
	}

	APawn* const Pawn = GetPawn<APawn>();
	for (const auto& Fragment : PawnData->Fragments)
	{
		Fragment->Activate(Pawn);
	}
}

void UModularPawnExtensionComponent::RemoveGrantedAbility()
{
	CurrentGrantedAbility.TakeFromAbilitySystem(GetModularAbilitySystemComponent());
}

void UModularPawnExtensionComponent::AddGrantedAbilities()
{
	for (const auto& AbilitySet : PawnData->AbilitySets)
	{
		if (const auto LoadedAbilitySet = AbilitySet.LoadSynchronous())
		{
			LoadedAbilitySet->GiveToAbilitySystem(GetModularAbilitySystemComponent(), &CurrentGrantedAbility, nullptr);
		}
	}

	const FName NAME_AbilityReady("AbilitiesReady");

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(GetOwner(), NAME_AbilityReady);
}

void UModularPawnExtensionComponent::InitializeAbilitySystem(UModularAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC);
	check(InOwnerActor);

	if (AbilitySystemComponent == InASC)
	{
		// The ability system component hasn't changed.
		return;
	}

	if (AbilitySystemComponent)
	{
		// Clean up the old ability system component.
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InASC->GetAvatarActor();

	UE_LOG(LogModularGameplayActors, Verbose, TEXT("Setting up ASC [%s] on pawn [%s] owner [%s], existing [%s] "), *GetNameSafe(InASC),
		*GetNameSafe(Pawn), *GetNameSafe(InOwnerActor), *GetNameSafe(ExistingAvatar));

	if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn))
	{
		UE_LOG(LogModularGameplayActors, Log, TEXT("Existing avatar (authority=%d)"), ExistingAvatar->HasAuthority() ? 1 : 0);

		// There is already a pawn acting as the ASC's avatar, so we need to kick it out
		// This can happen on clients if they're lagged: their new pawn is spawned + possessed before the dead one is removed
		ensure(!ExistingAvatar->HasAuthority());

		if (UModularPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtensionComponent->UninitializeAbilitySystem();
		}
	}

	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	if (ensure(PawnData))
	{
		InASC->SetTagRelationshipMapping(PawnData->TagRelationshipMapping);
	}

	OnAbilitySystemInitialized.Broadcast();
}

void UModularPawnExtensionComponent::UpdateAbilitySystemOwner(AActor* InOwnerActor)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->SetOwnerActor(InOwnerActor);
}

void UModularPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		FGameplayTagContainer AbilityTypesToIgnore;
		AbilityTypesToIgnore.AddTag(ModularGameplayTags::Ability_Behavior_SurvivesDeath);

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// If the ASC doesn't have a valid owner, we need to clear *all* actor info, not just the avatar pairing
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void UModularPawnExtensionComponent::HandleControllerChanged()
{
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()) &&
		AbilitySystemComponent->AbilityActorInfo)
	{
		ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			UninitializeAbilitySystem();
		}
		else
		{
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void UModularPawnExtensionComponent::CheckDefaultInitialization()
{
	// Before checking our progress, try progressing any other features we might depend on
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = {ModularGameplayTags::InitState_Spawned, ModularGameplayTags::InitState_DataAvailable,
		ModularGameplayTags::InitState_DataInitialized, ModularGameplayTags::InitState_GameplayReady};

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

bool UModularPawnExtensionComponent::CanChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
	if (!CurrentState.IsValid() && DesiredState == ModularGameplayTags::InitState_Spawned)
	{
		// As long as we are on a valid pawn, we count as spawned
		if (Pawn)
		{
			return true;
		}

		return false;
	}

	if (CurrentState == ModularGameplayTags::InitState_Spawned && DesiredState == ModularGameplayTags::InitState_DataAvailable)
	{
		// Pawn data is required.
		if (!PawnData)
		{
			return false;
		}

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			// Check for being possessed by a controller.
			if (!GetController<AController>())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == ModularGameplayTags::InitState_DataAvailable && DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		// Transition to initialize if all features have their data available
		return Manager->HaveAllFeaturesReachedInitState(Pawn, ModularGameplayTags::InitState_DataAvailable);
	}
	else if (CurrentState == ModularGameplayTags::InitState_DataInitialized && DesiredState == ModularGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UModularPawnExtensionComponent::HandleChangeInitState(
	UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == ModularGameplayTags::InitState_DataAvailable)
	{
		AActor* const Pawn = GetOwner();
		InitializeAbilitySystem(Pawn->FindComponentByClass<UModularAbilitySystemComponent>(), Pawn);
	}
	else if (DesiredState == ModularGameplayTags::InitState_DataInitialized)
	{
		TryAddPawnPartComponent();
		TryAddPawnAbilities();
		TryAddPawnComponents();
		TryActivateFragments();
	}
}

void UModularPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// If another feature is now in DataAvailable, see if we should transition to DataInitialized
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == ModularGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

void UModularPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void UModularPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}
