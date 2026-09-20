#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SunriseAreaIndicator.generated.h"

class UDecalComponent;
class USceneComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class SUNRISEGAME_API ASunriseAreaIndicator : public AActor
{
	GENERATED_BODY()
public:
	ASunriseAreaIndicator();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeIndicator(float InRadius, float InLifetime = -1.0f);

private:
	UPROPERTY(VisibleAnywhere, Category = "Sunrise|Weapon")
	TObjectPtr<UStaticMeshComponent> VisualSphere;
	float MaxRadius = 1.0f;
	float Elapsed = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Sunrise|Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float ExpansionDuration = 0.35f;
};

UCLASS()
class SUNRISEGAME_API ASunriseAreaDecalIndicator : public AActor
{
	GENERATED_BODY()

public:
	ASunriseAreaDecalIndicator();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeIndicator(float InRadius, float InLifetime, const FLinearColor& InColor);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_VisualParameters();
	void ApplyVisualParameters();

	UPROPERTY(VisibleAnywhere, Category = "Sunrise|Ability")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Sunrise|Ability")
	TObjectPtr<UDecalComponent> AreaDecal;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_VisualParameters)
	float Radius = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualParameters)
	FLinearColor IndicatorColor = FLinearColor::White;
};
