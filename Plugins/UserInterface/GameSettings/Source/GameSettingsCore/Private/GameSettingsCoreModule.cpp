// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"

/**
 * Implements the FGameSettingsCoreModule module.
 */
class FGameSettingsCoreModule : public IModuleInterface
{
public:
	FGameSettingsCoreModule();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};


FGameSettingsCoreModule::FGameSettingsCoreModule()
{
}

void FGameSettingsCoreModule::StartupModule()
{
}

void FGameSettingsCoreModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FGameSettingsCoreModule, GameSettingsCore);
