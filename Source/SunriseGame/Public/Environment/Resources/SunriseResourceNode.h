#pragma once

#include "GameFramework/Actor.h"
#include "GameModes/Survival/Types/SurvivalResourceTypes.h"

#include "SunriseResourceNode.generated.h"

class UStaticMeshComponent;
class FLifetimeProperty;

/** Finite server-owned resource source. */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseResourceNode : public AActor
{
	GENERATED_BODY()

public:
	ASunriseResourceNode();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Survival|Resource")
	ESurvivalResourceType GetResourceType() const { return ResourceType; }

	UFUNCTION(BlueprintPure, Category = "Survival|Resource")
	int32 GetRemainingAmount() const { return RemainingAmount; }

	int32 Extract(int32 RequestedAmount);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> ResourceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival|Resource")
	ESurvivalResourceType ResourceType = ESurvivalResourceType::Wood;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Survival|Resource", meta = (ClampMin = "0"))
	int32 RemainingAmount = 1500;
};
