#include "Environment/Resources/SunriseResourceNode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SunriseResourceNode)

ASunriseResourceNode::ASunriseResourceNode()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	SetRootComponent(ResourceMesh);
	ResourceMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	ResourceMesh->SetCanEverAffectNavigation(true);
	ResourceMesh->SetRelativeScale3D(FVector(1.3f, 1.3f, 1.8f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ResourceMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void ASunriseResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, RemainingAmount);
}

int32 ASunriseResourceNode::Extract(int32 RequestedAmount)
{
	if (!HasAuthority() || RequestedAmount <= 0 || RemainingAmount <= 0)
	{
		return 0;
	}
	const int32 Extracted = FMath::Min(RequestedAmount, RemainingAmount);
	RemainingAmount -= Extracted;
	return Extracted;
}
