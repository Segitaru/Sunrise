// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BehaviorTree/BlackboardData.h"
#include "CoreMinimal.h"

#include "SunriseBlackboardData.generated.h"

namespace SunriseBlackboardKeys
{
	inline const FName HavePlayerOrder(TEXT("bHavePlayerOrder"));
	inline const FName PlayerOrderTargetActor(TEXT("PlayerOrderTargetActor"));
	inline const FName PlayerOrderTargetLocation(TEXT("PlayerOrderTargetLocation"));
	inline const FName TargetActor(TEXT("TargetActor"));
	inline const FName TargetLocation(TEXT("TargetLocation"));
} // namespace SunriseBlackboardKeys

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
