#pragma once

#include "Engine/DataAsset.h"

#include "ModularTeamDisplayAsset.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UNiagaraComponent;
class UTexture;

/** Visual parameters that can be applied consistently to every presentation element of a team. */
UCLASS(BlueprintType)
class MODULARGAMEPLAYACTORS_API UModularTeamDisplayAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular|Team")
	TMap<FName, float> ScalarParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular|Team")
	TMap<FName, FLinearColor> ColorParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular|Team")
	TMap<FName, TObjectPtr<UTexture>> TextureParameters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular|Team")
	FText TeamShortName;

	UFUNCTION(BlueprintCallable, Category = "Modular|Team")
	void ApplyToMaterial(UMaterialInstanceDynamic* Material) const;
	UFUNCTION(BlueprintCallable, Category = "Modular|Team")
	void ApplyToMeshComponent(UMeshComponent* MeshComponent) const;
	UFUNCTION(BlueprintCallable, Category = "Modular|Team")
	void ApplyToNiagaraComponent(UNiagaraComponent* NiagaraComponent) const;
	UFUNCTION(BlueprintCallable, Category = "Modular|Team", meta = (DefaultToSelf = "TargetActor"))
	void ApplyToActor(AActor* TargetActor, bool bIncludeChildActors = true) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
