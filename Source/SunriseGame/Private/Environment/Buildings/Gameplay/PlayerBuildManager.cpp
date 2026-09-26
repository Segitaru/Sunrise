// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/PlayerBuildManager.h"

#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>

#include "BuildingTypes.h"
#include "Data/BuildingSaveGame.h"
#include "Gameplay/GameBuilding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerBuildManager)

namespace BuildingConstant
{
	FString SaveSlot = "BuildingInfo";
}

void UPlayerBuildManager::BeginPlay()
{
	Super::BeginPlay();

	InstigatingPawn = GetPawn<APawn>();
	if (!InstigatingPawn)
	{
		GetPlayerState<APlayerState>()->OnPawnSet.AddDynamic(this, &ThisClass::OnPawnSet);
		return;
	}

	TryLoadInfo();
}

void UPlayerBuildManager::TryLoadInfoForBot()
{
	if (const APlayerState* BotPs = Cast<APlayerState>(GetOwner()))
	{
		if (BotPs->IsABot())
		{
			UserIndex = AIBuildingUserSlot;
			LoadUserInfo();
		}
	}
}

void UPlayerBuildManager::TryLoadInfo()
{
	const APlayerController* OwningController = Cast<APlayerController>(GetOwner()->GetOwner());
	if (!OwningController)
	{
		TryLoadInfoForBot();

		return;
	}

	const ULocalPlayer* LocalPlayer = OwningController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UserIndex = LocalPlayer->GetPlatformUserIndex();

	LoadUserInfo();
}

void UPlayerBuildManager::OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	InstigatingPawn = NewPawn;

	if (!InstigatingPawn)
	{
		return;
	}

	GetPlayerState<APlayerState>()->OnPawnSet.RemoveAll(this);

	TryLoadInfo();
}

void UPlayerBuildManager::AddBuilding(AGameBuilding* Building)
{
	if (!Building)
	{
		return;
	}
	Buildings.Add(Building);
}

bool UPlayerBuildManager::SpawnBuilding(
	TSubclassOf<AGameBuilding> BuildingClass, const FTransform& SpawnTransform, AGameBuilding*& SpawnedBuilding)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return false;
	}
	FActorSpawnParameters NewParameters;
	NewParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	NewParameters.Owner = GetOwner();
	NewParameters.Instigator = InstigatingPawn;

	SpawnedBuilding = World->SpawnActorDeferred<AGameBuilding>(
		BuildingClass, SpawnTransform, GetOwner(), InstigatingPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!SpawnedBuilding)
	{
		return false;
	}

	SpawnedBuilding->FinishSpawning(SpawnTransform);

	if (GetOwnerRole() == ROLE_Authority)
	{
		AddBuilding(SpawnedBuilding);
	}

	SaveBuildingInfo(SpawnedBuilding);

	return true;
}



FString UPlayerBuildManager::GetSaveSlotName(const int32& FromUserIndex) const
{
	FString WorldMapName = FPackageName::GetShortName(GetWorld()->GetMapName());

	return BuildingConstant::SaveSlot + "_" + WorldMapName + "_" + FString::FromInt(FromUserIndex);
}
void UPlayerBuildManager::DEV_ClearBuildings()
{
	for (const auto& Building : Buildings)
	{
		if (Building)
		{
			Building->Destroy();
		}
	}

	Buildings.Empty();

	ClearSaveInfo();

	UGameplayStatics::DeleteGameInSlot(GetSaveSlotName(AIBuildingUserSlot), AIBuildingUserSlot);
}

void UPlayerBuildManager::CallOrRegister_OnPlayerBuilderReady(FOnPlayerBuilderReady::FDelegate&& Delegate)
{
	if (bIsReady)
	{
		Delegate.Execute();
	}
	else
	{
		OnPlayerBuilderReady.Add(MoveTemp(Delegate));
	}
}

void UPlayerBuildManager::LoadUserInfo()
{
	bHaveStoredInfo = UGameplayStatics::DoesSaveGameExist(GetSaveSlotName(UserIndex), UserIndex);

	if (bHaveStoredInfo)
	{
		UGameplayStatics::AsyncLoadGameFromSlot(GetSaveSlotName(UserIndex), UserIndex,
			FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &UPlayerBuildManager::OnSaveGameLoaded));
	}
	else
	{
		BuildingSaveInfo = Cast<UBuildingSaveGame>(UGameplayStatics::CreateSaveGameObject(UBuildingSaveGame::StaticClass()));

		bIsReady = true;
		OnPlayerBuilderReady.Broadcast();
	}
}

