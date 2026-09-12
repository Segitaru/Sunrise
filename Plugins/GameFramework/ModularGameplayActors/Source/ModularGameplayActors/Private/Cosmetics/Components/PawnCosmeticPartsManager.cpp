// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cosmetics/Components/PawnCosmeticPartsManager.h"

#include "Cosmetics/Components/PawnCosmeticCreatorComponent.h"
#include "Cosmetics/Settings/PawnCosmeticDeveloperSettings.h"
#include "GameFramework/CheatManagerDefines.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PawnCosmeticPartsManager)

//////////////////////////////////////////////////////////////////////

UPawnCosmeticPartsManager::UPawnCosmeticPartsManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPawnCosmeticPartsManager::BeginPlay()
{
	Super::BeginPlay();

	// Listen for pawn possession changed events
	if (HasAuthority())
	{
		if (AController* OwningController = GetController<AController>())
		{
			OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);

			if (APawn* ControlledPawn = GetPawn<APawn>())
			{
				OnPossessedPawnChanged(nullptr, ControlledPawn);
			}
		}

		ApplyDeveloperSettings();
	}
}

void UPawnCosmeticPartsManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllCharacterParts();
	Super::EndPlay(EndPlayReason);
}

UPawnCosmeticCreatorComponent* UPawnCosmeticPartsManager::GetPawnCustomizer() const
{
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		return ControlledPawn->FindComponentByClass<UPawnCosmeticCreatorComponent>();
	}
	return nullptr;
}

void UPawnCosmeticPartsManager::AddCharacterPart(const FPawnCosmeticPart& NewPart)
{
	AddCharacterPartInternal(NewPart, ECharacterPartSource::Natural);
}

void UPawnCosmeticPartsManager::AddCharacterPartInternal(const FPawnCosmeticPart& NewPart, ECharacterPartSource Source)
{
	FGamePawnControllerCharacterPartEntry& NewEntry = CharacterParts.AddDefaulted_GetRef();
	NewEntry.Part = NewPart;
	NewEntry.Source = Source;

	if (UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer())
	{
		if (NewEntry.Source != ECharacterPartSource::NaturalSuppressedViaCheat)
		{
			NewEntry.Handle = PawnCustomizer->AddCosmeticPart(NewPart);
		}
	}
}

void UPawnCosmeticPartsManager::RemoveCharacterPart(const FPawnCosmeticPart& PartToRemove)
{
	for (auto EntryIt = CharacterParts.CreateIterator(); EntryIt; ++EntryIt)
	{
		if (FPawnCosmeticPart::AreEquivalentParts(EntryIt->Part, PartToRemove))
		{
			if (UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer())
			{
				PawnCustomizer->RemoveCharacterPart(EntryIt->Handle);
			}

			EntryIt.RemoveCurrent();
			break;
		}
	}
}

void UPawnCosmeticPartsManager::RemoveAllCharacterParts()
{
	if (UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer())
	{
		for (FGamePawnControllerCharacterPartEntry& Entry : CharacterParts)
		{
			PawnCustomizer->RemoveCharacterPart(Entry.Handle);
		}
	}

	CharacterParts.Reset();
}

void UPawnCosmeticPartsManager::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// Remove from the old pawn
	if (UPawnCosmeticCreatorComponent* OldCustomizer = OldPawn ? OldPawn->FindComponentByClass<UPawnCosmeticCreatorComponent>() : nullptr)
	{
		for (FGamePawnControllerCharacterPartEntry& Entry : CharacterParts)
		{
			OldCustomizer->RemoveCharacterPart(Entry.Handle);
			Entry.Handle.Reset();
		}
	}

	// Apply to the new pawn
	if (UPawnCosmeticCreatorComponent* NewCustomizer = NewPawn ? NewPawn->FindComponentByClass<UPawnCosmeticCreatorComponent>() : nullptr)
	{
		for (FGamePawnControllerCharacterPartEntry& Entry : CharacterParts)
		{
			// Don't readd if it's already there, this can get called with a null oldpawn
			if (!Entry.Handle.IsValid() && Entry.Source != ECharacterPartSource::NaturalSuppressedViaCheat)
			{
				Entry.Handle = NewCustomizer->AddCosmeticPart(Entry.Part);
			}
		}
	}
}

void UPawnCosmeticPartsManager::ApplyDeveloperSettings()
{
#if UE_WITH_CHEAT_MANAGER
	const UPawnCosmeticDeveloperSettings* Settings = GetDefault<UPawnCosmeticDeveloperSettings>();

	// Suppress or unsuppress natural parts if needed
	const bool bSuppressNaturalParts =
		(Settings->CheatMode == ECosmeticCheatMode::ReplaceParts) && (Settings->CheatCosmeticCharacterParts.Num() > 0);
	SetSuppressionOnNaturalParts(bSuppressNaturalParts);

	// Remove anything added by developer settings and re-add it
	UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer();
	for (auto It = CharacterParts.CreateIterator(); It; ++It)
	{
		if (It->Source == ECharacterPartSource::AppliedViaDeveloperSettingsCheat)
		{
			if (PawnCustomizer != nullptr)
			{
				PawnCustomizer->RemoveCharacterPart(It->Handle);
			}
			It.RemoveCurrent();
		}
	}

	// Add new parts
	for (const FPawnCosmeticPart& PartDesc : Settings->CheatCosmeticCharacterParts)
	{
		AddCharacterPartInternal(PartDesc, ECharacterPartSource::AppliedViaDeveloperSettingsCheat);
	}
#endif
}


void UPawnCosmeticPartsManager::AddCheatPart(const FPawnCosmeticPart& NewPart, bool bSuppressNaturalParts)
{
#if UE_WITH_CHEAT_MANAGER
	SetSuppressionOnNaturalParts(bSuppressNaturalParts);
	AddCharacterPartInternal(NewPart, ECharacterPartSource::AppliedViaCheatManager);
#endif
}

void UPawnCosmeticPartsManager::ClearCheatParts()
{
#if UE_WITH_CHEAT_MANAGER
	UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer();

	// Remove anything added by cheat manager cheats
	for (auto It = CharacterParts.CreateIterator(); It; ++It)
	{
		if (It->Source == ECharacterPartSource::AppliedViaCheatManager)
		{
			if (PawnCustomizer != nullptr)
			{
				PawnCustomizer->RemoveCharacterPart(It->Handle);
			}
			It.RemoveCurrent();
		}
	}

	ApplyDeveloperSettings();
#endif
}

void UPawnCosmeticPartsManager::SetSuppressionOnNaturalParts(bool bSuppressed)
{
#if UE_WITH_CHEAT_MANAGER
	UPawnCosmeticCreatorComponent* PawnCustomizer = GetPawnCustomizer();

	for (FGamePawnControllerCharacterPartEntry& Entry : CharacterParts)
	{
		if ((Entry.Source == ECharacterPartSource::Natural) && bSuppressed)
		{
			// Suppress
			if (PawnCustomizer != nullptr)
			{
				PawnCustomizer->RemoveCharacterPart(Entry.Handle);
				Entry.Handle.Reset();
			}
			Entry.Source = ECharacterPartSource::NaturalSuppressedViaCheat;
		}
		else if ((Entry.Source == ECharacterPartSource::NaturalSuppressedViaCheat) && !bSuppressed)
		{
			// Unsuppress
			if (PawnCustomizer != nullptr)
			{
				Entry.Handle = PawnCustomizer->AddCosmeticPart(Entry.Part);
			}
			Entry.Source = ECharacterPartSource::Natural;
		}
	}
#endif
}
