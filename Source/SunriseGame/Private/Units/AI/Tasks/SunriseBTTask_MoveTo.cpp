// Fill out your copyright notice in the Description page of Project Settings.


#include "Units/AI/Tasks/SunriseBTTask_MoveTo.h"

#include <AIController.h>
#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>

EBTNodeResult::Type USunriseBTTask_MoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const auto* const ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerComp.GetAIOwner()->GetPawn());
	if (!ASC)
	{
		return Super::ExecuteTask(OwnerComp, NodeMemory);
	}

	if (ASC->HasAttributeSetForAttribute(AcceptableRadiusAttribute))
	{
		AcceptableRadius = ASC->GetNumericAttribute(AcceptableRadiusAttribute);
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}