void UPlayerBuildManager::SaveBuilderInfo()
{
	if (!BuildingSaveInfo)
	{
		return;
	}

	UGameplayStatics::AsyncSaveGameToSlot(BuildingSaveInfo, GetSaveSlotName(UserIndex), UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UPlayerBuildManager::OnSaveGameSaved));
}

void UPlayerBuildManager::SaveBuildingInfo(AGameBuilding* UpdatedBuilding)
{
	if (!UpdatedBuilding || !BuildingSaveInfo || GetNetMode() != NM_Standalone)
	{
		return;
	}

	UpdatedBuilding->OnUpdateBuildingInfo.AddUniqueDynamic(this, &ThisClass::SaveBuildingInfo);

	const auto BuildingInfo = BuildingSaveInfo->BuildingInfo.Find(UpdatedBuilding->GetUniqueID());

	if (!BuildingInfo)
	{
		FBuildingInfo NewBuildingInfo(UpdatedBuilding);

		UE_LOG(LogGameBuilder, Display, TEXT("New Save info for %s: Transform: %s"), *GetNameSafe(UpdatedBuilding),
			*NewBuildingInfo.Transform.ToString());

		BuildingSaveInfo->BuildingInfo.Add(UpdatedBuilding->GetUniqueID(), NewBuildingInfo);

		SaveBuilderInfo();

		return;
	}

	BuildingInfo->BuildingType = UpdatedBuilding->GetClass();
	UpdatedBuilding->GetCurrentState(BuildingInfo->CurrentLevel, BuildingInfo->BuildingProgress);
	BuildingInfo->Transform = UpdatedBuilding->GetTransform();

	UE_LOG(LogGameBuilder, Display, TEXT("Updated Save info for %s: Transform: %s"), *GetNameSafe(UpdatedBuilding),
		*BuildingInfo->Transform.ToString());

	BuildingInfo->VitalityState = UpdatedBuilding->GetVitalityValues();

	SaveBuilderInfo();
}

void UPlayerBuildManager::ClearSaveInfo()
{
	if (!BuildingSaveInfo)
	{
		return;
	}

	BuildingSaveInfo->BuildingInfo.Empty();

	UE_LOG(LogGameBuilder, Display, TEXT("Saved info for %s cleared!"), *GetNameSafe(GetOwner()));

	BuildingSaveInfo = nullptr;

	UGameplayStatics::DeleteGameInSlot(GetSaveSlotName(UserIndex), UserIndex);
}

bool UPlayerBuildManager::IsInfoWasStored()
{
	return bHaveStoredInfo;
}

void UPlayerBuildManager::TrySetReadyState()
{
	++LoadedBuildingIndex;
	if (LoadedBuildingIndex >= MustBeLoaded)
	{
		UE_LOG(LogGameBuilder, Display, TEXT("All building spawned! Builder ready for: %s!"), *GetNameSafe(GetOwner()));
		bIsReady = true;
		OnPlayerBuilderReady.Broadcast();
	}
}


