// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BuildingTypes)

DEFINE_LOG_CATEGORY(LogGameBuilder);

bool FGameplayAbilityTargetData_ActorPlacement::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	PlacementActorClass.Serialize(Ar);

	bOutSuccess = true;
	return true;
}

int32 UBuildingTypes::GetLocalUserId(AActor* FromActor)
{
	APlayerController* CurrentPC = nullptr;
	;
	AActor* TestActor = FromActor;
	while (TestActor)
	{
		if (APlayerController* CastPC = Cast<APlayerController>(TestActor))
		{
			CurrentPC = CastPC;
			break;
		}

		if (APawn* Pawn = Cast<APawn>(TestActor))
		{
			CurrentPC = Cast<APlayerController>(Pawn->GetController());
			break;
		}

		TestActor = TestActor->GetOwner();
	}

	if (!CurrentPC)
	{
		return 0;
	}

	const ULocalPlayer* LocalPlayer = CurrentPC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return 0;
	}

	return LocalPlayer->GetPlatformUserIndex();
}

FGameplayAbilityTargetData_ActorPlacement* UBuildingTypes::ExtractActorPlacementData(FGameplayAbilityTargetDataHandle InTargetData)
{
	FGameplayAbilityTargetData* TargetDataInfo = InTargetData.Get(0);

	if (TargetDataInfo && TargetDataInfo->GetScriptStruct()->IsChildOf(FGameplayAbilityTargetData_ActorPlacement::StaticStruct()))
	{
		return (FGameplayAbilityTargetData_ActorPlacement*)TargetDataInfo;
	}

	return nullptr;
}
TSoftClassPtr<AActor> UBuildingTypes::GetPlacementActorFromTargetData(FGameplayAbilityTargetDataHandle InTargetData)
{
	if (const auto ActorPlacementData = ExtractActorPlacementData(InTargetData))
	{
		return MoveTemp(ActorPlacementData->PlacementActorClass);
	}

	return nullptr;
}