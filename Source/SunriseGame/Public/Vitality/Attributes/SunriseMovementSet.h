#pragma once


#include "AbilitySystem/Attributes/SunriseAttributeSet.h"

#include "SunriseMovementSet.generated.h"


/** Movement-only runtime attributes. */
UCLASS(BlueprintType)
class SUNRISEGAME_API USunriseMovementSet : public USunriseAttributeSet
{
	GENERATED_BODY()
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	SUNRISE_ATTRIBUTE_ACCESSORS(USunriseMovementSet, MoveSpeed)
private:
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "Sunrise|Movement", meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MoveSpeed;
};
