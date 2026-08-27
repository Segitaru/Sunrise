#include "Weapons/Actors/SunriseProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Units/SunriseUnit.h"

ASunriseProjectile::ASunriseProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	CollisionRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionRoot"));
	SetRootComponent(CollisionRoot);
	CollisionRoot->SetBoxExtent(FVector(12.0f));
	CollisionRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionRoot->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionRoot->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	VisualSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
	VisualSphere->SetupAttachment(CollisionRoot);
	VisualSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualSphere->SetStaticMesh(SphereMesh.Object);
		VisualSphere->SetWorldScale3D(FVector(0.18f));
	}
	InitialLifeSpan = 4.0f;
}

void ASunriseProjectile::InitializeProjectile(ASunriseUnit* InSource, ASunriseUnit* InTarget, float InDamage)
{
	SourceUnit = InSource;
	TargetUnit = InTarget;
	Damage = FMath::Max(0.0f, InDamage);
	Velocity = GetActorForwardVector() * Speed;
}

void ASunriseProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !SourceUnit.IsValid() || !TargetUnit.IsValid())
	{
		return;
	}
	ASunriseUnit* Target = TargetUnit.Get();
	if (!Target->IsAlive())
	{
		Destroy();
		return;
	}

	// Recompute the flight direction every frame so moving targets are still hit.
	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float HitDistance = FMath::Max(50.0f, Speed * DeltaSeconds);
	if (ToTarget.SizeSquared2D() <= FMath::Square(HitDistance))
	{
		SourceUnit->DealWeaponDamage(Target, Damage);
		Destroy();
		return;
	}
	Velocity = ToTarget.GetSafeNormal() * Speed;

	FHitResult Hit;
	AddActorWorldOffset(Velocity * DeltaSeconds, true, &Hit);
	if (Hit.bBlockingHit && Hit.GetActor() != Target)
	{
		Destroy();
	}
}
