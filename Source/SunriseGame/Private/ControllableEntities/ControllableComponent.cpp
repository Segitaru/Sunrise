#include "ControllableEntities/ControllableComponent.h"

#include "ControllableEntities/IControllableEntity.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UControllableComponent::UControllableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UControllableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UControllableComponent, bPlayerControllable);
	DOREPLIFETIME(UControllableComponent, EntityDefinition);
}

void UControllableComponent::SetPlayerControllable(bool bNewControllable)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		bPlayerControllable = bNewControllable;
	}
}

bool UControllableComponent::CanBeControlledBy(const AController* Controller) const
{
	if (!bPlayerControllable || !Controller || !GetOwner())
	{
		return false;
	}
	IIControllableEntity* Entity = Cast<IIControllableEntity>(GetOwner());
	if (!Entity)
	{
		return false;
	}
	const TScriptInterface<IIControllableEntity> Agent = Entity->GetControllingAgent();
	return Agent.GetObject() == Controller || Agent.GetObject() == Controller->GetPlayerState<APlayerState>();
}

void UControllableComponent::SetEntityDefinition(UControllableEntityDefinition* NewDefinition)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		EntityDefinition = NewDefinition;
	}
}
