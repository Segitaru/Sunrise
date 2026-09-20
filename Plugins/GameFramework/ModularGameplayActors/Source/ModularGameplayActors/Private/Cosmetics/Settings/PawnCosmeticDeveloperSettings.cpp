// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cosmetics/Settings/PawnCosmeticDeveloperSettings.h"

#include "Cosmetics/Components/PawnCosmeticPartsManager.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/App.h"
#include "ModularDevelopmentStatics.h"
#include "TimerManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PawnCosmeticDeveloperSettings)

#define LOCTEXT_NAMESPACE "LyraCheats"

UPawnCosmeticDeveloperSettings::UPawnCosmeticDeveloperSettings()
{
}

FName UPawnCosmeticDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR

void UPawnCosmeticDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplySettings();
}

void UPawnCosmeticDeveloperSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	ApplySettings();
}

void UPawnCosmeticDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();

	ApplySettings();
}

void UPawnCosmeticDeveloperSettings::ApplySettings()
{
	if (GIsEditor && (GEngine != nullptr))
	{
		ReapplyLoadoutIfInPIE();
	}
}


void UPawnCosmeticDeveloperSettings::ReapplyLoadoutIfInPIE()
{
#if WITH_SERVER_CODE
	// Update the loadout on all players
	UWorld* ServerWorld = UModularDevelopmentStatics::FindPlayInEditorAuthorityWorld();
	if (!IsValid(ServerWorld))
	{
		return;
	}
	ServerWorld->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([=]()
			{
				for (TActorIterator<APlayerController> PCIterator(ServerWorld); PCIterator; ++PCIterator)
				{
					if (APlayerController* PC = *PCIterator)
					{
						if (UPawnCosmeticPartsManager* CosmeticComponent = PC->FindComponentByClass<UPawnCosmeticPartsManager>())
						{
							CosmeticComponent->ApplyDeveloperSettings();
						}
					}
				}
			}));
#endif	// WITH_SERVER_CODE
}

void UPawnCosmeticDeveloperSettings::OnPlayInEditorStarted() const
{
	// Show a notification toast to remind the user that there's an experience override set
	if (CheatCosmeticCharacterParts.Num() > 0)
	{
		FNotificationInfo Info(LOCTEXT("CosmeticOverrideActive", "Applying Cosmetic Override"));
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE

