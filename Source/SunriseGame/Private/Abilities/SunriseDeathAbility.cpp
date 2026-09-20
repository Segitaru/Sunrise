#include "Abilities/SunriseDeathAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Overload/OverloadGameMatchComponent.h"
#include "ModularGameplayTags.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"
#include "Vitality/VitalityComponent.h"

USunriseDeathAbility::USunriseDeathAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	AbilityTags.AddTag(ModularGameplayTags::Ability_Behavior_SurvivesDeath);
	FAbilityTriggerData Trigger;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	Trigger.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Sunrise.GameplayEvent.Death"));
	AbilityTriggers.Add(Trigger);
}

bool USunriseDeathAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}
	const ASunriseUnit* Unit = Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get());
	const UVitalityComponent* Vitality = UVitalityComponent::FindVitalityComponent(Unit);
	return Unit && Unit->HasAuthority() && Unit->GetHealth() <= 0.0f && Vitality && Vitality->IsHealthy();
}

void USunriseDeathAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Unit = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	UVitalityComponent* Vitality = UVitalityComponent::FindVitalityComponent(Unit);
	if (!Unit || !Unit->HasAuthority() || !Vitality || Unit->GetHealth() > 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Death is mandatory, so it has no cost/cooldown commit that could prevent it.
	Unit->GetAbilitySystemComponent()->CancelAbilities(nullptr, &AbilityTags, this);
	Vitality->StartDeath();
	Vitality->FinishDeath();
	OnDeathStarted(Unit);
}

void USunriseDeathAbility::OnDeathStarted(ASunriseUnit* Unit)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USunriseRespawnAbility::OnDeathStarted(ASunriseUnit* Unit)
{
	if (MaxRespawns >= 0 && CompletedRespawns >= MaxRespawns)
	{
		Super::OnDeathStarted(Unit);
		return;
	}
	RespawnTransform = bRespawnAtInitialSpawn ? Unit->GetInitialSpawnTransform() : Unit->GetActorTransform();
	WaitForRespawn(RespawnDelay);
}

void USunriseRespawnAbility::WaitForRespawn(float Delay)
{
	ASunriseUnit* Unit = Cast<ASunriseUnit>(GetAvatarActorFromActorInfo());
	const AGameStateBase* GameState = Unit && Unit->GetWorld() ? Unit->GetWorld()->GetGameState() : nullptr;
	if (!Unit || !GameState)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	const float WaitSeconds = FMath::Max(0.01f, Delay);
	Unit->SetRespawnReadyTime(GameState->GetServerWorldTimeSeconds() + WaitSeconds);
	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, WaitSeconds);
	Task->OnFinish.AddDynamic(this, &ThisClass::TryRespawn);
	Task->ReadyForActivation();
}

void USunriseRespawnAbility::TryRespawn()
{
	ASunriseUnit* Unit = Cast<ASunriseUnit>(GetAvatarActorFromActorInfo());
	const USunriseUnitManagerComponent* Registry = USunriseUnitManagerComponent::Find(Unit);
	const AController* OwnerController = Registry ? Registry->GetPlayerForHero(Unit) : nullptr;
	const APlayerState* PlayerState = OwnerController ? OwnerController->PlayerState : nullptr;
	const UOverloadGameMatchComponent* Match = UOverloadGameMatchComponent::Find(Unit);
	if (!Unit || !Unit->HasAuthority() || Unit->IsAlive() ||
		(Unit->IsHero() &&
			(!IsValid(OwnerController) || !IsValid(PlayerState) || PlayerState->IsInactive() || PlayerState->IsOnlyASpectator())) ||
		(Match && Match->GetMatchResult() != ESunriseMatchResult::InProgress))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	if (!Unit->RestoreAfterDeath(RespawnTransform))
	{
		// A blocked destination does not consume a life.
		WaitForRespawn(0.5f);
		return;
	}
	++CompletedRespawns;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USunriseRespawnAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ASunriseUnit* Unit = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Unit->SetRespawnReadyTime(-1.0f);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
