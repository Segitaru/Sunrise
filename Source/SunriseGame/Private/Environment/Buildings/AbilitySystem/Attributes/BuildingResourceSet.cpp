// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/BuildingResourceSet.h"

#include <Net/UnrealNetwork.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(BuildingResourceSet)

UBuildingResourceSet::UBuildingResourceSet()
	: Food(100.f)
	, Wood(100.f)
	, Stone(100.f)
	, Metal(100)
{
}

void UBuildingResourceSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBuildingResourceSet, Food, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBuildingResourceSet, Wood, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBuildingResourceSet, Stone, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBuildingResourceSet, Metal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UBuildingResourceSet::OnRep_Food(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Food, OldValue);
}

void UBuildingResourceSet::OnRep_Wood(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Wood, OldValue);
}

void UBuildingResourceSet::OnRep_Stone(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Stone, OldValue);
}

void UBuildingResourceSet::OnRep_Metal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Metal, OldValue);
}