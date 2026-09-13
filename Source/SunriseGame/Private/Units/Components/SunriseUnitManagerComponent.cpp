#include "Units/Components/SunriseUnitManagerComponent.h"

#include "Components/CapsuleComponent.h"
#include "ControllableEntities/ControllableComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/Components/ModularPawnExtensionComponent.h"
#include "Pawn/ModularPawnData.h"
#include "Rosters/Player/Components/PlayerPawnManager.h"
#include "Units/SunriseUnit.h"

DEFINE_LOG_CATEGORY_STATIC(LogSunriseHeroSpawn, Log, All);

USunriseUnitManagerComponent::USunriseUnitManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

USunriseUnitManagerComponent* USunriseUnitManagerComponent::Find(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<USunriseUnitManagerComponent>() : nullptr;
}

ASunriseUnit* USunriseUnitManagerComponent::SpawnUnit(const UModularPawnData* PawnData, const FTransform& Transform, AActor* Owner)
{
	if (!IsValid(Owner) || !Owner->HasAuthority() || !Owner->GetWorld() || !IsValid(PawnData))
	{
		return nullptr;
	}
	AController* Controller = Cast<AController>(Owner);
	APlayerState* PlayerState = Controller ? Controller->PlayerState : nullptr;
	const bool bHero = PawnData->Specification.HasTag(SunrisePawnTags::Kind_Hero);
	USunriseUnitManagerComponent* Registry = bHero ? Find(Owner) : nullptr;
	if (bHero)
	{
		const UPlayerPawnManager* Selection = IsValid(PlayerState) ? PlayerState->FindComponentByClass<UPlayerPawnManager>() : nullptr;
		if (!Selection || Selection->GetSelectedPawnDefinition() != PawnData || PlayerState->IsInactive() ||
			PlayerState->IsOnlyASpectator() || !Registry || Registry->GetHeroForPlayer(Controller))
		{
			return nullptr;
		}
	}
	UClass* PawnClass = PawnData->PawnClass.LoadSynchronous();
	if (!PawnClass || !PawnClass->IsChildOf(ASunriseUnit::StaticClass()) || PawnClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	ASunriseUnit* Unit = Owner->GetWorld()->SpawnActorDeferred<ASunriseUnit>(PawnClass, Transform, Owner,
		Controller ? Controller->GetPawn() : Cast<APawn>(Owner), ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
	if (!Unit)
	{
		return nullptr;
	}
	if (bHero)
	{
		for (auto It = Registry->PlayerHeroes.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || !It.Value().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		// Reserve before initialization can invoke gameplay callbacks or AI possession.
		Registry->PlayerHeroes.Add(Controller, Unit);
	}
	Unit->PawnExtensionComponent->SetPawnData(PawnData);
	const IModularTeamAgentInterface* TeamAgent =
		PlayerState ? Cast<IModularTeamAgentInterface>(PlayerState) : Cast<IModularTeamAgentInterface>(Owner);
	if (TeamAgent)
	{
		Unit->SetGenericTeamId(TeamAgent->GetGenericTeamId());
	}
	if (UControllableComponent* Component = UControllableComponent::FindControllableComponent(Unit))
	{
		Component->SetEntityDefinition(const_cast<UModularPawnData*>(PawnData));
	}
	TScriptInterface<IIControllableEntity> Agent;
	if (Controller && Cast<IIControllableEntity>(Controller))
	{
		Agent.SetObject(Controller);
		Agent.SetInterface(Cast<IIControllableEntity>(Controller));
	}
	Unit->ConfigureControl(Agent);
	UGameplayStatics::FinishSpawningActor(Unit, Transform);
	if (!IsValid(Unit))
	{
		if (bHero)
		{
			Registry->PlayerHeroes.Remove(Controller);
		}
		return nullptr;
	}
	if (Controller)
	{
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
		{
			Manager->RegisterControlledEntity(Unit);
		}
	}
	return Unit;
}

ASunriseUnit* USunriseUnitManagerComponent::SpawnHeroForPlayer(AController* Controller, const FTransform& GroundTransform)
{
	if (!HasAuthority() || !IsValid(Controller) || Controller->GetWorld() != GetWorld() || !IsValid(Controller->PlayerState))
	{
		return nullptr;
	}
	if (ASunriseUnit* Existing = GetHeroForPlayer(Controller))
	{
		return Existing;
	}
	const UPlayerPawnManager* Selection = Controller->PlayerState->FindComponentByClass<UPlayerPawnManager>();
	const UModularPawnData* Data = Selection ? Selection->GetSelectedPawnDefinition() : nullptr;
	if (!Data || !Data->Specification.HasTag(SunrisePawnTags::Kind_Hero))
	{
		UE_LOG(LogSunriseHeroSpawn, Verbose,
			TEXT("Waiting: Controller=%s PlayerState=%s PlayerPawnManager=%s SelectedPawnData=%s HeroTag=%d"), *GetNameSafe(Controller),
			*GetNameSafe(Controller->PlayerState), *GetNameSafe(Selection), *GetNameSafe(Data),
			Data && Data->Specification.HasTag(SunrisePawnTags::Kind_Hero));
		return nullptr;
	}
	UClass* PawnClass = Data->PawnClass.LoadSynchronous();
	const ASunriseUnit* Defaults =
		PawnClass && PawnClass->IsChildOf(ASunriseUnit::StaticClass()) ? PawnClass->GetDefaultObject<ASunriseUnit>() : nullptr;
	if (!Defaults)
	{
		UE_LOG(LogSunriseHeroSpawn, Verbose, TEXT("Invalid hero PawnClass: PlayerState=%s PawnData=%s PawnClass=%s"),
			*GetNameSafe(Controller->PlayerState), *GetNameSafe(Data), *GetNameSafe(PawnClass));
		return nullptr;
	}
	FTransform Transform = GroundTransform;
	Transform.AddToTranslation(FVector(0.0f, 0.0f, Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f));
	ASunriseUnit* Hero = SpawnUnit(Data, Transform, Controller);
	if (Hero)
	{
		UE_LOG(LogSunriseHeroSpawn, Log, TEXT("Hero spawned: PlayerState=%s PawnData=%s Hero=%s Team=%d"),
			*GetNameSafe(Controller->PlayerState), *GetNameSafe(Data), *GetNameSafe(Hero), Hero->GetTeamId());
	}
	else
	{
		UE_LOG(LogSunriseHeroSpawn, Verbose, TEXT("Hero spawn rejected: PlayerState=%s PawnData=%s Transform=%s"),
			*GetNameSafe(Controller->PlayerState), *GetNameSafe(Data), *Transform.ToHumanReadableString());
	}
	return Hero;
}

ASunriseUnit* USunriseUnitManagerComponent::GetHeroForPlayer(const AController* Controller) const
{
	if (!IsValid(Controller))
	{
		return nullptr;
	}
	const TWeakObjectPtr<ASunriseUnit>* Hero = PlayerHeroes.Find(Controller);
	return Hero ? Hero->Get() : nullptr;
}

const AController* USunriseUnitManagerComponent::GetPlayerForHero(const ASunriseUnit* Hero) const
{
	if (!IsValid(Hero))
	{
		return nullptr;
	}
	for (const auto& Entry : PlayerHeroes)
	{
		if (Entry.Value.Get() == Hero)
		{
			return Entry.Key.Get();
		}
	}
	return nullptr;
}

void USunriseUnitManagerComponent::RegisterUnit(ASunriseUnit* Unit)
{
	if (IsValid(Unit))
	{
		Units.RemoveAllSwap(
			[](const TObjectPtr<ASunriseUnit>& Entry)
			{
				return !IsValid(Entry);
			});
		Units.AddUnique(Unit);
	}
}

void USunriseUnitManagerComponent::NotifyUnitDied(ASunriseUnit* Unit)
{
	if (!HasAuthority() || !IsValid(Unit))
	{
		return;
	}
	if (AController* Controller = Cast<AController>(Unit->GetControllingAgent().GetObject()))
	{
		if (UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(Controller))
		{
			Manager->UnregisterControlledEntity(Unit);
		}
	}
	OnArmyCountChanged.Broadcast(GetFriendlyAlive(), GetEnemyAlive());
	OnUnitDied.Broadcast(Unit);
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
	return GetAliveUnitCountForTeam(0);
}
int32 USunriseUnitManagerComponent::GetEnemyAlive() const
{
	return GetAliveUnitCountForTeam(1);
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
	float Earliest = -1.0f;
	for (const ASunriseUnit* Unit : Units)
	{
		if (IsValid(Unit) && Unit->IsHero() && !Unit->IsAlive() && Unit->GetTeamId() == TeamId)
		{
			const float Remaining = Unit->GetRespawnSeconds();
			if (Remaining >= 0.0f && (Earliest < 0.0f || Remaining < Earliest))
			{
				Earliest = Remaining;
			}
		}
	}
	return Earliest;
}
