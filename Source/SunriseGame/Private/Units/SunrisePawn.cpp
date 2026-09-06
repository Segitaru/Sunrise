// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/SunrisePawn.h"

#include "Components/ModularHeroComponent.h"
#include "Components/ModularPawnExtensionComponent.h"
#include "Components/SceneComponent.h"

ASunrisePawn::ASunrisePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	HeroComponent = CreateDefaultSubobject<UModularHeroComponent>("HeroComponent");
	PawnExtensionComponent = CreateDefaultSubobject<UModularPawnExtensionComponent>("ExtensionComponent");
}

void ASunrisePawn::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);

	PawnExtensionComponent->HandlePlayerStateReplicated();
}

void ASunrisePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtensionComponent->HandleControllerChanged();
}
