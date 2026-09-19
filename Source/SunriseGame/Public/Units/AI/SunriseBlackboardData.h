// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BehaviorTree/BlackboardData.h"
#include "CoreMinimal.h"

#include "SunriseBlackboardData.generated.h"

/**
 *
 */
UCLASS()
class SUNRISEGAME_API USunriseBlackboardData : public UBlackboardData
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	void UpdateStartupKeys();
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
