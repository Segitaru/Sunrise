#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"

#include "AbilitySystem/Attributes/BuildingResourceSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalEconomyComponent)

USurvivalEconomyComponent::USurvivalEconomyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void USurvivalEconomyComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerState* PlayerState = GetPlayerState<APlayerState>();
	AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState);
	if (!ensureMsgf(AbilitySystemComponent, TEXT("SurvivalEconomyComponent requires an ASC on its PlayerState")))
	{
		return;
	}

	ResourceSet = NewObject<UBuildingResourceSet>(PlayerState, TEXT("SurvivalResourceAttributes"));
	AbilitySystemComponent->AddAttributeSetSubobject<UBuildingResourceSet>(ResourceSet);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetFoodAttribute())
		.AddUObject(this, &ThisClass::HandleResourceChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetWoodAttribute())
		.AddUObject(this, &ThisClass::HandleResourceChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetStoneAttribute())
		.AddUObject(this, &ThisClass::HandleResourceChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetMetalAttribute())
		.AddUObject(this, &ThisClass::HandleResourceChanged);

	if (PlayerState && PlayerState->HasAuthority())
	{
		AbilitySystemComponent->SetNumericAttributeBase(UBuildingResourceSet::GetFoodAttribute(), FMath::Max(0.0f, StartingResources.Food));
		AbilitySystemComponent->SetNumericAttributeBase(UBuildingResourceSet::GetWoodAttribute(), FMath::Max(0.0f, StartingResources.Wood));
		AbilitySystemComponent->SetNumericAttributeBase(
			UBuildingResourceSet::GetStoneAttribute(), FMath::Max(0.0f, StartingResources.Stone));
		AbilitySystemComponent->SetNumericAttributeBase(
			UBuildingResourceSet::GetMetalAttribute(), FMath::Max(0.0f, StartingResources.Metal));
		PopulationCap = FMath::Max(0, StartingPopulationCap);
		OnRep_Population();
	}
}

void USurvivalEconomyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetFoodAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetWoodAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetStoneAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBuildingResourceSet::GetMetalAttribute()).RemoveAll(this);
		if (ResourceSet)
		{
			AbilitySystemComponent->RemoveSpawnedAttribute(ResourceSet);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void USurvivalEconomyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Population, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, PopulationCap, COND_OwnerOnly, REPNOTIFY_Always);
}

USurvivalEconomyComponent* USurvivalEconomyComponent::Find(const UObject* WorldContextObject)
{
	const AActor* Actor = Cast<AActor>(WorldContextObject);
	if (ASunriseUnit* Unit = Cast<ASunriseUnit>(const_cast<UObject*>(WorldContextObject)))
	{
		const AController* Controller = Cast<AController>(Unit->GetControllingAgent().GetObject());
		return Controller && Controller->PlayerState ? Controller->PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	}
	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		return PlayerState->FindComponentByClass<USurvivalEconomyComponent>();
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return Pawn->GetPlayerState() ? Pawn->GetPlayerState()->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
	}
	const AController* Controller = Cast<AController>(Actor);
	return Controller && Controller->PlayerState ? Controller->PlayerState->FindComponentByClass<USurvivalEconomyComponent>() : nullptr;
}

FSurvivalResourceAmounts USurvivalEconomyComponent::GetResources() const
{
	FSurvivalResourceAmounts Result;
	if (ResourceSet)
	{
		Result.Food = ResourceSet->GetFood();
		Result.Wood = ResourceSet->GetWood();
		Result.Stone = ResourceSet->GetStone();
		Result.Metal = ResourceSet->GetMetal();
	}
	return Result;
}

bool USurvivalEconomyComponent::CanAfford(const FSurvivalResourceAmounts& Cost) const
{
	if (!Cost.IsNonNegative())
	{
		return false;
	}
	const FSurvivalResourceAmounts Current = GetResources();
	return Current.Food >= Cost.Food && Current.Wood >= Cost.Wood && Current.Stone >= Cost.Stone && Current.Metal >= Cost.Metal;
}

bool USurvivalEconomyComponent::TrySpend(const FSurvivalResourceAmounts& Cost)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !CanAfford(Cost))
	{
		return false;
	}
	FSurvivalResourceAmounts Delta;
	Delta.Food = -Cost.Food;
	Delta.Wood = -Cost.Wood;
	Delta.Stone = -Cost.Stone;
	Delta.Metal = -Cost.Metal;
	ApplyResourceDelta(Delta);
	return true;
}

void USurvivalEconomyComponent::AddResources(const FSurvivalResourceAmounts& Amounts)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Amounts.IsNonNegative())
	{
		ApplyResourceDelta(Amounts);
	}
}

bool USurvivalEconomyComponent::TryReservePopulation(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount < 0 || Population + Amount > PopulationCap)
	{
		return false;
	}
	Population += Amount;
	OnRep_Population();
	return true;
}

void USurvivalEconomyComponent::ReleasePopulation(int32 Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Population = FMath::Max(0, Population - FMath::Max(0, Amount));
		OnRep_Population();
	}
}

void USurvivalEconomyComponent::AddPopulationCapacity(int32 Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		PopulationCap = FMath::Max(0, PopulationCap + Amount);
		Population = FMath::Min(Population, PopulationCap);
		OnRep_Population();
	}
}

void USurvivalEconomyComponent::OnRep_Population()
{
	OnEconomyChanged.Broadcast();
}

void USurvivalEconomyComponent::HandleResourceChanged(const FOnAttributeChangeData& ChangeData)
{
	OnEconomyChanged.Broadcast();
}

void USurvivalEconomyComponent::ApplyResourceDelta(const FSurvivalResourceAmounts& Delta)
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->ApplyModToAttribute(UBuildingResourceSet::GetFoodAttribute(), EGameplayModOp::Additive, Delta.Food);
	AbilitySystemComponent->ApplyModToAttribute(UBuildingResourceSet::GetWoodAttribute(), EGameplayModOp::Additive, Delta.Wood);
	AbilitySystemComponent->ApplyModToAttribute(UBuildingResourceSet::GetStoneAttribute(), EGameplayModOp::Additive, Delta.Stone);
	AbilitySystemComponent->ApplyModToAttribute(UBuildingResourceSet::GetMetalAttribute(), EGameplayModOp::Additive, Delta.Metal);
}
