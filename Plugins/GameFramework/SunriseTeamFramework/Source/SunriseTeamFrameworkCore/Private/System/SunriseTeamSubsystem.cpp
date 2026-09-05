#include "System/SunriseTeamSubsystem.h"

#include "Components/SunriseTeamActorComponent.h"
#include "Data/SunriseTeamDisplayAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "System/SunriseTeamAgentInterface.h"

bool USunriseTeamSubsystem::ChangeTeamForActor(AActor* ActorToChange, int32 NewTeamId)
{
	if (!ActorToChange || !ActorToChange->HasAuthority())
	{
		return false;
	}
	if (USunriseTeamActorComponent* Component = ActorToChange->FindComponentByClass<USunriseTeamActorComponent>())
	{
		return Component->SetTeamId(NewTeamId);
	}
	if (ISunriseTeamAgentInterface* TeamAgent = Cast<ISunriseTeamAgentInterface>(ActorToChange))
	{
		const FGenericTeamId PreviousTeam = TeamAgent->GetGenericTeamId();
		TeamAgent->SetGenericTeamId(IntegerToSunriseTeamId(NewTeamId));
		return PreviousTeam != TeamAgent->GetGenericTeamId();
	}
	return false;
}

int32 USunriseTeamSubsystem::FindTeamFromObject(const UObject* TestObject) const
{
	if (!TestObject)
	{
		return INDEX_NONE;
	}
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestObject))
	{
		const int32 TeamId = SunriseTeamIdToInteger(TeamAgent->GetGenericTeamId());
		if (TeamId != INDEX_NONE)
		{
			return TeamId;
		}
	}
	if (const AActor* Actor = Cast<AActor>(TestObject))
	{
		TSet<const UObject*> VisitedObjects;
		return FindTeamFromActor(Actor, VisitedObjects);
	}
	if (const UActorComponent* Component = Cast<UActorComponent>(TestObject))
	{
		return FindTeamFromObject(Component->GetOwner());
	}
	return INDEX_NONE;
}

int32 USunriseTeamSubsystem::FindTeamFromActor(const AActor* Actor, TSet<const UObject*>& VisitedObjects) const
{
	if (!Actor || VisitedObjects.Contains(Actor))
	{
		return INDEX_NONE;
	}
	VisitedObjects.Add(Actor);
	if (const USunriseTeamActorComponent* Component = Actor->FindComponentByClass<USunriseTeamActorComponent>())
	{
		const int32 TeamId = Component->GetTeamId();
		if (TeamId != INDEX_NONE)
		{
			return TeamId;
		}
	}
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
	{
		const int32 TeamId = SunriseTeamIdToInteger(TeamAgent->GetGenericTeamId());
		if (TeamId != INDEX_NONE)
		{
			return TeamId;
		}
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const int32 TeamId = FindTeamFromActor(Pawn->GetController(), VisitedObjects); TeamId != INDEX_NONE)
		{
			return TeamId;
		}
	}
	if (const int32 TeamId = FindTeamFromActor(Actor->GetOwner(), VisitedObjects); TeamId != INDEX_NONE)
	{
		return TeamId;
	}
	return FindTeamFromActor(Actor->GetInstigator(), VisitedObjects);
}

void USunriseTeamSubsystem::FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, int32& TeamId) const
{
	TeamId = FindTeamFromObject(TestObject);
	bIsPartOfTeam = TeamId != INDEX_NONE;
}

ETeamComparison USunriseTeamSubsystem::CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA, int32& TeamIdB) const
{
	TeamIdA = FindTeamFromObject(A);
	TeamIdB = FindTeamFromObject(B);
	if (TeamIdA == INDEX_NONE || TeamIdB == INDEX_NONE)
	{
		return ETeamComparison::InvalidArgument;
	}
	return TeamIdA == TeamIdB ? ETeamComparison::OnSameTeam : ETeamComparison::DifferentTeams;
}

ETeamComparison USunriseTeamSubsystem::CompareTeams(const UObject* A, const UObject* B) const
{
	int32 TeamIdA = INDEX_NONE;
	int32 TeamIdB = INDEX_NONE;
	return CompareTeams(A, B, TeamIdA, TeamIdB);
}

bool USunriseTeamSubsystem::CanCauseDamage(const UObject* Instigator, const UObject* Target, bool bAllowDamageToSelf) const
{
	if (!Instigator || !Target)
	{
		return false;
	}
	if (Instigator == Target)
	{
		return bAllowDamageToSelf;
	}
	return CompareTeams(Instigator, Target) == ETeamComparison::DifferentTeams;
}

void USunriseTeamSubsystem::AddTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	if (TeamId >= 0 && Tag.IsValid() && StackCount > 0)
	{
		TeamMap.FindOrAdd(TeamId).TagStacks.FindOrAdd(Tag) += StackCount;
	}
}

void USunriseTeamSubsystem::RemoveTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	FTeamRuntimeData* Team = TeamMap.Find(TeamId);
	if (!Team || !Tag.IsValid() || StackCount <= 0)
	{
		return;
	}
	int32& Count = Team->TagStacks.FindOrAdd(Tag);
	Count = FMath::Max(0, Count - StackCount);
	if (Count == 0)
	{
		Team->TagStacks.Remove(Tag);
	}
}

int32 USunriseTeamSubsystem::GetTeamTagStackCount(int32 TeamId, FGameplayTag Tag) const
{
	const FTeamRuntimeData* Team = TeamMap.Find(TeamId);
	return Team ? Team->TagStacks.FindRef(Tag) : 0;
}

void USunriseTeamSubsystem::SetTeamDisplayAsset(int32 TeamId, USunriseTeamDisplayAsset* DisplayAsset)
{
	if (TeamId < 0)
	{
		return;
	}
	FTeamRuntimeData& Team = TeamMap.FindOrAdd(TeamId);
	if (Team.DisplayAsset != DisplayAsset)
	{
		Team.DisplayAsset = DisplayAsset;
		Team.OnDisplayAssetChanged.Broadcast(DisplayAsset);
	}
}

USunriseTeamDisplayAsset* USunriseTeamSubsystem::GetTeamDisplayAsset(int32 TeamId) const
{
	const FTeamRuntimeData* Team = TeamMap.Find(TeamId);
	return Team ? Team->DisplayAsset : nullptr;
}

TArray<int32> USunriseTeamSubsystem::GetTeamIds() const
{
	TArray<int32> Result;
	TeamMap.GetKeys(Result);
	Result.Sort();
	return Result;
}

void USunriseTeamSubsystem::NotifyTeamDisplayAssetModified(USunriseTeamDisplayAsset* ModifiedAsset)
{
	for (TPair<int32, FTeamRuntimeData>& Pair : TeamMap)
	{
		if (Pair.Value.DisplayAsset == ModifiedAsset)
		{
			Pair.Value.OnDisplayAssetChanged.Broadcast(ModifiedAsset);
		}
	}
}

FOnTeamDisplayAssetChangedDelegate& USunriseTeamSubsystem::GetTeamDisplayAssetChangedDelegate(int32 TeamId)
{
	return TeamMap.FindOrAdd(TeamId).OnDisplayAssetChanged;
}
