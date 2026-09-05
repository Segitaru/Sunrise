// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GamePawnRosterSubsystem.generated.h"

class UGameRosterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamSidesSwitched);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundChanged);

USTRUCT(BlueprintType)
struct FPlayerWithPayload
{
	GENERATED_BODY()

	FPlayerWithPayload() {}
	FPlayerWithPayload(const FUniqueNetIdRepl& NewNetId, const FString& NewAuthId)
	{
		NetId = NewNetId;
		AuthId = NewAuthId;
	}
	FPlayerWithPayload(
		const FUniqueNetIdRepl& NewNetId, const FString& NewAuthId, const TSubclassOf<UObject> NewPayloadClass)
	{
		NetId = NewNetId;
		AuthId = NewAuthId;
		PayloadClass = NewPayloadClass;
	}

	UPROPERTY(BlueprintReadWrite)
	FUniqueNetIdRepl NetId;
	UPROPERTY(BlueprintReadWrite)
	FString AuthId;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UObject> PayloadClass;

	bool operator==(const FPlayerWithPayload& TargetPayload) const
	{
		return AuthId == TargetPayload.AuthId || NetId == TargetPayload.NetId;
	}
};

UCLASS()
class SUNRISEGAME_API UGamePawnRosterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UGamePawnRosterSubsystem();

	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface

	UPROPERTY(BlueprintReadWrite)
	bool bIsPlayWorld = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPlayerWithPayload> PlayersWithPayload;
};
