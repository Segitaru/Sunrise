#pragma once

#include "Engine/DataAsset.h"

#include "TFTeamDisplayAsset.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UNiagaraComponent;
class UTexture;

/** Visual parameters that can be applied consistently to every presentation element of a team. */
UCLASS(BlueprintType)
class TEAMFRAMEWORKCORE_API UTFTeamDisplayAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	TMap<FName, float> ScalarParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	TMap<FName, FLinearColor> ColorParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	TMap<FName, TObjectPtr<UTexture>> TextureParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	FText TeamShortName;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team")
	void ApplyToMaterial(UMaterialInstanceDynamic* Material) const;
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team")
	void ApplyToMeshComponent(UMeshComponent* MeshComponent) const;
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team")
	void ApplyToNiagaraComponent(UNiagaraComponent* NiagaraComponent) const;
	UFUNCTION(BlueprintCallable, Category = "Sunrise|Team", meta = (DefaultToSelf = "TargetActor"))
	void ApplyToActor(AActor* TargetActor, bool bIncludeChildActors = true) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
