// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModularTeamInfoBase.h"

#include "ModularTeamPublicInfo.generated.h"

class UModularTeamDisplayAsset;
class UObject;
struct FFrame;

UCLASS()
class AModularTeamPublicInfo : public AModularTeamInfoBase
{
	GENERATED_BODY()

public:
	AModularTeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	friend class UModularTeamCreationComponent;

	UModularTeamDisplayAsset* GetTeamDisplayAsset() const { return TeamDisplayAsset; }

private:
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	void SetTeamDisplayAsset(TObjectPtr<UModularTeamDisplayAsset> NewDisplayAsset);

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamDisplayAsset)
	TObjectPtr<UModularTeamDisplayAsset> TeamDisplayAsset;
};
