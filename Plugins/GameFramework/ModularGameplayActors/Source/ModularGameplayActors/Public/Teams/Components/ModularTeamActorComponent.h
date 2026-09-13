#pragma once

#include "Components/ActorComponent.h"
#include "Teams/System/ModularTeamAgentInterface.h"

#include "ModularTeamActorComponent.generated.h"

/** Replicated, reusable source of team identity for any actor. */
UCLASS(ClassGroup = (Modular), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MODULARGAMEPLAYACTORS_API UModularTeamActorComponent : public UActorComponent, public IModularTeamAgentInterface
{
	GENERATED_BODY()

public:
	UModularTeamActorComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Modular|Team")
	int32 GetTeamId() const;

	/** Changes the team on authority. INDEX_NONE represents no team. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular|Team")
	bool SetTeamId(int32 NewTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }

	UPROPERTY(BlueprintAssignable, Category = "Modular|Team")
	FOnTeamIndexChangedDelegate OnTeamChanged;

private:
	UFUNCTION()
	void OnRep_TeamId(FGenericTeamId PreviousTeamId);

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TeamId, Category = "Modular|Team")
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;
};
