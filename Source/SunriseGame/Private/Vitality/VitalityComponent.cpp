#include "Vitality/VitalityComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Vitality/Attributes/SunriseHealthSet.h"
#include "Weapons/Effects/SunriseWeaponEffects.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_Status_Death_Dying, "Sunrise.Status.Death.Dying");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_Status_Death_Dead, "Sunrise.Status.Death.Dead");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_GameplayEvent_Death, "Sunrise.GameplayEvent.Death");

UVitalityComponent::UVitalityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVitalityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UVitalityComponent, VitalityState);
}

void UVitalityComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	Super::OnUnregister();
}

void UVitalityComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InASC)
{
	if (AbilitySystemComponent == InASC)
	{
		return;
	}
	UninitializeFromAbilitySystem();
	AbilitySystemComponent = InASC;
	HealthSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<USunriseHealthSet>() : nullptr;
	if (!HealthSet)
	{
		UE_LOG(LogTemp, Error, TEXT("VitalityComponent on %s requires USunriseHealthSet"), *GetNameSafe(GetOwner()));
		AbilitySystemComponent = nullptr;
		{
			return;
		}
	}
	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);
	ClearGameplayTags();
	OnAttributeChanged.Broadcast(this, USunriseHealthSet::GetMaxHealthAttribute(), GetMaxHealth(), GetMaxHealth(), nullptr);
	OnAttributeChanged.Broadcast(this, USunriseHealthSet::GetHealthAttribute(), GetHealth(), GetHealth(), nullptr);
}
void UVitalityComponent::UninitializeFromAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}
	ClearGameplayTags();
	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}
float UVitalityComponent::GetHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

float UVitalityComponent::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : 0.0f;
}

float UVitalityComponent::GetHealthNormalized() const
{
	return GetMaxHealth() > 0.0f ? FMath::Clamp(GetHealth() / GetMaxHealth(), 0.0f, 1.0f) : 0.0f;
}
void UVitalityComponent::StartDeath()
{
	if (GetOwner() && GetOwner()->HasAuthority() && VitalityState == EVitalityState::Healthy)
	{
		SetVitalityState(EVitalityState::Dying);
	}
}

void UVitalityComponent::FinishDeath()
{
	if (GetOwner() && GetOwner()->HasAuthority() && VitalityState == EVitalityState::Dying)
	{
		SetVitalityState(EVitalityState::Dead);
	}
}

void UVitalityComponent::RestoreVitality()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilitySystemComponent || !HealthSet)
	{
		return;
	}
	AbilitySystemComponent->SetNumericAttributeBase(USunriseHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());
	SetVitalityState(EVitalityState::Healthy);
}

void UVitalityComponent::DamageSelfDestruct()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilitySystemComponent || !IsHealthy())
	{
		return;
	}
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	FGameplayEffectSpecHandle Handle = AbilitySystemComponent->MakeOutgoingSpec(USunriseDamageEffect::StaticClass(), 1.0f, Context);
	if (FGameplayEffectSpec* Spec = Handle.Data.Get())
	{
		Spec->SetSetByCallerMagnitude(USunriseDamageEffect::GetMagnitudeDataName(), -GetMaxHealth());
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}

void UVitalityComponent::SetVitalityState(EVitalityState NewState)
{
	if (VitalityState == NewState)
	{
		return;
	}
	const EVitalityState OldState = VitalityState;
	VitalityState = NewState;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_Sunrise_Status_Death_Dying, NewState == EVitalityState::Dying ? 1 : 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_Sunrise_Status_Death_Dead, NewState == EVitalityState::Dead ? 1 : 0);
	}
	OnVitalityStateChanged.Broadcast(GetOwner(), OldState, NewState);
	if (GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
}
void UVitalityComponent::ClearGameplayTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->SetLooseGameplayTagCount(TAG_Sunrise_Status_Death_Dying, 0);
	AbilitySystemComponent->SetLooseGameplayTagCount(TAG_Sunrise_Status_Death_Dead, 0);
}
void UVitalityComponent::HandleHealthChanged(AActor* Instigator, AActor*, const FGameplayEffectSpec*, float, float OldValue, float NewValue)
{
	OnAttributeChanged.Broadcast(this, USunriseHealthSet::GetHealthAttribute(), OldValue, NewValue, Instigator);
}

void UVitalityComponent::HandleMaxHealthChanged(
	AActor* Instigator, AActor*, const FGameplayEffectSpec*, float, float OldValue, float NewValue)
{
	OnAttributeChanged.Broadcast(this, USunriseHealthSet::GetMaxHealthAttribute(), OldValue, NewValue, Instigator);
}
void UVitalityComponent::HandleOutOfHealth(AActor* Instigator, AActor*, const FGameplayEffectSpec* Spec, float Magnitude, float, float)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (AbilitySystemComponent)
	{
		FGameplayEventData Payload;
		Payload.EventTag = TAG_Sunrise_GameplayEvent_Death;
		Payload.Instigator = Instigator;
		Payload.Target = GetOwner();
		Payload.EventMagnitude = Magnitude;
		if (Spec)
		{
			Payload.ContextHandle = Spec->GetEffectContext();
		}
		AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
	StartDeath();
}
void UVitalityComponent::OnRep_VitalityState(EVitalityState OldState)
{
	OnVitalityStateChanged.Broadcast(GetOwner(), OldState, VitalityState);
}
