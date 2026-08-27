// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SunriseSettingsWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/LocalPlayer.h"
#include "GameSetting.h"
#include "GameSettingCollection.h"
#include "GameSettingFilterState.h"
#include "GameSettingValue.h"
#include "GameSettingValueDiscrete.h"
#include "GameSettingValueScalar.h"
#include "GameSettingsLocal.h"
#include "GameSettingsShared.h"
#include "Styling/CoreStyle.h"
#include "UI/SunriseWidgets.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace SunriseSettings
{
	TSharedRef<SButton> MakeButton(const FText& Label, const FOnClicked& Handler)
	{
		return SNew(SButton)
			.OnClicked(Handler)
			.ContentPadding(FMargin(18.0f, 8.0f))
			.HAlign(HAlign_Center)[SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))];
	}

	FText GetDiscreteValueText(TWeakObjectPtr<UGameSettingValueDiscrete> WeakSetting)
	{
		const UGameSettingValueDiscrete* Setting = WeakSetting.Get();
		if (!Setting)
		{
			return FText::GetEmpty();
		}
		const TArray<FText> Options = Setting->GetDiscreteOptions();
		const int32 Index = Setting->GetDiscreteOptionIndex();
		return Options.IsValidIndex(Index) ? Options[Index] : FText::FromString(TEXT("Unavailable"));
	}
} // namespace SunriseSettings

void USunriseSettingsWidget::SetOwningMenu(USunriseMainMenuWidget* InOwningMenu)
{
	OwningMenu = InOwningMenu;
}

TSharedRef<SWidget> USunriseSettingsWidget::RebuildWidget()
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	LocalSettings = UGameSettingsLocal::Get();
	SharedSettings = UGameSettingsShared::GetSharedSettings(LocalPlayer);
	Registry = NewObject<USunriseGameSettingRegistry>(this);
	if (LocalPlayer)
	{
		Registry->Initialize(LocalPlayer);
	}

	EditableValues.Reset();
	TArray<UGameSetting*> Settings;
	if (LocalPlayer)
	{
		FGameSettingFilterState FilterState;
		FilterState.bIncludeNestedPages = true;
		Registry->GetSettingsForFilter(FilterState, Settings);
	}

	TSharedRef<SVerticalBox> SettingsList = SNew(SVerticalBox);
	for (UGameSetting* Setting : Settings)
	{
		if (!IsValid(Setting) || !Setting->GetEditState().IsVisible())
		{
			continue;
		}

		if (Cast<UGameSettingCollection>(Setting))
		{
			SettingsList->AddSlot().AutoHeight().Padding(8.0f, 18.0f, 8.0f, 6.0f)[SNew(STextBlock)
																					  .Text(Setting->GetDisplayName())
																					  .ColorAndOpacity(FLinearColor(0.9f, 0.72f, 0.2f))
																					  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))];
			continue;
		}

		UGameSettingValue* ValueSetting = Cast<UGameSettingValue>(Setting);
		if (!ValueSetting)
		{
			continue;
		}
		EditableValues.Add(ValueSetting);
		ValueSetting->StoreInitial();

		const bool bEnabled = Setting->GetEditState().IsEnabled();
		if (UGameSettingValueDiscrete* Discrete = Cast<UGameSettingValueDiscrete>(ValueSetting))
		{
			const TWeakObjectPtr<UGameSettingValueDiscrete> WeakSetting(Discrete);
			SettingsList->AddSlot().AutoHeight().Padding(8.0f, 4.0f)
				[SNew(SHorizontalBox) +
					SHorizontalBox::Slot().FillWidth(0.55f).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(Setting->GetDisplayName()).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))] +
					SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SNew(SButton).IsEnabled(bEnabled).OnClicked(FOnClicked::CreateUObject(
						this, &ThisClass::AdjustDiscreteSetting, WeakSetting, -1))[SNew(STextBlock).Text(FText::FromString(TEXT("<")))]] +
					SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)[SNew(STextBlock)
																					  .Text_Lambda(
																						  [WeakSetting]()
																						  {
																							  return SunriseSettings::GetDiscreteValueText(
																								  WeakSetting);
																						  })
																					  .Justification(ETextJustify::Center)] +
					SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SNew(SButton).IsEnabled(bEnabled).OnClicked(FOnClicked::CreateUObject(
						this, &ThisClass::AdjustDiscreteSetting, WeakSetting, 1))[SNew(STextBlock).Text(FText::FromString(TEXT(">")))]]];
		}
		else if (UGameSettingValueScalar* Scalar = Cast<UGameSettingValueScalar>(ValueSetting))
		{
			const TWeakObjectPtr<UGameSettingValueScalar> WeakSetting(Scalar);
			SettingsList->AddSlot().AutoHeight().Padding(8.0f, 4.0f)
				[SNew(SHorizontalBox) +
					SHorizontalBox::Slot().FillWidth(0.55f).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(Setting->GetDisplayName()).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))] +
					SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SNew(SButton).IsEnabled(bEnabled).OnClicked(FOnClicked::CreateUObject(
						this, &ThisClass::AdjustScalarSetting, WeakSetting, -1))[SNew(STextBlock).Text(FText::FromString(TEXT("-")))]] +
					SHorizontalBox::Slot().FillWidth(0.35f).VAlign(VAlign_Center)[SNew(STextBlock)
																					  .Text_Lambda(
																						  [WeakSetting]()
																						  {
																							  return WeakSetting.IsValid()
																										 ? WeakSetting->GetFormattedText()
																										 : FText::GetEmpty();
																						  })
																					  .Justification(ETextJustify::Center)] +
					SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SNew(SButton).IsEnabled(bEnabled).OnClicked(FOnClicked::CreateUObject(
						this, &ThisClass::AdjustScalarSetting, WeakSetting, 1))[SNew(STextBlock).Text(FText::FromString(TEXT("+")))]]];
		}
	}

	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.01f, 0.018f, 0.03f, 0.99f))
		.Padding(
			24.0f)[SNew(SVerticalBox) +
				   SVerticalBox::Slot().AutoHeight().Padding(8.0f)[SNew(STextBlock)
																	   .Text(FText::FromString(TEXT("SETTINGS")))
																	   .Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
																	   .Justification(ETextJustify::Center)] +
				   SVerticalBox::Slot().FillHeight(1.0f).Padding(8.0f)[SNew(SScrollBox) + SScrollBox::Slot()[SettingsList]] +
				   SVerticalBox::Slot()
					   .AutoHeight()
					   .HAlign(HAlign_Center)
					   .Padding(
						   8.0f)[SNew(SHorizontalBox) +
								 SHorizontalBox::Slot().AutoWidth().Padding(
									 4.0f)[SunriseSettings::MakeButton(FText::FromString(TEXT("RESET DEFAULTS")),
									 FOnClicked::CreateUObject(this, &ThisClass::ResetToDefaults))] +
								 SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SunriseSettings::MakeButton(
									 FText::FromString(TEXT("APPLY")), FOnClicked::CreateUObject(this, &ThisClass::ApplyAndClose))] +
								 SHorizontalBox::Slot().AutoWidth().Padding(4.0f)[SunriseSettings::MakeButton(
									 FText::FromString(TEXT("CANCEL")), FOnClicked::CreateUObject(this, &ThisClass::CancelAndClose))]]];
}

