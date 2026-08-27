// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"

#include "OverloadEffects.generated.h"

UCLASS()
class SUNRISEGAME_API UOverloadIntegrityEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UOverloadIntegrityEffect();
	static FName MagnitudeName() { return TEXT("Overload.Integrity"); }
};

UCLASS()
class SUNRISEGAME_API UOverloadEnergyEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UOverloadEnergyEffect();
	static FName MagnitudeName() { return TEXT("Overload.Energy"); }
};

UCLASS()
class SUNRISEGAME_API UOverloadObjectiveScalingEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UOverloadObjectiveScalingEffect();
	static FName AttackName() { return TEXT("Overload.Scale.Attack"); }
	static FName ArmorName() { return TEXT("Overload.Scale.Armor"); }
	static FName ResistanceName() { return TEXT("Overload.Scale.Resistance"); }
};
