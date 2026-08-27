// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Components/OverloadInteractorComponent.h"

#include <EnvironmentQuery/EnvQuery.h>

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameModes/Overload/Abilities/OverloadHackAbility.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/SunriseUnit.h"
#include "Vitality/Attributes/SunriseCombatSet.h"

UOverloadInteractorComponent::UOverloadInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
	HackAbilityClass = UOverloadHackAbility::StaticClass();
	// Example direct asset path: create or rename the EQS asset at this location if you want a native default.
	static ConstructorHelpers::FObjectFinder<UObject> DefaultHackApproachQuery(
		TEXT("/Game/Sunrise/System/Experiences/Overload/EQS/EQS_OverloadHackApproach.EQS_OverloadHackApproach"));
	if (DefaultHackApproachQuery.Succeeded())
	{
		HackApproachQuery = Cast<UEnvQuery>(DefaultHackApproachQuery.Object);
	}
}

void UOverloadInteractorComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeForUnit();
}

void UOverloadInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelHack();
	if (Unit && Unit->HasAuthority() && HackAbilityHandle.IsValid())
	{
		Unit->GetAbilitySystemComponent()->ClearAbility(HackAbilityHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UOverloadInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Unit || !Unit->HasAuthority() || !HackTarget.IsValid())
	{
		return;
	}
	if (Unit->HasActivePlayerOrder())
	{
		CancelHack();
		return;
	}
	const float CurrentHealth = Unit->GetHealth();
	if (CurrentHealth + KINDA_SMALL_NUMBER < LastObservedHealth)
	{
		CancelHack();
		LastObservedHealth = CurrentHealth;
		return;
	}
	LastObservedHealth = CurrentHealth;
	if (ShouldAbortHackForThreat())
	{
		CancelHack();
		return;
	}
	if (bApproachQueryPending)
	{
		if (GetWorld() && ApproachQueryStartTime >= 0.0f && GetWorld()->GetTimeSeconds() - ApproachQueryStartTime > ApproachQueryTimeout)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overload EQS approach query timed out for %s; using fallback approach location"),
				*GetNameSafe(Unit));
			bApproachQueryPending = false;
			ApproachLocation = ResolveFallbackApproachLocation(HackTarget.Get());
			IssueApproachMove();
		}
		return;
	}
	AActor* Target = HackTarget.Get();
	if (!Unit->IsAlive() || !IOverloadHackable::Execute_CanBeHackedByTeam(Target, Unit->GetTeamId()))
	{
		CancelHack();
		return;
	}
	const FVector HackLocation = IOverloadHackable::Execute_GetHackLocation(Target);
	if (FVector::DistSquared2D(Unit->GetActorLocation(), HackLocation) <= FMath::Square(InteractionRange))
	{
		if (!bRegisteredAtTarget)
		{
			Unit->GetAbilitySystemComponent()->TryActivateAbility(HackAbilityHandle);
		}
	}
	else if (AAIController* AI = Cast<AAIController>(Unit->GetController()))
	{
		if (bRegisteredAtTarget)
		{
			IOverloadHackable::Execute_UnregisterHacker(Target, Unit);
		}
		bRegisteredAtTarget = false;
		IssueApproachMove();
	}
}

void UOverloadInteractorComponent::InitializeForUnit()
{
	Unit = Cast<ASunriseUnit>(GetOwner());
	if (!Unit || !Unit->HasAuthority() || HackAbilityHandle.IsValid() || !HackAbilityClass)
	{
		return;
	}
	HackAbilityHandle = Unit->GetAbilitySystemComponent()->GiveAbility(FGameplayAbilitySpec(HackAbilityClass, 1));
	LastObservedHealth = Unit->GetHealth();
}

bool UOverloadInteractorComponent::RequestHack(AActor* Target, bool bFromPlayer)
{
	if (Unit && Unit->HasActivePlayerOrder() && !bFromPlayer)
	{
		return false;
	}
	if (!Unit || !Unit->HasAuthority() || !IsValid(Target) || !Target->Implements<UOverloadHackable>() ||
		!IOverloadHackable::Execute_CanBeHackedByTeam(Target, Unit->GetTeamId()))
	{
		return false;
	}
	if (HackTarget.Get() == Target)
	{
		return true;
	};
	CancelHack();
	if (bFromPlayer)
	{
		Unit->StopOrder_Implementation();
	}
	HackTarget = Target;
	Unit->SetExternalInteractionActive(true);
	if (HackApproachQuery)
	{
		StartApproachQuery();
		return true;
	}
	ApproachLocation = ResolveFallbackApproachLocation(Target);
	IssueApproachMove();
	return true;
}

