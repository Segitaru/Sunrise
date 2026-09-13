// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Info.h"
#include "System/GameplayTagStack.h"

#include "ModularTeamInfoBase.generated.h"

namespace EEndPlayReason
{
	enum Type : int;
}

class UModularTeamCreationComponent;
class UModularTeamSubsystem;
class UObject;
struct FFrame;

UCLASS(Abstract)
class AModularTeamInfoBase : public AInfo
{
	GENERATED_BODY()

public:
	AModularTeamInfoBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	int32 GetTeamId() const { return TeamId; }

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

protected:
	virtual void RegisterWithTeamSubsystem(UModularTeamSubsystem* Subsystem);
	void TryRegisterWithTeamSubsystem();

private:
	void SetTeamId(int32 NewTeamId);

	UFUNCTION()
	void OnRep_TeamId();

public:
	friend UModularTeamCreationComponent;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer TeamTags;

private:
	UPROPERTY(ReplicatedUsing = OnRep_TeamId)
	int32 TeamId;
};
