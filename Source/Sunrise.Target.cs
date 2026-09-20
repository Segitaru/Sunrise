// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SunriseTarget : TargetRules
{
	public SunriseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		IncludeOrderVersion = EngineIncludeOrderVersion.Oldest;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("SunriseGame");
	}
}
