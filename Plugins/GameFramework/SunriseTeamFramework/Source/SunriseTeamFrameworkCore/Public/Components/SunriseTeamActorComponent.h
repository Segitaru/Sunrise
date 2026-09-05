#pragma once

#include "Components/ActorComponent.h"
#include "System/SunriseTeamAgentInterface.h"

#include "SunriseTeamActorComponent.generated.h"

/** Replicated, reusable source of team identity for any actor. */
UCLASS(ClassGroup = (Sunrise), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SUNRISETEAMFRAMEWORKCORE_API USunriseTeamActorComponent : public UActorComponent, public ISunriseTeamAgentInterface
{
	GENERATED_BODY()

public:
	USunriseTeamActorComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Team")
	int32 GetTeamId() const { return SunriseTeamIdToInteger(TeamId); }

	/** Changes the team on authority. INDEX_NONE represents no team. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Team")
	bool SetTeamId(int32 NewTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
	virtual FOnTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChanged; }

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Team")
	FOnTeamIndexChangedDelegate OnTeamChanged;

private:
	UFUNCTION()
	void OnRep_TeamId(FGenericTeamId PreviousTeamId);

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_TeamId, Category = "Sunrise|Team")
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;
};
