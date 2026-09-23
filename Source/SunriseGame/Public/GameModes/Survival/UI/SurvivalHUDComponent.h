#pragma once

#include "UI/Components/SunriseHUDComponent.h"

#include "SurvivalHUDComponent.generated.h"

class ASunriseHUD;

/** Asset-independent HUD for the Survival vertical slice. */
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SUNRISEGAME_API USurvivalHUDComponent : public USunriseHUDComponent
{
	GENERATED_BODY()

public:
	virtual void DrawHUD(ASunriseHUD* HUD) override;
};
