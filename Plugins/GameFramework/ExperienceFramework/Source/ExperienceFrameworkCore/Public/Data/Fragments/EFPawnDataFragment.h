// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <CoreMinimal.h>

#include "EFPawnDataFragment.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories)
class EXPERIENCEFRAMEWORKCORE_API UEFPawnDataFragment : public UObject
{
	GENERATED_BODY()

public:
	UEFPawnDataFragment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Activate(APawn* ForPawn) = delete;
};
