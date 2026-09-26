#include "GameModes/Survival/Components/SurvivalBuildComponent.h"

#include "CollisionQueryParams.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/Survival/Actors/SurvivalBuilding.h"
#include "GameModes/Survival/Components/SurvivalEconomyComponent.h"
#include "GameModes/Survival/Components/SurvivalWorkerComponent.h"
#include "GameModes/Survival/SurvivalGameMatchComponent.h"
#include "GameModes/Survival/SurvivalGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Teams/System/ModularTeamAgentInterface.h"
#include "Units/SunriseUnit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalBuildComponent)

USurvivalBuildComponent::USurvivalBuildComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	FSurvivalBuildOption MainBase;
	MainBase.BuildingId = SurvivalGameplayTags::Building_MainBase;
	MainBase.BuildingClass = ASurvivalMainBase::StaticClass();
	MainBase.Cost = ASurvivalMainBase::StaticClass()->GetDefaultObject<ASurvivalMainBase>()->GetConstructionCost();
	MainBase.MaxPerPlayer = 1;
	MainBase.PlacementExtent = FVector(220.0f, 220.0f, 140.0f);
	BuildOptions.Add(MainBase);

	FSurvivalBuildOption Barracks;
	Barracks.BuildingId = SurvivalGameplayTags::Building_Barracks;
	Barracks.BuildingClass = ASurvivalBarracks::StaticClass();
	Barracks.Cost = ASurvivalBarracks::StaticClass()->GetDefaultObject<ASurvivalBarracks>()->GetConstructionCost();
	Barracks.PlacementExtent = FVector(170.0f, 130.0f, 100.0f);
	BuildOptions.Add(Barracks);

	FSurvivalBuildOption House;
	House.BuildingId = SurvivalGameplayTags::Building_House;
	House.BuildingClass = ASurvivalHouse::StaticClass();
	House.Cost = ASurvivalHouse::StaticClass()->GetDefaultObject<ASurvivalHouse>()->GetConstructionCost();
	House.PlacementExtent = FVector(100.0f, 100.0f, 110.0f);
	BuildOptions.Add(House);

	FSurvivalBuildOption Defense;
	Defense.BuildingId = SurvivalGameplayTags::Building_Defense;
	Defense.BuildingClass = ASurvivalDefenseBuilding::StaticClass();
	Defense.Cost = ASurvivalDefenseBuilding::StaticClass()->GetDefaultObject<ASurvivalDefenseBuilding>()->GetConstructionCost();
	Defense.PlacementExtent = FVector(90.0f, 90.0f, 170.0f);
	BuildOptions.Add(Defense);
}

void USurvivalBuildComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, LastBuildFailure, COND_OwnerOnly, REPNOTIFY_Always);
}

void USurvivalBuildComponent::ServerRequestBuild_Implementation(FGameplayTag BuildingId, FTransform Transform, ASunriseUnit* Builder)
{
	const FSurvivalBuildOption* Option = FindOption(BuildingId);
	if (!Option)
	{
		SetBuildResult(ESurvivalBuildFailure::UnknownDefinition);
		return;
	}
	const ESurvivalBuildFailure Validation = ValidateRequest(*Option, Transform, Builder);
	if (Validation != ESurvivalBuildFailure::None)
	{
		SetBuildResult(Validation);
		return;
	}

	USurvivalEconomyComponent* Economy = GetPlayerState<APlayerState>()->FindComponentByClass<USurvivalEconomyComponent>();
	if (!Economy || !Economy->TrySpend(Option->Cost))
	{
		SetBuildResult(ESurvivalBuildFailure::InsufficientResources);
		return;
	}

	APlayerState* PlayerState = GetPlayerState<APlayerState>();
	ASurvivalBuilding* const Building = GetWorld()->SpawnActorDeferred<ASurvivalBuilding>(
		Option->BuildingClass, Transform, PlayerState, Builder, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Building)
	{
		Economy->AddResources(Option->Cost);
		SetBuildResult(ESurvivalBuildFailure::SpawnFailed);
		return;
	}

	Building->InitializeConstructionSite();
	if (const IModularTeamAgentInterface* const TeamAgent = Cast<IModularTeamAgentInterface>(PlayerState))
	{
		IModularTeamAgentInterface* const BuildingTeamAgent = Cast<IModularTeamAgentInterface>(Building);
		BuildingTeamAgent->SetGenericTeamId(TeamAgent->GetGenericTeamId());
	}
	Building->FinishSpawning(Transform);

	USurvivalWorkerComponent* WorkerComponent = Builder->FindComponentByClass<USurvivalWorkerComponent>();
	if (!WorkerComponent)
	{
		WorkerComponent = NewObject<USurvivalWorkerComponent>(Builder, TEXT("SurvivalWorker"));
		Builder->AddInstanceComponent(WorkerComponent);
		WorkerComponent->RegisterComponent();
	}
	if (!WorkerComponent->StartBuildOrder(Building))
	{
		Building->Destroy();
		Economy->AddResources(Option->Cost);
		SetBuildResult(ESurvivalBuildFailure::InvalidBuilder);
		return;
	}
	SetBuildResult(ESurvivalBuildFailure::None);
}

