#include "Weapons/Actors/SunriseAreaIndicator.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
	{
		VisualSphere->SetStaticMesh(SphereMesh.Object);
	}
	SetActorScale3D(FVector(0.01f));
	InitialLifeSpan = 0.45f;
}

void ASunriseAreaIndicator::InitializeIndicator(float InRadius)
{
	MaxRadius = FMath::Max(1.0f, InRadius / 50.0f);
}

void ASunriseAreaIndicator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	SetActorScale3D(FVector(FMath::Lerp(0.01f, MaxRadius, FMath::Clamp(Elapsed / ExpansionDuration, 0.0f, 1.0f))));
}