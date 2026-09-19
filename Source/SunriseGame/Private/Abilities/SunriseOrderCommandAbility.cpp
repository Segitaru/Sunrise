#include "Abilities/SunriseOrderCommandAbility.h"

#include "Abilities/SunriseSelectionAbility.h"
#include "Abilities/SunriseUnitOrderAbility.h"
#include "AbilitySystemComponent.h"
#include "ControllableEntities/ControllableEntitiesManager.h"
#include "GameModes/Overload/Interfaces/OverloadHackable.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Player/SunrisePlayerController.h"
#include "Units/SunrisePawn.h"
#include "Units/SunriseUnit.h"

USunriseOrderCommandAbility::USunriseOrderCommandAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USunriseOrderCommandAbility::PlayOrderFeedback(const FVector& Location, FGameplayTag OrderTag)
{
	if (OrderTag != SunriseOrders::Move && OrderTag != SunriseOrders::Target && OrderTag != SunriseOrders::Hack)
	{
		return;
	}

	static TWeakObjectPtr<UNiagaraSystem> CursorEffect;
	UNiagaraSystem* Effect = CursorEffect.Get();
	if (!Effect)
	{
		Effect = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Sunrise/Effects/Cursor/FX_Cursor.FX_Cursor"));
		CursorEffect = Effect;
	}
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (Effect && Avatar && Avatar->GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Avatar->GetWorld(), Effect, Location);
	}
}
void USunriseOrderCommandAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ASunrisePawn* Pawn = ActorInfo ? Cast<ASunrisePawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	ASunrisePlayerController* PC = Pawn ? Cast<ASunrisePlayerController>(Pawn->GetController()) : nullptr;
	UControllableEntitiesManager* Manager = UControllableEntitiesManager::FindControllableEntitiesManager(PC);
	TArray<ASunriseUnit*> Units;

	for (AActor* CurrentEntity : Manager->GetSelectedEntities())
	{
		ASunriseUnit* const Unit = Cast<ASunriseUnit>(CurrentEntity);
		if (IsValid(Unit) && Unit->IsAlive() && Manager && Manager->CanControlEntity(Unit))
		{
			Units.Add(Unit);
		}
	}

	FHitResult Hit;
	AActor* Target = nullptr;
	FGameplayTag OrderTag = SunriseOrders::Move;
	if (PC && PC->GetHitResultUnderCursorByChannel(TraceTypeQuery1, true, Hit))
	{
		Target = Hit.GetActor();
		if (Cast<ASunriseUnit>(Target))
		{
			OrderTag = SunriseOrders::Target;
		}
		else if (IsValid(Target) && Target->Implements<UOverloadHackable>())
		{
			OrderTag = SunriseOrders::Hack;
		}
	}

	if (Pawn && !Units.IsEmpty())
	{
		if (USunriseUnitOrderAbility::SendOrderEvent(Pawn, OrderTag, Units, nullptr, Hit.ImpactPoint, Target))
		{
			PlayOrderFeedback(Hit.ImpactPoint, OrderTag);
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
