// Fill out your copyright notice in the Description page of Project Settings.


#include "Units/AI/SunriseBlackboardData.h"

#include <BehaviorTree/Blackboard/BlackboardKeyType_Bool.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Object.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Vector.h>

void USunriseBlackboardData::PostLoad()
{
	Super::PostLoad();
	UpdateStartupKeys();
}

void USunriseBlackboardData::UpdateStartupKeys()
{
	if (UBlackboardKeyType_Bool* PlayerOrderKeyType = UpdatePersistentKey<UBlackboardKeyType_Bool>(TEXT("bHavePlayerOrder")))
	{
		PlayerOrderKeyType->bDefaultValue = false;
	}

	if (UBlackboardKeyType_Object* PlayerOrderTargetActorKeyType =
			UpdatePersistentKey<UBlackboardKeyType_Object>(TEXT("PlayerOrderTargetActor")))
	{
		PlayerOrderTargetActorKeyType->BaseClass = AActor::StaticClass();
	}

	if (UBlackboardKeyType_Vector* PlayerOrderTargetLocationKeyType =
			UpdatePersistentKey<UBlackboardKeyType_Vector>(TEXT("PlayerOrderTargetLocation")))
	{
		PlayerOrderTargetLocationKeyType->DefaultValue = FVector::ZeroVector;
	}

	if (UBlackboardKeyType_Object* TargetActorKeyType = UpdatePersistentKey<UBlackboardKeyType_Object>(TEXT("TargetActor")))
	{
		TargetActorKeyType->BaseClass = AActor::StaticClass();
	}

	if (UBlackboardKeyType_Vector* TargetLocationKeyType = UpdatePersistentKey<UBlackboardKeyType_Vector>(TEXT("TargetLocation")))
	{
		TargetLocationKeyType->DefaultValue = FVector::ZeroVector;
	}

#if WITH_EDITOR
	// MarkPackageDirty returning false means marking wasn't possible at this moment. Give it one more try in a moment
	if (GEditor != nullptr && MarkPackageDirty() == false)
	{
		TWeakObjectPtr<UBlackboardData> WeakAsset = this;
		AsyncTask(ENamedThreads::GameThread,
			[WeakAsset]()
			{
				if (UBlackboardData* AssetPtr = WeakAsset.Get())
				{
					AssetPtr->MarkPackageDirty();
				}
			});
	}
#endif // WITH_EDITOR
}

void USunriseBlackboardData::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_NeedPostLoad | RF_ClassDefaultObject) == false)
	{
		UpdateStartupKeys();
	}
}

#if WITH_EDITOR
void USunriseBlackboardData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateStartupKeys();
}
#endif