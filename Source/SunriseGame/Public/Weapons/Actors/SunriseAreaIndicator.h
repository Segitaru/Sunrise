#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SunriseAreaIndicator.generated.h"

class UStaticMeshComponent;

UCLASS()
class SUNRISEGAME_API ASunriseAreaIndicator : public AActor
{
	GENERATED_BODY()
public:
	ASunriseAreaIndicator();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeIndicator(float InRadius);

private:
	UPROPERTY(VisibleAnywhere, Category = "Sunrise|Weapon")
	TObjectPtr<UStaticMeshComponent> VisualSphere;
	float MaxRadius = 1.0f;
	float Elapsed = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float ExpansionDuration = 0.35f;
};