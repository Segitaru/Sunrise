// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModularPawn.h"

#include "SunrisePawn.generated.h"

class UModularHeroComponent;
class UModularPawnExtensionComponent;

/** Neutral player pawn shell. Experiences add camera/input/gameplay features as components. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunrisePawn : public AModularPawn
{
	GENERATED_BODY()

public:
	ASunrisePawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UModularHeroComponent> HeroComponent;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UModularPawnExtensionComponent> PawnExtensionComponent;

	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
