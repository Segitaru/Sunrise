// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/SunrisePawn.h"

#include "Components/SceneComponent.h"

ASunrisePawn::ASunrisePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
