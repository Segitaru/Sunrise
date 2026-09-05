// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>

#include "ModularPawnDataFragment.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories)
class MODULARGAMEPLAYACTORS_API UModularPawnDataFragment : public UObject
{
	GENERATED_BODY()

public:
	UModularPawnDataFragment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Activate(AActor* ForOwner, APawn* ForPawn) {};
};