const FSurvivalBuildOption* USurvivalBuildComponent::FindOption(FGameplayTag BuildingId) const
{
	return BuildOptions.FindByPredicate(
		[BuildingId](const FSurvivalBuildOption& Option)
		{
			return Option.BuildingId == BuildingId;
		});
}

ESurvivalBuildFailure USurvivalBuildComponent::ValidateRequest(
	const FSurvivalBuildOption& Option, const FTransform& Transform, ASunriseUnit* Builder) const
{
	const APlayerState* PlayerState = GetPlayerState<APlayerState>();
	const AController* Controller = PlayerState ? Cast<AController>(PlayerState->GetOwner()) : nullptr;
	const USurvivalGameMatchComponent* Match = USurvivalGameMatchComponent::Find(this);
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return ESurvivalBuildFailure::NotAuthority;
	}
	if (!Match || Match->GetSurvivalMatchState() != ESurvivalMatchState::InProgress)
	{
		return ESurvivalBuildFailure::MatchEnded;
	}
	const UObject* ControllingAgent = IsValid(Builder) ? Builder->GetControllingAgent().GetObject() : nullptr;
	if (!IsValid(Builder) || !Builder->IsAlive() || !Controller || (ControllingAgent != Controller && ControllingAgent != PlayerState) ||
		(!Builder->FindComponentByClass<USurvivalWorkerComponent>() && !Builder->HasPawnTag(SurvivalGameplayTags::Unit_Worker)))
	{
		return ESurvivalBuildFailure::InvalidBuilder;
	}
	if (Transform.ContainsNaN() || Transform.GetLocation().GetAbsMax() > HALF_WORLD_MAX ||
		!Transform.GetScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER))
	{
		return ESurvivalBuildFailure::InvalidTransform;
	}

	int32 ExistingCount = 0;
	for (TActorIterator<ASurvivalBuilding> It(GetWorld()); It; ++It)
	{
		if (It->GetOwner() == PlayerState && It->IsA(Option.BuildingClass) && It->IsAlive() && It->GetHealth() > 0.0f)
		{
			++ExistingCount;
		}
	}
	if (ExistingCount >= Option.MaxPerPlayer)
	{
		return ESurvivalBuildFailure::LimitReached;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SurvivalBuildingPlacement), false, Builder);
	const FVector TestExtent(FMath::Max(1.0f, Option.PlacementExtent.X - 5.0f), FMath::Max(1.0f, Option.PlacementExtent.Y - 5.0f),
		FMath::Max(1.0f, Option.PlacementExtent.Z - 5.0f));
	const FVector TestLocation = Transform.GetLocation() + FVector(0.0f, 0.0f, 6.0f);
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (GetWorld()->OverlapBlockingTestByChannel(
			TestLocation, Transform.GetRotation(), ECC_WorldStatic, FCollisionShape::MakeBox(TestExtent), QueryParams))
	{
		return ESurvivalBuildFailure::Blocked;
	}
	const USurvivalEconomyComponent* Economy = PlayerState->FindComponentByClass<USurvivalEconomyComponent>();
	return Economy && Economy->CanAfford(Option.Cost) ? ESurvivalBuildFailure::None : ESurvivalBuildFailure::InsufficientResources;
}

void USurvivalBuildComponent::SetBuildResult(ESurvivalBuildFailure Failure)
{
	LastBuildFailure = Failure;
	OnRep_LastBuildFailure();
}

void USurvivalBuildComponent::OnRep_LastBuildFailure()
{
	OnBuildResult.Broadcast(LastBuildFailure);
}
