#include "Weapons/Actors/SunriseAreaIndicator.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ASunriseAreaIndicator::ASunriseAreaIndicator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	VisualSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
	SetRootComponent(VisualSphere);
	VisualSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
		VisualSphere->SetStaticMesh(SphereMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> IndicatorMaterial(
		TEXT("/Game/Sunrise/Effects/Indicators/M_Indicator.M_Indicator"));
	if (IndicatorMaterial.Succeeded())
		VisualSphere->SetMaterial(0, IndicatorMaterial.Object);
	SetActorScale3D(FVector(0.01f));
	InitialLifeSpan = 0.45f;
}

void ASunriseAreaIndicator::InitializeIndicator(float InRadius, float InLifetime)
{
	MaxRadius = FMath::Max(1.0f, InRadius / 50.0f);
	if (InLifetime > 0.0f)
		SetLifeSpan(InLifetime);
}

void ASunriseAreaIndicator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	SetActorScale3D(FVector(FMath::Lerp(0.01f, MaxRadius, FMath::Clamp(Elapsed / ExpansionDuration, 0.0f, 1.0f))));
}

ASunriseAreaDecalIndicator::ASunriseAreaDecalIndicator()
{
	bReplicates = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AreaDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("AreaDecal"));
	AreaDecal->SetupAttachment(SceneRoot);
	AreaDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMaterial(
		TEXT("/Game/Sunrise/Effects/Decals/M_Selection_Decal.M_Selection_Decal"));
	if (DecalMaterial.Succeeded())
	{
		AreaDecal->SetDecalMaterial(DecalMaterial.Object);
	}
	ApplyVisualParameters();
}

void ASunriseAreaDecalIndicator::BeginPlay()
{
	Super::BeginPlay();
	DynamicMaterial = AreaDecal ? AreaDecal->CreateDynamicMaterialInstance() : nullptr;
	ApplyVisualParameters();
}

void ASunriseAreaDecalIndicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Radius);
	DOREPLIFETIME(ThisClass, IndicatorColor);
}

void ASunriseAreaDecalIndicator::InitializeIndicator(float InRadius, float InLifetime, const FLinearColor& InColor)
{
	Radius = FMath::Max(1.0f, InRadius);
	IndicatorColor = InColor;
	ApplyVisualParameters();
	SetLifeSpan(FMath::Max(0.1f, InLifetime));
}

void ASunriseAreaDecalIndicator::OnRep_VisualParameters()
{
	ApplyVisualParameters();
}

void ASunriseAreaDecalIndicator::ApplyVisualParameters()
{
	if (AreaDecal)
	{
		const float Diameter = Radius * 2.0f;
		AreaDecal->DecalSize = FVector(256.0f, Diameter, Diameter);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), IndicatorColor);
		}
		AreaDecal->MarkRenderStateDirty();
	}
}
