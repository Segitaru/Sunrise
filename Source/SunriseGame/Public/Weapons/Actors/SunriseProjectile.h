#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SunriseProjectile.generated.h"

class ASunriseUnit;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class SUNRISEGAME_API ASunriseProjectile : public AActor
{
	GENERATED_BODY()
public:
	ASunriseProjectile();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeProjectile(ASunriseUnit* InSource, ASunriseUnit* InTarget, float InDamage);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Projectile")
	TObjectPtr<UBoxComponent> CollisionRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Projectile")
	TObjectPtr<UStaticMeshComponent> VisualSphere;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Projectile", meta = (ClampMin = "1.0", Units = "cm/s"))
	float Speed = 1800.0f;

private:
	TWeakObjectPtr<ASunriseUnit> SourceUnit;
	TWeakObjectPtr<ASunriseUnit> TargetUnit;
	float Damage = 0.0f;
	FVector Velocity = FVector::ZeroVector;
};