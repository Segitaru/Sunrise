#include "GameModes/Survival/Actors/SurvivalBuilding.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/Components/SurvivalProductionComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalBuilding)

ASurvivalBuilding::ASurvivalBuilding()
{
	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(GetPlacementComponent());
	BuildingMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BuildingMesh->SetCanEverAffectNavigation(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BuildingMesh->SetStaticMesh(CubeMesh.Object);
	}
	ConstructionStages.Add(0.0f, FConstructionParameters());
}

void ASurvivalBuilding::BeginPlay()
{
	Super::BeginPlay();
	VitalityComponent->OnVitalityStateChanged.AddDynamic(this, &ThisClass::HandleBuildingVitalityStateChanged);
	if (HasAuthority())
	{
		if (USurvivalEconomyComponent* Economy = USurvivalEconomyComponent::Find(GetOwner()))
		{
			Economy->AddPopulationCapacity(PopulationCapacity);
			bEconomyCapacityApplied = true;
		}
		if (USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this))
		{
			Match->RegisterBuilding(this);
		}
	}
}

void ASurvivalBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	VitalityComponent->OnVitalityStateChanged.RemoveDynamic(this, &ThisClass::HandleBuildingVitalityStateChanged);
	if (HasAuthority())
	{
		if (USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this))
		{
			Match->UnregisterBuilding(this);
		}
		if (bEconomyCapacityApplied)
		{
			if (USurvivalEconomyComponent* Economy = USurvivalEconomyComponent::Find(GetOwner()))
			{
				Economy->AddPopulationCapacity(-PopulationCapacity);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}
void ASurvivalBuilding::HandleBuildingVitalityStateChanged(AActor*, EVitalityState, EVitalityState NewState)
{
	if (!HasAuthority() || NewState == EVitalityState::Healthy)
	{
		return;
	}
	if (USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this))
	{
		Match->UnregisterBuilding(this);
	}
	if (bEconomyCapacityApplied)
	{
		if (USurvivalEconomyComponent* Economy = USurvivalEconomyComponent::Find(GetOwner()))
		{
			Economy->AddPopulationCapacity(-PopulationCapacity);
		}
		bEconomyCapacityApplied = false;
	}
}


void ASurvivalBuilding::SetLocallySelected(bool bSelected)
{
	if (BuildingMesh)
	{
		BuildingMesh->SetRenderCustomDepth(bSelected);
		BuildingMesh->SetCustomDepthStencilValue(bSelected ? 1 : 0);
	}
}

void ASurvivalBuilding::ConfigureBasicShape(
	const FVector& RelativeScale, ESurvivalBuildingRole InRole, float MaxHealth, int32 InPopulationCapacity)
{
	BuildingMesh->SetRelativeScale3D(RelativeScale);
	BuildingRole = InRole;
	InitialMaxHealth = MaxHealth;
	PopulationCapacity = InPopulationCapacity;
}

ASurvivalMainBase::ASurvivalMainBase()
{
	ProductionComponent = CreateDefaultSubobject<USurvivalProductionComponent>(TEXT("SurvivalProduction"));
	ConfigureBasicShape(FVector(4.0f, 4.0f, 2.2f), ESurvivalBuildingRole::MainBase, 2500.0f, 10);
	ConstructionCost.Wood = 400.0f;
	ConstructionCost.Stone = 250.0f;
	ConstructionCost.Metal = 100.0f;
}

ASurvivalBarracks::ASurvivalBarracks()
{
	ProductionComponent = CreateDefaultSubobject<USurvivalProductionComponent>(TEXT("SurvivalProduction"));
	ConfigureBasicShape(FVector(3.0f, 2.0f, 1.5f), ESurvivalBuildingRole::Barracks, 1200.0f, 0);
	ConstructionCost.Wood = 180.0f;
	ConstructionCost.Stone = 80.0f;
}

ASurvivalHouse::ASurvivalHouse()
{
	ConfigureBasicShape(FVector(1.5f, 1.5f, 1.8f), ESurvivalBuildingRole::Housing, 650.0f, 10);
	ConstructionCost.Wood = 100.0f;
}

ASurvivalDefenseBuilding::ASurvivalDefenseBuilding()
{
	ConfigureBasicShape(FVector(1.2f, 1.2f, 3.0f), ESurvivalBuildingRole::Defense, 900.0f, 0);
	ConstructionCost.Wood = 120.0f;
	ConstructionCost.Stone = 140.0f;
}
