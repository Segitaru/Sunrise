#pragma once

#include "Components/ActorComponent.h"
#include "System/TFTeamAgentInterface.h"

#include "TFTeamActorComponent.generated.h"

/** Replicated, reusable source of team identity for any actor. */
UCLASS(ClassGroup = (Sunrise), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TEAMFRAMEWORKCORE_API UTFTeamActorComponent : public UActorComponent, public ITFTeamAgentInterface
{
	GENERATED_BODY()

public:
	UTFTeamActorComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	int32 GetTeamId() const { return TFTeamIdToInteger(TeamId); }

	/** Changes the team on authority. INDEX_NONE represents no team. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	bool SetTeamId(int32 NewTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual FOnTFTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Team")
	FOnTFTeamIndexChangedDelegate OnTeamChanged;

private:
	UFUNCTION()
	void OnRep_TeamId(FGenericTeamId PreviousTeamId);

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TeamId, Category = "Sunrise|Team")
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;
};