void UPlayerBuildManager::AsyncCreateBuilding()
{
	if (!BuildingSaveInfo)
	{
		return;
	}

	MustBeLoaded = BuildingSaveInfo->BuildingInfo.Num();
	if (MustBeLoaded == 0)
	{
		UE_LOG(LogGameBuilder, Display, TEXT("Nothing to load, builder ready for: %s!"), *GetNameSafe(GetOwner()));

		bIsReady = true;
		OnPlayerBuilderReady.Broadcast();
	}
	else
	{
		UE_LOG(LogGameBuilder, Display, TEXT("Async creation building started for: %s!"), *GetNameSafe(GetOwner()));
	}

	UE_LOG(LogGameBuilder, Display, TEXT("Get info, from object: %s!"), *GetNameSafe(BuildingSaveInfo));

	const auto CurrentSavedInfo = MoveTemp(BuildingSaveInfo->BuildingInfo);

	for (const auto [BuildingId, Info] : CurrentSavedInfo)
	{
		const auto BuildingClass = Info.BuildingType.Get();

		UE_LOG(LogGameBuilder, Display, TEXT("Try spawn %s in %s for %s"), *GetNameSafe(BuildingClass), *Info.Transform.ToString(),
			*GetNameSafe(GetOwner()));

		if (!BuildingClass)
		{
			AsyncLoad<AGameBuilding>(Info.BuildingType,
				TFunction<void(TSubclassOf<AGameBuilding>)>(
					[this, Info](const TSubclassOf<AGameBuilding>& LoadedClass)
					{
						if (!LoadedClass)
						{
							UE_LOG(LogGameBuilder, Error, TEXT("Upload building class was failed!"));
							return;
						}

						UE_LOG(LogGameBuilder, Display, TEXT("%s load completed for %s!"), *GetNameSafe(LoadedClass),
							*GetNameSafe(GetOwner()));

						UWorld* CurrentWorld = GetWorld();

						if (!CurrentWorld || CurrentWorld->bIsTearingDown)
						{
							UE_LOG(LogGameBuilder, Error, TEXT("World dont existed or ready to destroy"));
							return;
						}

						if (!InstigatingPawn)
						{
							UE_LOG(LogGameBuilder, Error, TEXT("Controller with not existed Instigating Pawn %i"), UserIndex);
							return;
						}

						const auto SpawnedActor = CurrentWorld->SpawnActorDeferred<AGameBuilding>(
							LoadedClass, FTransform(), GetOwner(), InstigatingPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

						if (!SpawnedActor)
						{
							UE_LOG(LogGameBuilder, Error, TEXT("Spawn build failed!"));
							return;
						}

						SpawnedActor->SetCurrentState(Info.CurrentLevel, Info.BuildingProgress);

						SpawnedActor->FinishSpawning(Info.Transform);

						UE_LOG(LogGameBuilder, Display, TEXT("%s spawn build finished in %s for %s"), *GetNameSafe(SpawnedActor),
							*Info.Transform.ToString(), *GetNameSafe(GetOwner()));

						AddBuilding(SpawnedActor);
						SaveBuildingInfo(SpawnedActor);
						TrySetReadyState();
					}));
		}
		else
		{
			UWorld* CurrentWorld = GetWorld();

			UE_LOG(LogGameBuilder, Display, TEXT("%s asset was already loaded %s!"), *GetNameSafe(BuildingClass), *GetNameSafe(GetOwner()));

			if (!CurrentWorld || CurrentWorld->bIsTearingDown)
			{
				UE_LOG(LogGameBuilder, Error, TEXT("World dont existed or ready to destroy"));
				return;
			}

			const auto SpawnedActor = CurrentWorld->SpawnActorDeferred<AGameBuilding>(
				BuildingClass, FTransform(), GetOwner(), InstigatingPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (!SpawnedActor)
			{
				UE_LOG(LogGameBuilder, Error, TEXT("Spawn build failed!"));
				return;
			}
			SpawnedActor->SetCurrentState(Info.CurrentLevel, Info.BuildingProgress);

			SpawnedActor->FinishSpawning(Info.Transform);

			UE_LOG(LogGameBuilder, Display, TEXT("%s spawn build finished in %s for %s"), *GetNameSafe(SpawnedActor),
				*Info.Transform.ToString(), *GetNameSafe(GetOwner()));

			AddBuilding(SpawnedActor);
			SaveBuildingInfo(SpawnedActor);
			TrySetReadyState();
		}
	}
}

void UPlayerBuildManager::OnSaveGameLoaded(const FString& SlotName, const int32 InUserIndex, USaveGame* SaveObject)
{
	UBuildingSaveGame* LoadedBuildingInfo = Cast<UBuildingSaveGame>(SaveObject);

	if (!LoadedBuildingInfo)
	{
		return;
	}

	BuildingSaveInfo = MoveTemp(LoadedBuildingInfo);

	UE_LOG(LogGameBuilder, Display, TEXT("Building data loaded for: %s from index: %i, from object: %s!"), *GetNameSafe(GetOwner()),
		InUserIndex, *GetNameSafe(LoadedBuildingInfo));

	AsyncCreateBuilding();
}

void UPlayerBuildManager::OnSaveGameSaved(const FString& SlotName, const int32 InUserIndex, bool bIsSuccess)
{
	if (!bIsSuccess)
	{
		return;
	}

	UE_LOG(LogGameBuilder, Display, TEXT("Building data saved for: %s in index %i!"), *GetNameSafe(GetOwner()), InUserIndex);
}