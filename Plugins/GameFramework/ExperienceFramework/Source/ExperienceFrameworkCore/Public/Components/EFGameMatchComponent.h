// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "CoreMinimal.h"

#include "EFGameMatchComponent.generated.h"


USTRUCT()
struct FGameMatchTeamData
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	int32 AttackingTeamId = 1;

	UPROPERTY()
	int32 DefendingTeamId = 2;
};

USTRUCT()
struct FTeamPointsPerMatch
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	int32 TeamId = -1;

	UPROPERTY()
	int32 Points = 0;
};

USTRUCT()
struct FExtraTimeForTeam
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	int32 TeamId = -1;

	UPROPERTY()
	float Time = 0.0f;
};

UCLASS()
class EXPERIENCEFRAMEWORKCORE_API UEFGameMatchComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UEFGameMatchComponent(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeComponent() override;


	void SetTeamData(FGameMatchTeamData NewTeamData);

protected:
	UFUNCTION()
	virtual void OnRep_TeamData();

	UFUNCTION()
	virtual void OnRep_RoundCounter();

public:
	UPROPERTY(ReplicatedUsing = OnRep_TeamData)
	FGameMatchTeamData TeamData;

	UPROPERTY(Replicated)
	TArray<FTeamPointsPerMatch> TeamPointsPerMatch;

	UPROPERTY(Replicated)
	TArray<FExtraTimeForTeam> ExtraTimeForTeam;

	UPROPERTY(ReplicatedUsing = OnRep_RoundCounter)
	int32 RoundCounter;
};
