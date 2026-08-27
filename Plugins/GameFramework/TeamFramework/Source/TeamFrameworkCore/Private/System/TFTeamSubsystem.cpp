#include "System/TFTeamSubsystem.h"

#include "Components/TFTeamActorComponent.h"
#include "Data/TFTeamDisplayAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "System/TFTeamAgentInterface.h"

bool UTFTeamSubsystem::ChangeTeamForActor(AActor* ActorToChange, int32 NewTeamId)
{
	if (!ActorToChange || !ActorToChange->HasAuthority())
	{
		return false;
	}
	if (UTFTeamActorComponent* Component = ActorToChange->FindComponentByClass<UTFTeamActorComponent>())
	{
		return Component->SetTeamId(NewTeamId);
	}
	if (ITFTeamAgentInterface* TeamAgent = Cast<ITFTeamAgentInterface>(ActorToChange))
	{
		const FGenericTeamId PreviousTeam = TeamAgent->GetGenericTeamId();
		TeamAgent->SetGenericTeamId(IntegerToTFTeamId(NewTeamId));
		return PreviousTeam != TeamAgent->GetGenericTeamId();
	}
	return false;
}

int32 UTFTeamSubsystem::FindTeamFromObject(const UObject* TestObject) const
{
	if (!TestObject)
	{
		return INDEX_NONE;
	}
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestObject))
	{
		const int32 TeamId = TFTeamIdToInteger(TeamAgent->GetGenericTeamId());
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

int32 UTFTeamSubsystem::FindTeamFromActor(const AActor* Actor, TSet<const UObject*>& VisitedObjects) const
{
	if (!Actor || VisitedObjects.Contains(Actor))
	{
		return INDEX_NONE;
	}
	VisitedObjects.Add(Actor);
	if (const UTFTeamActorComponent* Component = Actor->FindComponentByClass<UTFTeamActorComponent>())
	{
		const int32 TeamId = Component->GetTeamId();
		if (TeamId != INDEX_NONE)
		{
			return TeamId;
		}
	}
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
	{
		const int32 TeamId = TFTeamIdToInteger(TeamAgent->GetGenericTeamId());
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

void UTFTeamSubsystem::FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, int32& TeamId) const
{
	TeamId = FindTeamFromObject(TestObject);
	bIsPartOfTeam = TeamId != INDEX_NONE;
}

ETFTeamComparison UTFTeamSubsystem::CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA, int32& TeamIdB) const
{
	TeamIdA = FindTeamFromObject(A);
	TeamIdB = FindTeamFromObject(B);
	if (TeamIdA == INDEX_NONE || TeamIdB == INDEX_NONE)
	{
		return ETFTeamComparison::InvalidArgument;
	}
	return TeamIdA == TeamIdB ? ETFTeamComparison::OnSameTeam : ETFTeamComparison::DifferentTeams;
}

ETFTeamComparison UTFTeamSubsystem::CompareTeams(const UObject* A, const UObject* B) const
{
	int32 TeamIdA = INDEX_NONE;
	int32 TeamIdB = INDEX_NONE;
	return CompareTeams(A, B, TeamIdA, TeamIdB);
}

bool UTFTeamSubsystem::CanCauseDamage(const UObject* Instigator, const UObject* Target, bool bAllowDamageToSelf) const
{
	if (!Instigator || !Target)
	{
		return false;
	}
	if (Instigator == Target)
	{
		return bAllowDamageToSelf;
	}
	return CompareTeams(Instigator, Target) == ETFTeamComparison::DifferentTeams;
}

void UTFTeamSubsystem::AddTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	if (TeamId >= 0 && Tag.IsValid() && StackCount > 0)
	{
		TeamMap.FindOrAdd(TeamId).TagStacks.FindOrAdd(Tag) += StackCount;
	}
}

void UTFTeamSubsystem::RemoveTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	FTFTeamRuntimeData* Team = TeamMap.Find(TeamId);
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

int32 UTFTeamSubsystem::GetTeamTagStackCount(int32 TeamId, FGameplayTag Tag) const
{
	const FTFTeamRuntimeData* Team = TeamMap.Find(TeamId);
	return Team ? Team->TagStacks.FindRef(Tag) : 0;
}

void UTFTeamSubsystem::SetTeamDisplayAsset(int32 TeamId, UTFTeamDisplayAsset* DisplayAsset)
{
	if (TeamId < 0)
	{
		return;
	}
	FTFTeamRuntimeData& Team = TeamMap.FindOrAdd(TeamId);
	if (Team.DisplayAsset != DisplayAsset)
	{
		Team.DisplayAsset = DisplayAsset;
		Team.OnDisplayAssetChanged.Broadcast(DisplayAsset);
	}
}

UTFTeamDisplayAsset* UTFTeamSubsystem::GetTeamDisplayAsset(int32 TeamId) const
{
	const FTFTeamRuntimeData* Team = TeamMap.Find(TeamId);
	return Team ? Team->DisplayAsset : nullptr;
}

TArray<int32> UTFTeamSubsystem::GetTeamIds() const
{
	TArray<int32> Result;
	TeamMap.GetKeys(Result);
	Result.Sort();
	return Result;
}

void UTFTeamSubsystem::NotifyTeamDisplayAssetModified(UTFTeamDisplayAsset* ModifiedAsset)
{
	for (TPair<int32, FTFTeamRuntimeData>& Pair : TeamMap)
	{
		if (Pair.Value.DisplayAsset == ModifiedAsset)
		{
			Pair.Value.OnDisplayAssetChanged.Broadcast(ModifiedAsset);
		}
	}
}

FOnTFTeamDisplayAssetChangedDelegate& UTFTeamSubsystem::GetTeamDisplayAssetChangedDelegate(int32 TeamId)
{
	return TeamMap.FindOrAdd(TeamId).OnDisplayAssetChanged;
}
