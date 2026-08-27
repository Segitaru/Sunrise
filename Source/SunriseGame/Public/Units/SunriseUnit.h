// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "ControllableEntities/IControllableEntity.h"
#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameFramework/Character.h"
#include "System/TFTeamAgentInterface.h"
#include "Units/SunriseUnitInterfaces.h"
#include "Units/SunriseUnitTypes.h"
#include "Vitality/VitalityComponent.h"

#include "SunriseUnit.generated.h"

class UDecalComponent;
class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;
class USphereComponent;
class UAbilitySystemComponent;
class USunriseHealthSet;
class USunriseCombatSet;
class USunriseMovementSet;
class UVitalityComponent;
class UControllableComponent;
class UGameplayEffect;
class USunriseWeapon;
class USunriseHeroSquadAbility;
class UTFTeamActorComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitMoveCompletedDelegate, ASunriseUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSunriseUnitHealthChanged, ASunriseUnit*, Unit, float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSunriseUnitDied, ASunriseUnit*, Unit);

/**
 * Reusable RTS combat unit. Movement, orders, aggro, combat, healing and death are native;
 * Blueprint children only need to supply presentation assets and optional event effects.
 */
UCLASS(Blueprintable)
class SUNRISEGAME_API ASunriseUnit : public ACharacter,
									 public IAbilitySystemInterface,
									 public ITFTeamAgentInterface,
									 public IIControllableEntity,
									 public ISunriseSelectable,
									 public ISunriseOrderReceiver
{
	GENERATED_BODY()

public:
	ASunriseUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(
		float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ISunriseSelectable
	virtual bool CanBeSelectedBy_Implementation(const APlayerController* InController) const override;
	virtual void SetSunriseSelected_Implementation(bool bSelected) override;

	// ISunriseOrderReceiver
	virtual void IssueMoveOrder_Implementation(const FVector& Destination) override;
	virtual void IssueTargetOrder_Implementation(AActor* TargetActor) override;
	virtual void StopOrder_Implementation() override;

	/** Internal autonomous navigation command. It never overrides an active player order. Returns true when the destination was accepted. */
	bool IssueAutonomousMoveOrder(const FVector& Destination);
	/** Forces this autonomous unit to focus a hostile target without overriding an active player order. */
	bool ApplyFocusTarget(ASunriseUnit* Target, float Duration);
	UFUNCTION(BlueprintPure, Category = "Sunrise|Orders")
	bool HasActivePlayerOrder() const { return bPlayerOrderActive; }
	ASunriseUnit* GetActionTarget() const { return ActionTarget; }

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Orders")
	void StopMoving();

	/** Suspends autonomous combat decisions while an external component owns movement/action intent. */
	void SetExternalInteractionActive(bool bActive);
	bool IsExternalInteractionActive() const { return bExternalInteractionActive; }

	/** Compatibility wrappers used by the original Epic Sunrise Blueprint. */
	void UnitSelected();
	void UnitDeselected();
	void Interact(ASunriseUnit* Interactor);
	void MoveToLocation(const FVector& Location, bool bInteract = false, const TArray<ASunriseUnit*> IgnoreList = {});

	UFUNCTION(BlueprintPure, Category = "Sunrise|Orders")
	FVector GetMovementGoal() const { return CurrentMovementGoal; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseTeam GetTeam() const { return Team; }

	/** Numeric authority used by multi-team modes. 0=player, 1=legacy enemy, -1=neutral. */
	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	int32 GetTeamId() const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Unit")
	void SetTeam(ESunriseTeam NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Unit")
	void SetTeamId(int32 NewTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnTFTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	virtual TScriptInterface<IIControllableEntity> GetControllingAgent() override;
	virtual void SetControllingAgent(TScriptInterface<IIControllableEntity> NewAgent) override;
	virtual FOnControllingAgentChanged* GetOnControllingAgentChangedDelegate() override { return &OnControllingAgentChanged; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseUnitRole GetUnitRole() const { return UnitRole; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseCombatRole GetCombatRole() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	FText GetUnitClassDisplayName() const;

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Unit")
	void SetUnitRole(ESunriseUnitRole NewRole, bool bApplyDefaults = true);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseOrderState GetOrderState() const { return OrderState; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|GAS")
	USunriseHealthSet* GetHealthSet() const { return HealthSet; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|GAS")
	USunriseCombatSet* GetCombatSet() const { return CombatSet; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|GAS")
	USunriseMovementSet* GetMovementSet() const { return MovementSet; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	ESunriseUnitKind GetUnitKind() const { return UnitKind; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	bool IsHero() const { return UnitKind == ESunriseUnitKind::Hero; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Hero")
	TSubclassOf<USunriseHeroSquadAbility> GetHeroSquadAbilityClass() const { return HeroSquadAbilityClass; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Hero")
	void SetHeroSquadAbilityClass(TSubclassOf<USunriseHeroSquadAbility> NewAbilityClass) { HeroSquadAbilityClass = NewAbilityClass; }
	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	float GetHeroRespawnDelay() const { return HeroRespawnDelay; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Sunrise|Unit")
	void ConfigureControl(ESunriseUnitKind NewKind, TScriptInterface<IIControllableEntity> NewAgent);

	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	USunriseWeapon* GetWeapon() const { return Weapon; }

	UFUNCTION(BlueprintPure, Category = "Sunrise|Weapon")
	bool CanTargetWithWeapon(const ASunriseUnit* Target) const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "Sunrise|Unit")
	bool IsSelected() const { return bSelected; }

	UFUNCTION(BlueprintCallable, Category = "Sunrise|Combat")
	float ReceiveHealing(float Amount, ASunriseUnit* Healer);

	/** Applies difficulty through GAS base attributes while preserving current health percentage. */
	void ApplyDifficultyScaling(float HealthMultiplier, float PowerMultiplier);

	/** Weapon-facing API. The Weapon owns attack timing and ability execution. */
	void ApplyWeaponAttributes(float Damage, float Range, float Interval);
	float GetAttackPowerAttribute() const;
	void DealWeaponDamage(ASunriseUnit* Target, float Damage);
	void DealWeaponHealing(ASunriseUnit* Target, float Healing);
	void NotifyWeaponAction(ASunriseUnit* Target, bool bHealing, bool bAreaAction);

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Events")
	FOnUnitMoveCompletedDelegate OnMoveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Events")
	FOnSunriseUnitHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Sunrise|Events")
	FOnSunriseUnitDied OnDied;

protected:
	void ApplyRoleDefaults();
	void EquipDefaultWeaponForRole();
	void InitializeAbilityAttributes(float HealthPercent = 1.0f);
	float ApplyHealthGameplayEffect(
		ASunriseUnit* Source, TSubclassOf<UGameplayEffect> EffectClass, FName MagnitudeDataName, float SignedMagnitude);
	void HandleHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMoveSpeedAttributeChanged(const FOnAttributeChangeData& ChangeData);
	UFUNCTION()
	void HandleVitalityStateChanged(AActor* OwningActor, EVitalityState OldState, EVitalityState NewState);
	UFUNCTION()
	void OnRep_ControllingAgent(AActor* OldAgent);
	UFUNCTION()
	void HandleTeamChanged(UObject* TeamAgent, int32 PreviousTeamId, int32 NewTeamId);
	void UpdateOrder(float DeltaSeconds);
	void AcquireAutomaticTarget();
	bool IsValidActionTarget(const ASunriseUnit* Candidate) const;
	void PerformAction(ASunriseUnit* Target);
	void MoveTowardActor(ASunriseUnit* Target);
	void Die(AController* KillerController, AActor* DamageCauser);
	void HandleMoveFinished();
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);
	bool IssueMoveOrderInternal(const FVector& Destination, bool bFromPlayer);
	void IssueTargetOrderInternal(ASunriseUnit* Target, bool bFromPlayer);
	void SetPlayerOrderActive(bool bActive);

	UFUNCTION()
	void OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation", meta = (DisplayName = "Unit Selected"))
	void BP_UnitSelected();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation", meta = (DisplayName = "Unit Deselected"))
	void BP_UnitDeselected();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Sunrise|Presentation", meta = (DisplayName = "Stop Animation"))
	void BP_StopAnimation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation", meta = (DisplayName = "Interaction Behavior"))
	void BP_InteractionBehavior(ASunriseUnit* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation", meta = (DisplayName = "Combat Action"))
	void BP_CombatAction(ASunriseUnit* Target, bool bWasHealing);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sunrise|Presentation", meta = (DisplayName = "Unit Died"))
	void BP_UnitDied();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> SelectionDecal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|GAS")
	TObjectPtr<USunriseHealthSet> HealthSet;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|GAS")
	TObjectPtr<USunriseCombatSet> CombatSet;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|GAS")
	TObjectPtr<USunriseMovementSet> MovementSet;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UVitalityComponent> VitalityComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UControllableComponent> ControllableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sunrise|Team")
	TObjectPtr<UTFTeamActorComponent> TeamComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Instanced, Transient, Category = "Sunrise|Weapon")
	TObjectPtr<USunriseWeapon> Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Identity")
	ESunriseTeam Team = ESunriseTeam::Friendly;

	/** Deprecated compatibility mirror for existing BP_SunriseUnit assets. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sunrise|Identity", meta = (DeprecatedProperty))
	int32 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Identity")
	ESunriseUnitRole UnitRole = ESunriseUnitRole::Melee;
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Sunrise|Identity")
	ESunriseUnitKind UnitKind = ESunriseUnitKind::Creep;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Hero", meta = (ClampMin = "1.0", Units = "s"))
	float HeroRespawnDelay = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Hero", meta = (ClampMin = "1.0"))
	float HeroHealthMultiplier = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Hero", meta = (ClampMin = "1.0"))
	float HeroPowerMultiplier = 1.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sunrise|Hero")
	TSubclassOf<USunriseHeroSquadAbility> HeroSquadAbilityClass;

	/** Applies balanced native presets at BeginPlay. Disable to use Stats verbatim. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sunrise|Stats")
	bool bUseRoleDefaults = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sunrise|Stats")
	FSunriseUnitStats Stats;

	/** Optional original-template EQS assets are retained for Blueprint compatibility. */
	UPROPERTY(EditAnywhere, Category = "Sunrise|Navigation")
	TObjectPtr<UEnvQuery> InteractionQuery;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Navigation")
	TObjectPtr<UEnvQuery> NoInteractionQuery;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Navigation", meta = (ClampMin = "0", Units = "cm"))
	float MovementAcceptanceRadius = 75.0f;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Combat", meta = (ClampMin = "0", Units = "cm"))
	float InteractionRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Combat")
	bool bAutoAcquireTargets = true;

	UPROPERTY(EditAnywhere, Category = "Sunrise|Combat", meta = (ClampMin = "0.1", Units = "s"))
	float DecisionInterval = 0.4f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sunrise|Runtime")
	ESunriseOrderState OrderState = ESunriseOrderState::Idle;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Sunrise|Runtime")
	TObjectPtr<ASunriseUnit> ActionTarget;

	TObjectPtr<AAIController> AIController;
	TObjectPtr<UEnvQueryInstanceBlueprintWrapper> EnvQueryInstance;
	FVector CurrentMovementGoal = FVector::ZeroVector;
	TArray<TObjectPtr<ASunriseUnit>> InteractIgnoreList;
	float DecisionTimeRemaining = 0.0f;
	float ActionTimeRemaining = 0.0f;
	bool bSelected = false;
	bool bForcedTarget = false;
	/** Explicit player intent has priority over autonomous aggro and retaliation. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Sunrise|Orders")
	bool bPlayerOrderActive = false;
	bool bInteractOnArrival = false;
	bool bHasExplicitTeamId = false;
	bool bExternalInteractionActive = false;
	TWeakObjectPtr<ASunriseUnit> LastDamageSource;
	TWeakObjectPtr<ASunriseUnit> FocusTarget;
	float FocusTargetExpiryTime = 0.0f;
	FAIRequestID ActiveMoveRequestId;
	UPROPERTY(ReplicatedUsing = OnRep_ControllingAgent)
	TObjectPtr<AActor> ControllingAgentActor;
	UPROPERTY()
	FOnControllingAgentChanged OnControllingAgentChanged;
};
