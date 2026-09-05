// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_EDITORONLY_DATA

#include "Rosters/Development/GameRostersDeveloperSettings.h"

UGameRostersDeveloperSettings::UGameRostersDeveloperSettings()
{
}

FName UGameRostersDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#endif