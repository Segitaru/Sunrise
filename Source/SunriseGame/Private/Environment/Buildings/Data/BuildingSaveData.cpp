// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/BuildingSaveData.h"

#include "BuildingTypes.h"
#include "Gameplay/GameBuilding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BuildingSaveData)

FBuildingInfo::FBuildingInfo(const AGameBuilding* UpdatedBuilding)
{
	if (!UpdatedBuilding)
	{
		UE_LOG(LogGameBuilder, Error, TEXT("Try create building info with not existed build!"));
		return;
	}

	BuildingType = UpdatedBuilding->GetClass();

	UpdatedBuilding->GetCurrentState(CurrentLevel, BuildingProgress);

	Transform = UpdatedBuilding->GetTransform();

	VitalityState = UpdatedBuilding->GetVitalityValues();
}