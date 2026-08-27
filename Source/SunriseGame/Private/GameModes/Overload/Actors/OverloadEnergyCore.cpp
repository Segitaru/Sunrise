// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/Overload/Actors/OverloadEnergyCore.h"

#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameModes/Overload/AbilitySystem/OverloadAttributeSet.h"
#include "GameModes/Overload/Effects/OverloadEffects.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AOverloadEnergyCore::AOverloadEnergyCore()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CoreDummyMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CoreDummyMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CoreDummyMesh.Object);
	}
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	VisualMesh->SetRelativeScale3D(FVector(3.0f));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
}

void AOverloadEnergyCore::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UOverloadEnergySet::GetOverloadEnergyAttribute())
		.AddUObject(this, &AOverloadEnergyCore::HandleEnergyChanged);
}

void AOverloadEnergyCore::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || CoreState == EOverloadCoreState::Destroyed)
	{
		return;
	}
	float DeltaEnergy = 0.0f;
	if (CoreState == EOverloadCoreState::Overloading)
	{
		DeltaEnergy = EnergyAttributes->GetMaxOverloadEnergy() / OverloadSeconds * DeltaSeconds;
	}
	else if (CoreState == EOverloadCoreState::Cooling)
	{
		DeltaEnergy = -CoolingPerSecond * DeltaSeconds;
	}
	if (!FMath::IsNearlyZero(DeltaEnergy))
	{
		ApplyInstantEffect(UOverloadEnergyEffect::StaticClass(), UOverloadEnergyEffect::MagnitudeName(), DeltaEnergy);
	}
	if (CoreState == EOverloadCoreState::Cooling && EnergyAttributes->GetOverloadEnergy() <= KINDA_SMALL_NUMBER)
	{
		SetCoreState(EOverloadCoreState::Stable);
	}
}

void AOverloadEnergyCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOverloadEnergyCore, CoreState);
	DOREPLIFETIME(AOverloadEnergyCore, OverloadingTeamId);
}

void AOverloadEnergyCore::InitializeCore(int32 TeamId, float InOverloadSeconds, float InCoolingPerSecond)
{
	InitializeTeam(TeamId);
	OverloadSeconds = FMath::Max(1.0f, InOverloadSeconds);
	CoolingPerSecond = FMath::Max(0.0f, InCoolingPerSecond);
	InitializeObjectiveAttributes(MaxIntegrity, 0.0f, 35.0f, 1.0f, 100.0f);
}

void AOverloadEnergyCore::SetSupplyCompromised(bool bCompromised, int32 InOverloadingTeamId)
{
	if (!HasAuthority() || CoreState == EOverloadCoreState::Destroyed)
	{
		return;
	}
	OverloadingTeamId = bCompromised ? InOverloadingTeamId : INDEX_NONE;
	SetCoreState(bCompromised									? EOverloadCoreState::Overloading
				 : EnergyAttributes->GetOverloadEnergy() > 0.0f ? EOverloadCoreState::Cooling
																: EOverloadCoreState::Stable);
}

float AOverloadEnergyCore::GetOverloadPercent() const
{
	return EnergyAttributes && EnergyAttributes->GetMaxOverloadEnergy() > 0.0f
			   ? FMath::Clamp(EnergyAttributes->GetOverloadEnergy() / EnergyAttributes->GetMaxOverloadEnergy(), 0.0f, 1.0f)
			   : 0.0f;
}

void AOverloadEnergyCore::SetCoreState(EOverloadCoreState NewState)
{
	if (CoreState == NewState)
	{
		return;
	}
	CoreState = NewState;
	OnCoreStateChanged.Broadcast(this, CoreState);
	BP_CoreStateChanged(CoreState);
}

void AOverloadEnergyCore::HandleEnergyChanged(const FOnAttributeChangeData& Data)
{
	if (HasAuthority() && Data.NewValue >= EnergyAttributes->GetMaxOverloadEnergy() - KINDA_SMALL_NUMBER &&
		CoreState != EOverloadCoreState::Destroyed)
	{
		SetCoreState(EOverloadCoreState::Destroyed);
		OnCoreExploded.Broadcast(this, OverloadingTeamId);
		BP_CoreExploded();
		SetActorEnableCollision(false);
	}
}
