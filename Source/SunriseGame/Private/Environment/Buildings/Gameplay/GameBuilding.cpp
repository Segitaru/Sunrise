// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/GameBuilding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameBuilding)

AGameBuilding::AGameBuilding()
{
}

void AGameBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (bWasLoaded)
	{
		SetBuildingMode();
	}
}

void AGameBuilding::SetPlacementMode()
{
	K2_SetPlacementMode();
}

void AGameBuilding::SetBuildingMode()
{
	const auto CurrentStage = ConstructionStages.Find(CurrentLevel);
	if (!CurrentStage)
	{
		return;
	}

	if (!CurrentStage->bRequireBuild)
	{
		FinishBuilding();
		return;
	}

	LastInteractionTime = GetGameTimeSinceCreation();
	K2_SetBuildingMode();

	if (CurrentStage->bAutoConstruction)
	{
		const float ConstructionTimeByProgress = CurrentStage->ConstructionTime * (1.f - ConstructionProgress);

		if (CurrentStage->VisualStages.IsEmpty())
		{
			GetWorldTimerManager().SetTimer(ConstructionHandle, this, &ThisClass::UpdateBuildProgress, ConstructionTimeByProgress);
		}
		else
		{
			const float TimeBeforeUpdate = ConstructionTimeByProgress / (CurrentStage->VisualStages.Num() - 1);

			GetWorldTimerManager().SetTimer(ConstructionHandle, this, &ThisClass::UpdateBuildProgress, TimeBeforeUpdate);
		}
	}

	UpdateVisualState();
}

void AGameBuilding::UpdateBuildProgress()
{
	const auto CurrentStage = ConstructionStages.Find(CurrentLevel);
	if (!CurrentStage)
	{
		FinishBuilding();

		return;
	}

	const float NewInteractionTime = GetGameTimeSinceCreation();

	const float DeltaInteractionTime = NewInteractionTime - LastInteractionTime;

	LastInteractionTime = NewInteractionTime;

	ConstructionProgress += DeltaInteractionTime / CurrentStage->ConstructionTime;

	K2_UpdateBuildProgress();

	if (ConstructionProgress >= 1.f)
	{
		ConstructionProgress = 0.f;
		++CurrentLevel;

		FinishBuilding();
	}
	else
	{
		if (CurrentStage->bAutoConstruction)
		{
			const float TimeBeforeUpdate = CurrentStage->ConstructionTime / (CurrentStage->VisualStages.Num() - 1);

			GetWorldTimerManager().SetTimer(ConstructionHandle, this, &ThisClass::UpdateBuildProgress, TimeBeforeUpdate);
		}

		UpdateVisualState();
	}

	OnUpdateBuildingInfo.Broadcast(this);
}

void AGameBuilding::UpdateVisualState()
{
	K2_UpdateVisualState();
}

void AGameBuilding::FinishBuilding()
{
	OnFinishBuilding();

	UpdateVisualState();
}

TMap<FGameplayAttribute, float> AGameBuilding::GetVitalityValues() const
{
	return TMap<FGameplayAttribute, float>();
}

void AGameBuilding::GetCurrentState(float& Level, float& Progress) const
{
	Level = CurrentLevel;
	Progress = ConstructionProgress;
}
void AGameBuilding::SetCurrentState(const float& Level, const float& Progress)
{
	bWasLoaded = true;
	CurrentLevel = Level;
	ConstructionProgress = Progress;
}
