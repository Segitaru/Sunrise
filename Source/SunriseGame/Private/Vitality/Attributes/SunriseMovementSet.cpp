#include "Vitality/Attributes/SunriseMovementSet.h"

#include "Net/UnrealNetwork.h"

void USunriseMovementSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}
void USunriseMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USunriseMovementSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}
void USunriseMovementSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USunriseMovementSet, MoveSpeed, OldValue);
}
