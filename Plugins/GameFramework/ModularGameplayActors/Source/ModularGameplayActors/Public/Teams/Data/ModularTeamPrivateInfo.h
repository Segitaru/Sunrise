// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Teams/Data/ModularTeamInfoBase.h"

#include "ModularTeamPrivateInfo.generated.h"

class UObject;

UCLASS()
class AModularTeamPrivateInfo : public AModularTeamInfoBase
{
	GENERATED_BODY()

public:
	AModularTeamPrivateInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
