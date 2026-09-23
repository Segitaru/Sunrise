#pragma once

#include "Components/PlayerStateComponent.h"
#include "GameModes/Survival/Types/SurvivalTypes.h"

#include "SurvivalBuildComponent.generated.h"

class ASunriseUnit;
class ASurvivalBuilding;
class FLifetimeProperty;

USTRUCT(BlueprintType)
struct FSurvivalBuildOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ASurvivalBuilding> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSurvivalResourceAmounts Cost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 MaxPerPlayer = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1.0", Units = "cm"))
	FVector PlacementExtent = FVector(150.0f, 150.0f, 150.0f);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivalBuildResult, ESurvivalBuildFailure, Failure);

/** Client intent entry point and authoritative building transaction. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalBuildComponent : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	USurvivalBuildComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Survival|Build")
	const TArray<FSurvivalBuildOption>& GetBuildOptions() const { return BuildOptions; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Survival|Build")
	void ServerRequestBuild(FGameplayTag BuildingId, FTransform Transform, ASunriseUnit* Builder);

	UFUNCTION(BlueprintPure, Category = "Survival|Build")
	ESurvivalBuildFailure GetLastBuildFailure() const { return LastBuildFailure; }

	UPROPERTY(BlueprintAssignable, Category = "Survival|Build")
	FOnSurvivalBuildResult OnBuildResult;

protected:
	const FSurvivalBuildOption* FindOption(FGameplayTag BuildingId) const;
	ESurvivalBuildFailure ValidateRequest(const FSurvivalBuildOption& Option, const FTransform& Transform, ASunriseUnit* Builder) const;
	void SetBuildResult(ESurvivalBuildFailure Failure);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Build")
	TArray<FSurvivalBuildOption> BuildOptions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Survival|Build", meta = (ClampMin = "1.0", Units = "cm"))
	float MaxBuildDistance = 1000.0f;

private:
	UFUNCTION()
	void OnRep_LastBuildFailure();

	UPROPERTY(ReplicatedUsing = OnRep_LastBuildFailure)
	ESurvivalBuildFailure LastBuildFailure = ESurvivalBuildFailure::None;
};
