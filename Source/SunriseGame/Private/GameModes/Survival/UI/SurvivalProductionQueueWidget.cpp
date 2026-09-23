#include "GameModes/Survival/UI/SurvivalProductionQueueWidget.h"

#include "GameFramework/GameStateBase.h"
#include "GameModes/Survival/Components/SurvivalProductionComponent.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SurvivalProductionQueueWidget)

namespace
{
	FString GetShortTagName(const FGameplayTag Tag)
	{
		FString Name = Tag.ToString();
		int32 Separator = INDEX_NONE;
		return Name.FindLastChar(TEXT('.'), Separator) ? Name.Mid(Separator + 1) : Name;
	}
} // namespace

void USurvivalProductionQueueWidget::SetProductionComponent(USurvivalProductionComponent* InProduction)
{
	ProductionComponent = InProduction;
}

TSharedRef<SWidget> USurvivalProductionQueueWidget::RebuildWidget()
{
	const TWeakObjectPtr<ThisClass> WeakThis(this);
	return SNew(SBox).MinDesiredWidth(210.0f).MaxDesiredWidth(
		300.0f)[SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.18f, 0.22f, 0.28f, 0.98f))
					.Padding(2.0f)[SNew(SBorder)
									   .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
									   .BorderBackgroundColor(FLinearColor(0.005f, 0.008f, 0.015f, 0.96f))
									   .Padding(FMargin(10.0f, 6.0f))[SNew(STextBlock)
																		  .Text_Lambda(
																			  [WeakThis]()
																			  {
																				  return WeakThis.IsValid() ? WeakThis->GetQueueText()
																											: FText::GetEmpty();
																			  })
																		  .ColorAndOpacity(FLinearColor(0.9f, 0.95f, 1.0f))
																		  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
																		  .Justification(ETextJustify::Center)]]];
}

FText USurvivalProductionQueueWidget::GetQueueText() const
{
	const USurvivalProductionComponent* Production = ProductionComponent.Get();
	if (!Production)
	{
		return FText::GetEmpty();
	}
	const TArray<FGameplayTag>& Queue = Production->GetQueue();
	if (Queue.IsEmpty())
	{
		return FText::FromString(TEXT("PRODUCTION\nIDLE"));
	}

	float RemainingSeconds = 0.0f;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		RemainingSeconds = FMath::Max(0.0f, Production->GetCurrentCompletionServerTime() - GameState->GetServerWorldTimeSeconds());
	}
	FString Text = FString::Printf(TEXT("PRODUCTION  %d\n1. %s  %.1fs"), Queue.Num(), *GetShortTagName(Queue[0]), RemainingSeconds);
	for (int32 Index = 1; Index < Queue.Num(); ++Index)
	{
		Text += FString::Printf(TEXT("\n%d. %s"), Index + 1, *GetShortTagName(Queue[Index]));
	}
	return FText::FromString(Text);
}
