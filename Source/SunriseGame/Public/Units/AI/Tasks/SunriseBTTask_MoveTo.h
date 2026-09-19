// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <AttributeSet.h>

#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "CoreMinimal.h"

#include "SunriseBTTask_MoveTo.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISEGAME_API USunriseBTTask_MoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(config, Category = Node, EditAnywhere)
	FGameplayAttribute AcceptableRadiusAttribute;
};
