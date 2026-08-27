// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadCaptureComponent.h"

#include "Components/TFTeamActorComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"
#include "GameModes/Overload/Actors/Base/OverloadObjectiveBase.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "Net/UnrealNetwork.h"
#include "Units/SunriseUnit.h"

UOverloadCaptureComponent::UOverloadCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
	SetIsReplicatedByDefault(true);
}

void UOverloadCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
	TeamComponent = GetOwner()->FindComponentByClass<UTFTeamActorComponent>();
}

void UOverloadCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner()->HasAuthority() || !TeamComponent)
	{
		return;
	}

	const FVector CaptureLocation = GetOwner()->Implements<UOverloadHackable>() ? IOverloadHackable::Execute_GetHackLocation(GetOwner())
																				: GetOwner()->GetActorLocation();
	TMap<int32, int32> RegisteredHackers;
	TMap<int32, int32> PresentTeams;
	PruneAndCountHackers(RegisteredHackers);
	CountPresentTeams(CaptureLocation, PresentTeams);

	int32 PresentTeamId = INDEX_NONE;
	int32 PresentUnitCount = 0;
	int32 PresentTeamCount = 0;
	for (const TPair<int32, int32>& Pair : PresentTeams)
	{
		if (Pair.Value <= 0)
		{
			continue;
		}
		++PresentTeamCount;
		PresentTeamId = Pair.Key;
		PresentUnitCount = Pair.Value;
	}
	bCaptureContested = PresentTeamCount > 1;

	const bool bCanCapture =
		PresentTeamCount == 1 && PresentTeamId != TeamComponent->GetTeamId() && RegisteredHackers.FindRef(PresentTeamId) > 0;
	if (!bCanCapture)
	{
		// Contested hackers must be able to fight until only one team controls the zone.
		UpdateHackerInteractionLocks(false, INDEX_NONE);
		DecayCapture(DeltaTime);
		return;
	}
	UpdateHackerInteractionLocks(true, PresentTeamId);
	if (ActiveHackingTeamId != PresentTeamId)
	{
		ActiveHackingTeamId = PresentTeamId;
		CaptureProgress = 0.0f;
	}
	ActiveCapturingUnitCount = PresentUnitCount;
	CaptureProgress += DeltaTime * PresentUnitCount / (BaseHackSeconds * GetOwnerHackResistance());
	OnCaptureProgress.Broadcast(ActiveHackingTeamId, FMath::Clamp(CaptureProgress, 0.0f, 1.0f));
	if (CaptureProgress >= 1.0f)
	{
		const int32 Previous = TeamComponent->GetTeamId();
		if (TeamComponent->SetTeamId(ActiveHackingTeamId))
		{
			OnCaptured.Broadcast(Previous, ActiveHackingTeamId);
		}
		UpdateHackerInteractionLocks(false, INDEX_NONE);
		CaptureProgress = 0.0f;
		ActiveHackingTeamId = INDEX_NONE;
		ActiveCapturingUnitCount = 0;
		bCaptureContested = false;
		Hackers.Reset();
	}
}

void UOverloadCaptureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UOverloadCaptureComponent, CaptureProgress);
	DOREPLIFETIME(UOverloadCaptureComponent, ActiveHackingTeamId);
	DOREPLIFETIME(UOverloadCaptureComponent, ActiveCapturingUnitCount);
	DOREPLIFETIME(UOverloadCaptureComponent, bCaptureContested);
}

void UOverloadCaptureComponent::RegisterHacker(ASunriseUnit* Unit)
{
	if (GetOwner()->HasAuthority() && IsValid(Unit) && Unit->IsAlive() && TeamComponent && Unit->GetTeamId() >= 0 &&
		Unit->GetTeamId() != TeamComponent->GetTeamId())
	{
		Hackers.AddUnique(Unit);
	}
}

void UOverloadCaptureComponent::UnregisterHacker(ASunriseUnit* Unit)
{
	Hackers.Remove(Unit);
}

void UOverloadCaptureComponent::PruneAndCountHackers(TMap<int32, int32>& OutCounts)
{
	const FVector CaptureLocation = GetOwner()->Implements<UOverloadHackable>() ? IOverloadHackable::Execute_GetHackLocation(GetOwner())
																				: GetOwner()->GetActorLocation();
	for (int32 Index = Hackers.Num() - 1; Index >= 0; --Index)
	{
		ASunriseUnit* Unit = Hackers[Index].Get();
		if (!Unit || !Unit->IsAlive() || !TeamComponent || Unit->GetTeamId() == TeamComponent->GetTeamId() ||
			FVector::DistSquared2D(Unit->GetActorLocation(), CaptureLocation) > FMath::Square(HackRadius))
		{
			Hackers.RemoveAtSwap(Index);
			continue;
		}
		OutCounts.FindOrAdd(Unit->GetTeamId())++;
	}
}

void UOverloadCaptureComponent::CountPresentTeams(const FVector& CaptureLocation, TMap<int32, int32>& OutCounts) const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OverloadCapturePresence), false, GetOwner());
	GetWorld()->OverlapMultiByObjectType(Overlaps, CaptureLocation, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(HackRadius), QueryParams);

	TSet<ASunriseUnit*> UniqueUnits;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ASunriseUnit* Unit = Cast<ASunriseUnit>(Overlap.GetActor());
		if (Unit && Unit->IsAlive() && Unit->GetTeamId() >= 0)
		{
			UniqueUnits.Add(Unit);
		}
	}
	for (const ASunriseUnit* Unit : UniqueUnits)
	{
		OutCounts.FindOrAdd(Unit->GetTeamId())++;
	}
}

void UOverloadCaptureComponent::UpdateHackerInteractionLocks(bool bLockCapturingTeam, int32 CapturingTeamId)
{
	for (const TWeakObjectPtr<ASunriseUnit>& UnitPtr : Hackers)
	{
		if (ASunriseUnit* Unit = UnitPtr.Get())
		{
			if (!Unit->HasActivePlayerOrder())
			{
				Unit->SetExternalInteractionActive(bLockCapturingTeam && Unit->GetTeamId() == CapturingTeamId);
			}
		}
	}
}

void UOverloadCaptureComponent::DecayCapture(float DeltaTime)
{
	ActiveCapturingUnitCount = 0;
	CaptureProgress = FMath::Max(0.0f, CaptureProgress - IdleDecayPerSecond * DeltaTime);
	if (CaptureProgress <= 0.0f)
	{
		ActiveHackingTeamId = INDEX_NONE;
	}
}

float UOverloadCaptureComponent::GetOwnerHackResistance() const
{
	const AOverloadObjectiveBase* Objective = Cast<AOverloadObjectiveBase>(GetOwner());
	return Objective && Objective->GetHackAttributes() ? FMath::Max(0.1f, Objective->GetHackAttributes()->GetHackResistance()) : 1.0f;
}
