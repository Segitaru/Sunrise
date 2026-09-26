#pragma once

#include "Blueprint/UserWidget.h"

#include "SurvivalProductionQueueWidget.generated.h"

class USurvivalProductionComponent;

UCLASS()
class SUNRISEGAME_API USurvivalProductionQueueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetProductionComponent(USurvivalProductionComponent* InProduction);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FText GetQueueText() const;

	TWeakObjectPtr<USurvivalProductionComponent> ProductionComponent;
};