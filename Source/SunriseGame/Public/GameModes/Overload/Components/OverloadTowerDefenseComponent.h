// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "OverloadTowerDefenseComponent.generated.h"

UCLASS(ClassGroup = (Overload), BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API UOverloadTowerDefenseComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOverloadTowerDefenseComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UFUNCTION()
	void ExecuteDefensePulse();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Defense", meta = (ClampMin = "100.0", Units = "cm"))
	float ScanRadius = 900.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Overload|Defense", meta = (ClampMin = "0.1", Units = "s"))
	float DefensePulseInterval = 1.25f;

private:
	FTimerHandle DefenseTimer;
};
