// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OverloadHackable.generated.h"

class ASunriseUnit;

UINTERFACE(BlueprintType)
class SUNRISEGAME_API UOverloadHackable : public UInterface
{
	GENERATED_BODY()
};

class SUNRISEGAME_API IOverloadHackable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Overload|Hack")
	bool CanBeHackedByTeam(int32 TeamId) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Overload|Hack")
	FVector GetHackLocation() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Overload|Hack")
	void RegisterHacker(ASunriseUnit* Unit);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Overload|Hack")
	void UnregisterHacker(ASunriseUnit* Unit);
};
