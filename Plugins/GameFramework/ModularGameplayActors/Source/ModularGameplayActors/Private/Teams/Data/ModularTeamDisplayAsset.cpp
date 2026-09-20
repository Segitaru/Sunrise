// Copyright Epic Games, Inc. All Rights Reserved.

#include "Teams/Data/ModularTeamDisplayAsset.h"

#include "Components/MeshComponent.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Teams/System/ModularTeamSubsystem.h"
#include "UObject/UObjectIterator.h"

void UModularTeamDisplayAsset::ApplyToMaterial(UMaterialInstanceDynamic* Material) const
{
	if (Material)
	{
		for (const auto& KVP : ScalarParameters)
		{
			Material->SetScalarParameterValue(KVP.Key, KVP.Value);
		}

		for (const auto& KVP : ColorParameters)
		{
			Material->SetVectorParameterValue(KVP.Key, FVector(KVP.Value));
		}

		for (const auto& KVP : TextureParameters)
		{
			Material->SetTextureParameterValue(KVP.Key, KVP.Value);
		}
	}
}

void UModularTeamDisplayAsset::ApplyToMeshComponent(UMeshComponent* MeshComponent) const
{
	if (MeshComponent)
	{
		for (const auto& KVP : ScalarParameters)
		{
			MeshComponent->SetScalarParameterValueOnMaterials(KVP.Key, KVP.Value);
		}

		for (const auto& KVP : ColorParameters)
		{
			MeshComponent->SetVectorParameterValueOnMaterials(KVP.Key, FVector(KVP.Value));
		}

		const TArray<UMaterialInterface*> MaterialInterfaces = MeshComponent->GetMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialInterfaces.Num(); ++MaterialIndex)
		{
			if (UMaterialInterface* MaterialInterface = MaterialInterfaces[MaterialIndex])
			{
				UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
				if (!DynamicMaterial)
				{
					DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
				}

				for (const auto& KVP : TextureParameters)
				{
					DynamicMaterial->SetTextureParameterValue(KVP.Key, KVP.Value);
				}
			}
		}
	}
}

void UModularTeamDisplayAsset::ApplyToNiagaraComponent(UNiagaraComponent* NiagaraComponent) const
{
	if (NiagaraComponent)
	{
		for (const auto& KVP : ScalarParameters)
		{
			NiagaraComponent->SetVariableFloat(KVP.Key, KVP.Value);
		}

		for (const auto& KVP : ColorParameters)
		{
			NiagaraComponent->SetVariableLinearColor(KVP.Key, KVP.Value);
		}

		for (const auto& KVP : TextureParameters)
		{
			UTexture* Texture = KVP.Value;
			NiagaraComponent->SetVariableTexture(KVP.Key, Texture);
		}
	}
}

void UModularTeamDisplayAsset::ApplyToActor(AActor* TargetActor, bool bIncludeChildActors) const
{
	if (TargetActor != nullptr)
	{
		TargetActor->ForEachComponent(bIncludeChildActors,
			[this](UActorComponent* InComponent)
			{
				if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(InComponent))
				{
					ApplyToMeshComponent(MeshComponent);
				}
				else if (UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(InComponent))
				{
					ApplyToNiagaraComponent(NiagaraComponent);
				}
			});
	}
}

#if WITH_EDITOR
void UModularTeamDisplayAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (UModularTeamSubsystem* TeamSubsystem : TObjectRange<UModularTeamSubsystem>())
	{
		TeamSubsystem->NotifyTeamDisplayAssetModified(this);
	}
}
#endif
