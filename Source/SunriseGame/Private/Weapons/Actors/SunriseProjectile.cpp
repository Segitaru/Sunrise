#include "Weapons/Actors/SunriseProjectile.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
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
	CollisionRoot->SetCanEverAffectNavigation(false);
	CollisionRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionRoot->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionRoot->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	VisualSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
	VisualSphere->SetupAttachment(CollisionRoot);
	VisualSphere->SetCanEverAffectNavigation(false);
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

	const FVector Start = GetActorLocation();
	const FVector ToTarget = Target->GetActorLocation() - Start;
	Velocity = ToTarget.GetSafeNormal() * Speed;
	const FVector Delta = Velocity * FMath::Max(0.0f, DeltaSeconds);
	const FVector End = Start + Delta;

	const float ProjectileRadius = CollisionRoot ? CollisionRoot->GetScaledBoxExtent().GetMax() : 0.0f;
	const float TargetRadius = Target->GetCapsuleComponent() ? Target->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
	const FVector ClosestPoint = FMath::ClosestPointOnSegment(Target->GetActorLocation(), Start, End);
	if (FVector::DistSquared(ClosestPoint, Target->GetActorLocation()) <= FMath::Square(ProjectileRadius + TargetRadius))
	{
		SourceUnit->DealWeaponDamage(Target, Damage);
		Destroy();
		return;
	}

	FHitResult Hit;
	SetActorLocation(End, true, &Hit);
	if (Hit.bBlockingHit)
	{
		Destroy();
	}
}