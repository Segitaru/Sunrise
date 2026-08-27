// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "SunriseUnitInterfaces.generated.h"

/** Selection is an interface so buildings or future vehicle actors can reuse the controller. */
UINTERFACE(BlueprintType)
class SUNRISEGAME_API USunriseSelectable : public UInterface
{
	GENERATED_BODY()
};

class SUNRISEGAME_API ISunriseSelectable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sunrise|Selection")
	bool CanBeSelectedBy(const APlayerController* Controller) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sunrise|Selection")
	void SetSunriseSelected(bool bSelected);
};

/** Orders are intentionally independent from the concrete Character implementation. */
UINTERFACE(BlueprintType)
class SUNRISEGAME_API USunriseOrderReceiver : public UInterface
{
	GENERATED_BODY()
};

class SUNRISEGAME_API ISunriseOrderReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sunrise|Orders")
	void IssueMoveOrder(const FVector& Destination);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sunrise|Orders")
	void IssueTargetOrder(AActor* TargetActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Sunrise|Orders")
	void StopOrder();
};
