// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagStack.h"
#include "Teams/System/ModularTeamAgentInterface.h"

#include "ModularPlayerState.generated.h"

#define UE_API MODULARGAMEPLAYACTORS_API

DECLARE_MULTICAST_DELEGATE(FOnPawnDataReady);

namespace EEndPlayReason
{
	enum Type : int;
}

class UModularPawnData;
class UExperienceDefinition;

/** Minimal class that supports extension by game feature plugins */
UCLASS(MinimalAPI, Blueprintable)
class AModularPlayerState : public APlayerState, public IAbilitySystemInterface, public IModularTeamAgentInterface
{
	GENERATED_BODY()

public:
	UE_API AModularPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Modular|PlayerState")
	UModularAbilitySystemComponent* GetModularAbilitySystemComponent() const { return AbilitySystemComponent; }
	UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	template <class T>
	const T* GetPawnData() const
	{
		return Cast<T>(PawnData);
	}

	UE_API void SetPawnData(const UModularPawnData* InPawnData);

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Teams)
	UE_API void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Teams)
	UE_API void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category = Teams)
	UE_API int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category = Teams)
	UE_API bool HasStatTag(FGameplayTag Tag) const;

	UE_API void CallOrRegister_OnPawnDataReady(FOnPawnDataReady::FDelegate&& Delegate);

	FOnPawnDataReady OnPawnDataReady;

private:
	UE_API void OnExperienceLoaded(const UExperienceDefinition* CurrentExperience);


protected:
	UFUNCTION()
	UE_API void OnRep_PawnData();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UModularPawnData> PawnData;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	// Global ASC for player
	UPROPERTY(VisibleAnywhere, Category = "Modular|PlayerState")
	TObjectPtr<UModularAbilitySystemComponent> AbilitySystemComponent;

public:
	static UE_API const inline FName NAME_ModularAbilityReady = TEXT("AbilityReady");

	//~ Begin AActor interface
	UE_API virtual void PostInitializeComponents() override;
	UE_API virtual void PreInitializeComponents() override;
	UE_API virtual void ClientInitialize(AController* C) override;
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UE_API virtual void Reset() override;
	//~ End AActor interface

	UE_API const FString& GetAccountId() const;

#pragma region IModularTeamAgentInterface
	/** Returns the Squad ID of the squad the player belongs to. */
	UFUNCTION(BlueprintCallable)
	int32 GetSquadId() const { return MySquadID; }

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
	int32 GetTeamId() const { return GenericTeamIdToInteger(MyTeamID); }

	UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	UE_API virtual FGenericTeamId GetGenericTeamId() const override;
	UE_API virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;

	UE_API void SetSquadID(int32 NewSquadID);

	UPROPERTY()
	FOnTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	UPROPERTY(ReplicatedUsing = OnRep_MySquadID)
	int32 MySquadID;

	UFUNCTION()
	UE_API void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	UE_API void OnRep_MySquadID();
#pragma endregion IModularTeamAgentInterface

protected:
	UPROPERTY(Replicated)
	FString PlayerAccountId = "";

	//~ Begin APlayerState interface
	UE_API virtual void CopyProperties(APlayerState* PlayerState) override;
	//~ End APlayerState interface
};

#undef UE_API
