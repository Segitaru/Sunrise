#include "Abilities/SunriseHeroAbilities.h"

#include <Engine/DamageEvents.h>

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"
#include "Units/SunriseUnit.h"
#include "Weapons/Actors/SunriseAreaIndicator.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_HeroHealingAuraCooldown, "Cooldown.Sunrise.HeroHealingAura");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_HeroBombCooldown, "Cooldown.Sunrise.HeroBomb");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_HeroBlackHoleCooldown, "Cooldown.Sunrise.HeroBlackHole");

namespace
{
	FGameplayTag Tag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	void Damage(ASunriseUnit* Source, ASunriseUnit* Target, float Amount)
	{
		if (IsValid(Target) && Target->IsAlive() && Amount > 0.0f)
			Target->TakeDamage(Amount, FDamageEvent(), Source ? Source->GetController() : nullptr, Source);
	}
} // namespace

USunriseHeroAbilityCooldownEffect::USunriseHeroAbilityCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(10.0f);
}

USunriseHeroAbility::USunriseHeroAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool USunriseHeroAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo)
		return false;
	ASunriseUnit* Hero = nullptr;
	if (!GetHero(ActorInfo, Hero) || !Hero->HasAuthority() || !Hero->IsHero() || !Hero->IsAlive())
		return false;
	const FGameplayTag CooldownTag = GetCooldownTag();
	return !CooldownTag.IsValid() || !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
}

FGameplayTag USunriseHeroHealingAuraAbility::GetCooldownTag() const
{
	return TAG_Sunrise_HeroHealingAuraCooldown;
}
FGameplayTag USunriseHeroBombAbility::GetCooldownTag() const
{
	return TAG_Sunrise_HeroBombCooldown;
}
FGameplayTag USunriseHeroBlackHoleAbility::GetCooldownTag() const
{
	return TAG_Sunrise_HeroBlackHoleCooldown;
}

float USunriseHeroAbility::GetCooldownRemaining(const ASunriseUnit* Hero) const
{
	const UAbilitySystemComponent* ASC = IsValid(Hero) ? Hero->GetAbilitySystemComponent() : nullptr;
	const FGameplayTag CooldownTag = GetCooldownTag();
	float Remaining = 0.0f;
	if (ASC && CooldownTag.IsValid())
	{
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
		for (float Time : ASC->GetActiveEffectsTimeRemaining(Query))
		{
			Remaining = FMath::Max(Remaining, Time);
		}
	}
	return Remaining;
}

bool USunriseHeroAbility::GetHero(const FGameplayAbilityActorInfo* ActorInfo, ASunriseUnit*& OutHero) const
{
	OutHero = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	return IsValid(OutHero);
}

bool USunriseHeroAbility::StartHeroAbility(
	FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo)
{
	ASunriseUnit* Hero = nullptr;
	return GetHero(ActorInfo, Hero) && Hero->HasAuthority() && CommitAbility(Handle, ActorInfo, ActivationInfo);
}

