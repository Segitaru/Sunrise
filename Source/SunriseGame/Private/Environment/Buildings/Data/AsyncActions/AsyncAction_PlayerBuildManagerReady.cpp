#include "Data/AsyncActions/AsyncAction_PlayerBuildManagerReady.h"

#include "Gameplay/PlayerBuildManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_PlayerBuildManagerReady)

UAsyncAction_PlayerBuildManagerReady::UAsyncAction_PlayerBuildManagerReady(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_PlayerBuildManagerReady* UAsyncAction_PlayerBuildManagerReady::WaitForPlayerBuildManagerReady(UPlayerBuildManager* FromManager)
{
	UAsyncAction_PlayerBuildManagerReady* Action = nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(FromManager, EGetWorldErrorMode::LogAndReturnNull))
	{
		Action = NewObject<UAsyncAction_PlayerBuildManagerReady>();
		Action->TargetPlayerBuildManager = FromManager;
		Action->RegisterWithGameInstance(World);
	}

	return Action;
}

void UAsyncAction_PlayerBuildManagerReady::Activate()
{
	if (UPlayerBuildManager* PBM = TargetPlayerBuildManager.Get())
	{
		PBM->CallOrRegister_OnPlayerBuilderReady(FOnPlayerBuilderReady::FDelegate::CreateUObject(this, &ThisClass::OnManagerReady));
	}
	else
	{
		OnFailed.Broadcast();
		// No world so we'll never finish naturally
		SetReadyToDestroy();
	}
}
void UAsyncAction_PlayerBuildManagerReady::OnManagerReady() const
{
	OnReady.Broadcast();
}