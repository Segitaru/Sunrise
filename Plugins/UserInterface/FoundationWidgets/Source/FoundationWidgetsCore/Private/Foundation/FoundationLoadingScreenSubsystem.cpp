// Copyright Epic Games, Inc. All Rights Reserved.

#include "Foundation/FoundationLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FoundationLoadingScreenSubsystem)

class UUserWidget;

//////////////////////////////////////////////////////////////////////
// UFoundationLoadingScreenSubsystem

UFoundationLoadingScreenSubsystem::UFoundationLoadingScreenSubsystem()
{
}

void UFoundationLoadingScreenSubsystem::SetLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (LoadingScreenWidgetClass != NewWidgetClass)
	{
		LoadingScreenWidgetClass = NewWidgetClass;

		OnLoadingScreenWidgetChanged.Broadcast(LoadingScreenWidgetClass);
	}
}

TSubclassOf<UUserWidget> UFoundationLoadingScreenSubsystem::GetLoadingScreenContentWidget() const
{
	return LoadingScreenWidgetClass;
}
