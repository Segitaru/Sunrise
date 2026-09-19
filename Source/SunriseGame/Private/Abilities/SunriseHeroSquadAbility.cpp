#include "Abilities/SunriseHeroSquadAbility.h"

#include "AbilitySystemComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "GameModes/Spawning/PawnFormationTypes.h"
#include "ModularPawnData.h"
#include "NativeGameplayTags.h"
#include "Units/Components/SunriseUnitManagerComponent.h"
#include "Units/SunriseUnit.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Sunrise_HeroSquadCooldown, "Cooldown.Sunrise.HeroSquad");

USunriseHeroSquadCooldownEffect::USunriseHeroSquadCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(30.0f);
}

float USunriseHeroSquadAbility::GetCooldownRemaining(const ASunriseUnit* Hero)
{
	const UAbilitySystemComponent* ASC = Hero ? Hero->GetAbilitySystemComponent() : nullptr;
	float Remaining = 0.0f;
	if (ASC)
	{
		const FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(TAG_Sunrise_HeroSquadCooldown));
		for (float Time : ASC->GetActiveEffectsTimeRemaining(Query))
		{
			Remaining = FMath::Max(Remaining, Time);
		}
	}
	return Remaining;
}

USunriseHeroSquadAbility::USunriseHeroSquadAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool USunriseHeroSquadAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo ||
		!ActorInfo->AvatarActor.IsValid() || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}
	const ASunriseUnit* Hero = Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get());
	return IsValid(Hero) && Hero->IsHero() && Hero->IsAlive() && !SquadFormations.IsEmpty() && Hero->GetWorld() &&
		   !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(TAG_Sunrise_HeroSquadCooldown);
}
void USunriseHeroSquadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Hero = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Hero || !Hero->HasAuthority() || !CommitAbility(Handle, ActorInfo, ActivationInfo) || !SpawnSquad(Hero))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	FGameplayEffectSpecHandle CooldownSpec =
		ASC->MakeOutgoingSpec(USunriseHeroSquadCooldownEffect::StaticClass(), GetAbilityLevel(), ASC->MakeEffectContext());
	if (CooldownSpec.IsValid())
	{
		CooldownSpec.Data->SetDuration(FMath::Max(1.0f, Cooldown), true);
		CooldownSpec.Data->DynamicGrantedTags.AddTag(TAG_Sunrise_HeroSquadCooldown);
		ASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
bool USunriseHeroSquadAbility::ActivateForHero(ASunriseUnit* Hero, TSubclassOf<USunriseHeroSquadAbility> AbilityClass)
{
	if (!IsValid(Hero) || !AbilityClass || !Hero->IsHero() || !Hero->IsAlive())
	{
		return false;
	}
	UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}
	if (Hero->HasAuthority() && !ASC->FindAbilitySpecFromClass(AbilityClass))
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
	return ASC->TryActivateAbilityByClass(AbilityClass);
}

bool USunriseHeroSquadAbility::SpawnSquad(ASunriseUnit* Hero)
{
	UWorld* World = Hero ? Hero->GetWorld() : nullptr;
	if (!World || SquadFormations.IsEmpty())
	{
		return false;
	}
	for (TWeakObjectPtr<ASunriseUnit>& Existing : SpawnedUnits)
	{
		if (Existing.IsValid())
		{
			Existing->Destroy();
		}
	}
	SpawnedUnits.Reset();
	for (int32 Index = 0; Index < SquadFormations.Num(); ++Index)
	{
		const FPawnFormation Definition = SquadFormations[Index];
		UModularPawnData* const LoadedData = Definition.Definition.LoadSynchronous();
		if (!LoadedData)
		{
			continue;
		}
		const TSubclassOf<APawn> UnitClass = LoadedData->PawnClass.LoadSynchronous();
		if (!UnitClass || Definition.Definition.LoadSynchronous()->Specification.HasTag(SunrisePawnTags::Kind_Hero))
		{
			continue;
		}
		const float Angle = Index * UE_PI;
		const FVector Location = Hero->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FormationSpacing;
		ASunriseUnit* Unit = USunriseUnitManagerComponent::SpawnUnit(LoadedData, FTransform(Hero->GetActorRotation(), Location), Hero);
		if (!Unit)
		{
			continue;
		}
		Unit->SetGenericTeamId(Hero->GetGenericTeamId());
		const bool bPlayerSummon = Hero->GetControllingAgent().GetObject() != nullptr;
		Unit->ConfigureControl(Hero->GetControllingAgent());
		Unit->SpawnDefaultController();
		if (UControllableComponent* Controllable = UControllableComponent::FindControllableComponent(Unit))
		{
			Controllable->SetEntityDefinition(LoadedData);
		}
		if (AController* Controller = Cast<AController>(Hero->GetControllingAgent().GetObject()))
		{
			if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
			{
				Manager->RegisterControlledEntity(Unit);
			}
		}
		if (!bPlayerSummon)
		{
			if (UOverloadInteractorComponent* Interactor = NewObject<UOverloadInteractorComponent>(Unit, TEXT("OverloadInteractor")))
			{
				Interactor->RegisterComponent();
				Interactor->InitializeForUnit();
			}
			for (TActorIterator<AOverloadLaneSpline> It(World); It; ++It)
			{
				AOverloadLaneSpline* Candidate = *It;
				if (Candidate && (Candidate->GetSourceTeamId() == Hero->GetTeamId() || Candidate->GetTargetTeamId() == Hero->GetTeamId()))
				{
					UOverloadLaneFollowerComponent* Follower =
						NewObject<UOverloadLaneFollowerComponent>(Unit, TEXT("OverloadLaneFollower"));
					Follower->RegisterComponent();
					Follower->Initialize(Candidate);
					break;
				}
			}
		}
		SpawnedUnits.Add(Unit);
	}
	return !SpawnedUnits.IsEmpty();
}