void USunriseHeroAbility::ApplyHeroCooldown(
	const FGameplayAbilityActorInfo* ActorInfo, float Duration, const FGameplayTag& CooldownTag) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !CooldownTag.IsValid())
		return;
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	FGameplayEffectSpecHandle Spec =
		ASC->MakeOutgoingSpec(USunriseHeroAbilityCooldownEffect::StaticClass(), GetAbilityLevel(), ASC->MakeEffectContext());
	if (!Spec.IsValid())
		return;
	Spec.Data->SetDuration(FMath::Max(1.0f, Duration), true);
	Spec.Data->DynamicGrantedTags.AddTag(CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

USunriseHeroHealingAuraAbility::USunriseHeroHealingAuraAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = Tag(TEXT("InputTag.Ability.Secondary"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void USunriseHeroHealingAuraAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Hero = nullptr;
	if (!StartHeroAbility(Handle, ActorInfo, ActivationInfo) || !GetHero(ActorInfo, Hero))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AuraHero = Hero;
	ApplyHeroCooldown(ActorInfo, Cooldown, GetCooldownTag());
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FTransform SpawnTransform = FTransform(Hero->GetActorLocation());

	if (ASunriseAreaDecalIndicator* Indicator =
			Hero->GetWorld()->SpawnActor<ASunriseAreaDecalIndicator>(ASunriseAreaDecalIndicator::StaticClass(), SpawnTransform, SpawnInfo))
	{
		Indicator->InitializeIndicator(AuraRadius, AuraDuration, FLinearColor::Green);
		Indicator->AttachToActor(Hero, FAttachmentTransformRules::KeepWorldTransform);
	}

	ApplyAuraPulse();
	Hero->GetWorldTimerManager().SetTimer(AuraPulseTimer, this, &ThisClass::ApplyAuraPulse, 0.5f, true, 0.5f);
	Hero->GetWorldTimerManager().SetTimer(AuraFinishTimer, this, &ThisClass::FinishAura, AuraDuration, false);
}

void USunriseHeroHealingAuraAbility::ApplyAuraPulse()
{
	ASunriseUnit* Hero = AuraHero.Get();
	if (!IsValid(Hero) || !Hero->HasAuthority())
		return;
	for (TActorIterator<ASunriseUnit> It(Hero->GetWorld()); It; ++It)
	{
		ASunriseUnit* Unit = *It;
		if (IsValid(Unit) && Unit->IsAlive() && Unit->GetTeamId() == Hero->GetTeamId() &&
			FVector::DistSquared2D(Unit->GetActorLocation(), Hero->GetActorLocation()) <= FMath::Square(AuraRadius))
			Unit->ReceiveHealing(HealPerPulse, Hero);
	}
}

void USunriseHeroHealingAuraAbility::FinishAura()
{
	if (AuraHero.IsValid())
		AuraHero->GetWorldTimerManager().ClearTimer(AuraPulseTimer);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

USunriseHeroBombAbility::USunriseHeroBombAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = Tag(TEXT("InputTag.Ability.Optional"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void USunriseHeroBombAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Hero = nullptr;
	if (!StartHeroAbility(Handle, ActorInfo, ActivationInfo) || !GetHero(ActorInfo, Hero))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	BombHero = Hero;
	BombLocation = Hero->GetActorLocation();
	ApplyHeroCooldown(ActorInfo, Cooldown, GetCooldownTag());
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FTransform SpawnTransform = FTransform(BombLocation);

	if (ASunriseAreaDecalIndicator* Indicator =
			Hero->GetWorld()->SpawnActor<ASunriseAreaDecalIndicator>(ASunriseAreaDecalIndicator::StaticClass(), SpawnTransform, SpawnInfo))
	{
		Indicator->InitializeIndicator(ExplosionRadius, FuseDuration, FLinearColor::Red);
	}

	Hero->GetWorldTimerManager().SetTimer(FuseTimer, this, &ThisClass::ExplodeBomb, FuseDuration, false);
}

void USunriseHeroBombAbility::ExplodeBomb()
{
	ASunriseUnit* Hero = BombHero.Get();
	if (IsValid(Hero) && Hero->HasAuthority())
	{
		for (TActorIterator<ASunriseUnit> It(Hero->GetWorld()); It; ++It)
		{
			ASunriseUnit* Unit = *It;
			if (IsValid(Unit) && FVector::DistSquared2D(Unit->GetActorLocation(), BombLocation) <= FMath::Square(ExplosionRadius))
				Damage(Hero, Unit, ExplosionDamage);
		}
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform SpawnTransform = FTransform(BombLocation);

		if (ASunriseAreaIndicator* Indicator =
				Hero->GetWorld()->SpawnActor<ASunriseAreaIndicator>(ASunriseAreaIndicator::StaticClass(), SpawnTransform, SpawnInfo))
		{
			Indicator->InitializeIndicator(ExplosionRadius, 0.45f);
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

USunriseHeroBlackHoleAbility::USunriseHeroBlackHoleAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = Tag(TEXT("InputTag.Ability.Ultimate"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void USunriseHeroBlackHoleAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Hero = nullptr;
	if (!StartHeroAbility(Handle, ActorInfo, ActivationInfo) || !GetHero(ActorInfo, Hero))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	BlackHoleHero = Hero;
	BlackHoleLocation = Hero->GetActorLocation();
	ApplyHeroCooldown(ActorInfo, Cooldown, GetCooldownTag());

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FTransform SpawnTransform = FTransform(BlackHoleLocation);

	if (ASunriseAreaIndicator* Indicator =
			Hero->GetWorld()->SpawnActor<ASunriseAreaIndicator>(ASunriseAreaIndicator::StaticClass(), SpawnTransform, SpawnInfo))
	{
		Indicator->InitializeIndicator(Radius, Duration);
	}

	Hero->GetWorldTimerManager().SetTimer(BlackHolePulseTimer, this, &ThisClass::PullAndDamage, 0.25f, true, 0.0f);
	Hero->GetWorldTimerManager().SetTimer(BlackHoleFinishTimer, this, &ThisClass::FinishBlackHole, Duration, false);
}

void USunriseHeroBlackHoleAbility::PullAndDamage()
{
	ASunriseUnit* Hero = BlackHoleHero.Get();
	if (!IsValid(Hero) || !Hero->HasAuthority())
		return;
	for (TActorIterator<ASunriseUnit> It(Hero->GetWorld()); It; ++It)
	{
		ASunriseUnit* Unit = *It;
		if (!IsValid(Unit) || Unit == Hero || !Unit->IsAlive())
			continue;
		const float Distance = FVector::Dist2D(Unit->GetActorLocation(), BlackHoleLocation);
		if (Distance <= Radius && Distance > 1.0f)
		{
			const FVector NewLocation = FMath::VInterpConstantTo(Unit->GetActorLocation(), BlackHoleLocation, 0.25f, PullSpeed);
			Unit->SetActorLocation(NewLocation, true);
			Damage(Hero, Unit, DamagePerPulse * 0.25f);
		}
	}
}

void USunriseHeroBlackHoleAbility::FinishBlackHole()
{
	if (BlackHoleHero.IsValid())
	{
		BlackHoleHero->GetWorldTimerManager().ClearTimer(BlackHolePulseTimer);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
