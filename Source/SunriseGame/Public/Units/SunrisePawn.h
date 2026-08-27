// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Pawn.h"

#include "SunrisePawn.generated.h"

/** Neutral player pawn shell. Experiences add camera/input/gameplay features as components. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunrisePawn : public APawn
{
	GENERATED_BODY()

public:
	ASunrisePawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
