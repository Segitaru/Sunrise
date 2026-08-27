#include "Abilities/SunriseHeroSquadAbility.h"

#include "AbilitySystemComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "ControllableEntities/Data/ControllableEntityDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameModes/Overload/Actors/OverloadLaneSpline.h"
#include "GameModes/Overload/Components/OverloadInteractorComponent.h"
#include "GameModes/Overload/Components/OverloadLaneFollowerComponent.h"
#include "Units/SunriseUnit.h"

USunriseHeroSquadAbility::USunriseHeroSquadAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool USunriseHeroSquadAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) || !ActorInfo ||
		!ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}
	const ASunriseUnit* Hero = Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get());
	return Hero && Hero->IsHero() && Hero->IsAlive() && !SquadDefinitions.IsEmpty() && Hero->GetWorld() &&
		   Hero->GetWorld()->GetTimeSeconds() >= NextActivationTime;
}

void USunriseHeroSquadAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunriseUnit* Hero = ActorInfo ? Cast<ASunriseUnit>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Hero || !Hero->HasAuthority() || !SpawnSquad(Hero))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	NextActivationTime = Hero->GetWorld()->GetTimeSeconds() + FMath::Max(1.0f, Cooldown);
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
	if (!World || SquadDefinitions.IsEmpty())
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
	for (int32 Index = 0; Index < SquadDefinitions.Num(); ++Index)
	{
		UControllableEntityDefinition* Definition = SquadDefinitions[Index].LoadSynchronous();
		TSubclassOf<ASunriseUnit> UnitClass = Definition ? Definition->UnitClass.LoadSynchronous() : nullptr;
		if (!UnitClass)
		{
			continue;
		}
		const float Angle = Index * UE_PI;
		const FVector Location = Hero->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FormationSpacing;
		FActorSpawnParameters Params;
		Params.Owner = Hero->GetOwner();
		Params.Instigator = Hero;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ASunriseUnit* Unit = World->SpawnActor<ASunriseUnit>(UnitClass, Location, Hero->GetActorRotation(), Params);
		if (!Unit)
		{
			continue;
		}
		Unit->SetTeamId(Hero->GetTeamId());
		const bool bPlayerSummon = Hero->GetControllingAgent().GetObject() != nullptr;
		Unit->ConfigureControl(bPlayerSummon ? ESunriseUnitKind::Summoned : ESunriseUnitKind::Creep, Hero->GetControllingAgent());
		Unit->SpawnDefaultController();
		if (UControllableComponent* Controllable = UControllableComponent::FindControllableComponent(Unit))
		{
			Controllable->SetEntityDefinition(Definition);
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
