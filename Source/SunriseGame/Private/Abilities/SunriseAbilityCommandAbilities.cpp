#include "Abilities/SunriseAbilityCommandAbilities.h"

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
	const FGameplayTag InputTag = GetCommandInputTag();

	if (Pawn && Manager && InputTag.IsValid())
	{
		FGameplayEventData Event;
		Event.EventTag = InputTag;
		for (AActor* Entity : Manager->GetControlledEntities())
		{
			if (ASunriseUnit* Unit = Cast<ASunriseUnit>(Entity); IsValid(Unit) && Unit->IsAlive() && Manager->CanControlEntity(Unit))
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
