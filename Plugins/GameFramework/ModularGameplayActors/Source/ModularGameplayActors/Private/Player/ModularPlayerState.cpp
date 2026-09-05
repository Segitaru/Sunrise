// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModularPlayerState.h"

#include <Net/UnrealNetwork.h>

#include "AbilitySystem/ModularAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/ModularPawnExtensionComponent.h"
#include "Components/PlayerStateComponent.h"
#include "ModularAbilitySet.h"
#include "ModularLogChannels.h"
#include "ModularPawnData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularPlayerState)

AModularPlayerState::AModularPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UModularAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	if (GetNetMode() == ENetMode::NM_Standalone)
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	}
	else
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}
}

UAbilitySystemComponent* AModularPlayerState::GetAbilitySystemComponent() const
{
	return GetModularAbilitySystemComponent();
}

void AModularPlayerState::SetPawnData(const UModularPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogModularGameplayActors, Error,
			TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData),
			*GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	for (const TSoftObjectPtr<UModularAbilitySet> AbilitySet : PawnData->AbilitySets)
	{
		if (const auto* const LoadedAbilitySet = AbilitySet.LoadSynchronous())
		{
			LoadedAbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_ModularAbilityReady);

	ForceNetUpdate();
}

void AModularPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AModularPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AModularPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AModularPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void AModularPlayerState::CallOrRegister_OnPawnDataReady(FOnPawnDataReady::FDelegate&& Delegate)
{
	if (PawnData.Get())
	{
		Delegate.Execute();
	}
	else
	{
		OnPawnDataReady.Add(MoveTemp(Delegate));
	}
}

void AModularPlayerState::OnRep_PawnData()
{
}

void AModularPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void AModularPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AModularPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (UModularPawnExtensionComponent* PawnExtComp = UModularPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckDefaultInitialization();
	}
}

void AModularPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, PlayerAccountId);
}

void AModularPlayerState::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();
}

void AModularPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
}

void AModularPlayerState::Reset()
{
	Super::Reset();

	TArray<UPlayerStateComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UPlayerStateComponent* Component : ModularComponents)
	{
		Component->Reset();
	}
}

const FString& AModularPlayerState::GetAccountId() const
{
	return PlayerAccountId;
}

void AModularPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	TInlineComponentArray<UPlayerStateComponent*> PlayerStateComponents;
	GetComponents(PlayerStateComponents);
	for (UPlayerStateComponent* SourcePSComp : PlayerStateComponents)
	{
		if (UPlayerStateComponent* TargetComp = Cast<UPlayerStateComponent>(
				static_cast<UObject*>(FindObjectWithOuter(PlayerState, SourcePSComp->GetClass(), SourcePSComp->GetFName()))))
		{
			SourcePSComp->CopyProperties(TargetComp);
		}
	}
}
