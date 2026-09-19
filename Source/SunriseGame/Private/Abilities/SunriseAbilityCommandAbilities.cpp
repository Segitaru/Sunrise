#include "Abilities/SunriseAbilityCommandAbilities.h"

#include <AbilitySystemComponent.h>

#include "Abilities/SunriseSelectionAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"

namespace
{
	FGameplayTag MakeInputTag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}
} // namespace

USunriseAbilityCommandAbility::USunriseAbilityCommandAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void USunriseAbilityCommandAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunrisePawn* Pawn = ActorInfo ? Cast<ASunrisePawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	ASunrisePlayerController* Controller = Pawn ? Cast<ASunrisePlayerController>(Pawn->GetController()) : nullptr;
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller);
	TArray<ASunriseUnit*> Units;
	if (Pawn && Pawn->GetAbilitySystemComponent())
	{
		if (FGameplayAbilitySpec* SelectionSpec =
				Pawn->GetAbilitySystemComponent()->FindAbilitySpecFromClass(USunriseSelectionAbility::StaticClass()))
		{
			if (USunriseSelectionAbility* Selection = Cast<USunriseSelectionAbility>(SelectionSpec->Ability))
			{
				Units = Selection->GetSelectedUnits();
			}
		}
	}
	const FGameplayTag InputTag = GetCommandInputTag();

	if (Pawn && Manager && InputTag.IsValid())
	{
		FGameplayEventData Event;
		Event.EventTag = InputTag;
		for (ASunriseUnit* Unit : Units)
		{
			if (IsValid(Unit) && Unit->IsAlive() && Manager && Manager->CanControlEntity(Unit))
			{
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Unit, InputTag, Event);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

FGameplayTag USunrisePrimaryAbilityCommand::GetCommandInputTag() const
{
	return MakeInputTag(TEXT("InputTag.Ability.Primary"));
}

FGameplayTag USunriseSecondaryAbilityCommand::GetCommandInputTag() const
{
	return MakeInputTag(TEXT("InputTag.Ability.Secondary"));
}

FGameplayTag USunriseOptionalAbilityCommand::GetCommandInputTag() const
{
	return MakeInputTag(TEXT("InputTag.Ability.Optional"));
}

FGameplayTag USunriseUltimateAbilityCommand::GetCommandInputTag() const
{
	return MakeInputTag(TEXT("InputTag.Ability.Ultimate"));
}