void UOverloadInteractorComponent::CancelHack()
{
	if (bRegisteredAtTarget && HackTarget.IsValid())
	{
		IOverloadHackable::Execute_UnregisterHacker(HackTarget.Get(), Unit);
	}
	bRegisteredAtTarget = false;
	bApproachQueryPending = false;
	ApproachQueryStartTime = -1.0f;
	bApproachMoveIssued = false;
	HackTarget.Reset();
	ApproachLocation = FVector::ZeroVector;
	LastObservedHealth = Unit ? Unit->GetHealth() : 0.0f;
	if (Unit)
	{
		Unit->SetExternalInteractionActive(false);
	}
}

void UOverloadInteractorComponent::CommitHack()
{
	if (!Unit || !HackTarget.IsValid())
	{
		return;
	}
	IOverloadHackable::Execute_RegisterHacker(HackTarget.Get(), Unit);
	bRegisteredAtTarget = true;
	Unit->StopMoving();
}

void UOverloadInteractorComponent::StartApproachQuery()
{
	if (!Unit || !HackTarget.IsValid() || !HackApproachQuery || bApproachQueryPending)
	{
		return;
	}
	bApproachQueryPending = true;
	ApproachQueryStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;
	FEnvQueryRequest Request(HackApproachQuery, Unit.Get());
	Request.Execute(EEnvQueryRunMode::SingleResult, this, &UOverloadInteractorComponent::HandleApproachQueryFinished);
}

void UOverloadInteractorComponent::HandleApproachQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	bApproachQueryPending = false;
	if (!Unit || !HackTarget.IsValid())
	{
		return;
	}
	if (Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0)
	{
		ApproachLocation = Result->GetItemAsLocation(0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Overload EQS approach query failed for %s; using fallback approach location"), *GetNameSafe(Unit));
		ApproachLocation = ResolveFallbackApproachLocation(HackTarget.Get());
	}
	bApproachMoveIssued = false;
	IssueApproachMove();
}

bool UOverloadInteractorComponent::ShouldAbortHackForThreat() const
{
	if (!Unit || !HackTarget.IsValid())
	{
		return false;
	}
	const USunriseCombatSet* CombatSet = Unit->GetCombatSet();
	const float SenseRadius = CombatSet ? FMath::Max(CombatSet->GetAggroRadius(), InteractionRange) : InteractionRange;
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OverloadHackSense), false, Unit);
	if (!Unit->GetWorld()->OverlapMultiByObjectType(
			Overlaps, Unit->GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(SenseRadius), Params))
	{
		return false;
	}
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ASunriseUnit* OtherUnit = Cast<ASunriseUnit>(Overlap.GetActor());
		if (!OtherUnit || OtherUnit == Unit || !OtherUnit->IsAlive() || OtherUnit->GetTeamId() == Unit->GetTeamId())
		{
			continue;
		}
		return true;
	}
	return false;
}

void UOverloadInteractorComponent::IssueApproachMove()
{
	if (bApproachMoveIssued || !Unit)
	{
		return;
	}
	if (AAIController* AI = Cast<AAIController>(Unit->GetController()))
	{
		AI->MoveToLocation(ApproachLocation, ApproachAcceptanceRadius);
		bApproachMoveIssued = true;
	}
}

FVector UOverloadInteractorComponent::ResolveFallbackApproachLocation(AActor* Target) const
{
	const FVector HackLocation = IOverloadHackable::Execute_GetHackLocation(Target);
	if (!Unit || ApproachSlotCount <= 0)
	{
		return HackLocation;
	}
	const int32 SlotIndex = static_cast<int32>(Unit->GetUniqueID() % static_cast<uint32>(ApproachSlotCount));
	const float SlotAlpha = ApproachSlotCount > 1 ? static_cast<float>(SlotIndex) / static_cast<float>(ApproachSlotCount - 1) : 0.5f;
	const float Angle = FMath::Lerp(-UE_HALF_PI, UE_HALF_PI, SlotAlpha);
	FVector Outward = HackLocation - Target->GetActorLocation();
	Outward.Z = 0.0f;
	if (!Outward.Normalize())
	{
		Outward = Target->GetActorRightVector();
		Outward.Z = 0.0f;
		Outward.Normalize();
	}
	const FVector Side(-Outward.Y, Outward.X, 0.0f);
	const FVector ApproachDirection = Outward * FMath::Cos(Angle) + Side * FMath::Sin(Angle);
	return HackLocation + ApproachDirection * ApproachRingRadius;
}
