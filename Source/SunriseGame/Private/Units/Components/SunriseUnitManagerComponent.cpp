// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Components/SunriseUnitManagerComponent.h"

#include "Components/CapsuleComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Units/SunriseUnit.h"

USunriseUnitManagerComponent::USunriseUnitManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	UnitClass = ASunriseUnit::StaticClass();
	HeroClass = ASunriseUnit::StaticClass();
}

USunriseUnitManagerComponent* USunriseUnitManagerComponent::Find(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<USunriseUnitManagerComponent>() : nullptr;
}

void USunriseUnitManagerComponent::RegisterUnit(ASunriseUnit* Unit)
{
	if (IsValid(Unit))
	{
		Units.AddUnique(Unit);
	}
}

void USunriseUnitManagerComponent::NotifyUnitDied(ASunriseUnit* Unit)
{
	if (Unit)
	{
		if (AController* Controller = Cast<AController>(Unit->GetControllingAgent().GetObject()))
		{
			if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
			{
				Manager->UnregisterControlledEntity(Unit);
			}
		}
		if (Unit->IsHero())
		{
			ScheduleHeroRespawn(Unit);
		}
	}
	OnArmyCountChanged.Broadcast(GetFriendlyAlive(), GetEnemyAlive());
	OnUnitDied.Broadcast(Unit);
}

ASunriseUnit* USunriseUnitManagerComponent::SpawnHeroForTeam(
	int32 TeamId, const FTransform& SpawnTransform, TScriptInterface<IIControllableEntity> ControllingAgent)
{
	const FSunriseHeroRespawnData* ExistingData = HeroRespawnData.Find(TeamId);
	TSubclassOf<ASunriseUnit> ClassToSpawn = ExistingData && ExistingData->HeroClass ? ExistingData->HeroClass : HeroClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UnitClass;
	}
	if (!ClassToSpawn || !GetWorld())
	{
		return nullptr;
	}

	FTransform ActualTransform = SpawnTransform;
	if (const ASunriseUnit* Defaults = ClassToSpawn->GetDefaultObject<ASunriseUnit>())
	{
		if (const UCapsuleComponent* Capsule = Defaults->GetCapsuleComponent())
		{
			ActualTransform.AddToTranslation(FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() + 2.0f));
		}
	}
	ASunriseUnit* Hero = GetWorld()->SpawnActorDeferred<ASunriseUnit>(
		ClassToSpawn, ActualTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Hero)
	{
		return nullptr;
	}
	Hero->SetTeamId(TeamId);
	Hero->SetUnitRole(HeroRole, true);
	Hero->ConfigureControl(ESunriseUnitKind::Hero, ControllingAgent);
	UGameplayStatics::FinishSpawningActor(Hero, ActualTransform);
	RegisterUnit(Hero);

	FSunriseHeroRespawnData& Data = HeroRespawnData.FindOrAdd(TeamId);
	Data.HeroClass = ClassToSpawn;
	Data.SpawnTransform = SpawnTransform;
	Data.Role = HeroRole;
	Data.TeamId = TeamId;
	Data.ControllingAgent = ControllingAgent;
	Data.RespawnDelay = Hero->GetHeroRespawnDelay();
	if (AController* Controller = Cast<AController>(ControllingAgent.GetObject()))
	{
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
		{
			Manager->RegisterControlledEntity(Hero);
		}
	}
	return Hero;
}

int32 USunriseUnitManagerComponent::GetAliveUnitCountForTeam(int32 TeamId) const
{
	int32 Count = 0;
	for (const ASunriseUnit* Unit : Units)
	{
		Count += IsValid(Unit) && Unit->IsAlive() && Unit->GetTeamId() == TeamId ? 1 : 0;
	}
	return Count;
}

int32 USunriseUnitManagerComponent::GetFriendlyAlive() const
{
	return GetAliveUnitCountForTeam(static_cast<int32>(ESunriseTeam::Friendly));
}

int32 USunriseUnitManagerComponent::GetEnemyAlive() const
{
	return GetAliveUnitCountForTeam(static_cast<int32>(ESunriseTeam::Enemy));
}

ASunriseUnit* USunriseUnitManagerComponent::GetLivingHeroForTeam(int32 TeamId) const
{
	for (ASunriseUnit* Unit : Units)
	{
		if (IsValid(Unit) && Unit->IsHero() && Unit->IsAlive() && Unit->GetTeamId() == TeamId)
		{
			return Unit;
		}
	}
	return nullptr;
}

float USunriseUnitManagerComponent::GetHeroRespawnSeconds(int32 TeamId) const
{
	const FTimerHandle* Handle = HeroRespawnTimers.Find(TeamId);
	return Handle && Handle->IsValid() && GetWorld() ? FMath::Max(0.0f, GetWorld()->GetTimerManager().GetTimerRemaining(*Handle)) : -1.0f;
}

void USunriseUnitManagerComponent::ScheduleHeroRespawn(ASunriseUnit* Hero)
{
	FSunriseHeroRespawnData* Data = Hero ? HeroRespawnData.Find(Hero->GetTeamId()) : nullptr;
	if (!Data || !GetWorld())
	{
		return;
	}
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &ThisClass::RespawnHeroForTeam, Hero->GetTeamId());
	GetWorld()->GetTimerManager().SetTimer(HeroRespawnTimers.FindOrAdd(Hero->GetTeamId()), Delegate, Data->RespawnDelay, false);
}

void USunriseUnitManagerComponent::RespawnHeroForTeam(int32 TeamId)
{
	if (const FSunriseHeroRespawnData* Data = HeroRespawnData.Find(TeamId))
	{
		SpawnHeroForTeam(TeamId, Data->SpawnTransform, Data->ControllingAgent);
	}
}