FReply USunriseSettingsWidget::AdjustDiscreteSetting(TWeakObjectPtr<UGameSettingValueDiscrete> Setting, int32 Direction)
{
	if (UGameSettingValueDiscrete* Value = Setting.Get())
	{
		const TArray<FText> Options = Value->GetDiscreteOptions();
		if (!Options.IsEmpty())
		{
			const int32 Current = FMath::Max(0, Value->GetDiscreteOptionIndex());
			Value->SetDiscreteOptionByIndex((Current + Direction + Options.Num()) % Options.Num());
		}
	}
	return FReply::Handled();
}

FReply USunriseSettingsWidget::AdjustScalarSetting(TWeakObjectPtr<UGameSettingValueScalar> Setting, int32 Direction)
{
	if (UGameSettingValueScalar* Value = Setting.Get())
	{
		const double Step = FMath::Max(Value->GetNormalizedStepSize(), 0.01);
		Value->SetValueNormalized(FMath::Clamp(Value->GetValueNormalized() + Step * Direction, 0.0, 1.0));
	}
	return FReply::Handled();
}

FReply USunriseSettingsWidget::ResetToDefaults()
{
	for (UGameSettingValue* Value : EditableValues)
	{
		if (IsValid(Value) && Value->GetEditState().IsResetable())
		{
			Value->ResetToDefault();
		}
	}
	return FReply::Handled();
}

FReply USunriseSettingsWidget::ApplyAndClose()
{
	for (UGameSettingValue* Value : EditableValues)
	{
		if (IsValid(Value))
		{
			Value->Apply();
		}
	}
	if (Registry)
	{
		Registry->SaveChanges();
	}
	CloseAndRestoreMenu();
	return FReply::Handled();
}

FReply USunriseSettingsWidget::CancelAndClose()
{
	for (UGameSettingValue* Value : EditableValues)
	{
		if (IsValid(Value))
		{
			Value->RestoreToInitial();
		}
	}
	CloseAndRestoreMenu();
	return FReply::Handled();
}

void USunriseSettingsWidget::CloseAndRestoreMenu()
{
	RemoveFromParent();
	if (USunriseMainMenuWidget* Menu = OwningMenu.Get())
	{
		Menu->SetVisibility(ESlateVisibility::Visible);
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, Menu, EMouseLockMode::DoNotLock);
		}
	}
}
