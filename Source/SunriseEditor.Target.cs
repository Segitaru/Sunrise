// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SunriseEditorTarget : TargetRules
{
	public SunriseEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		IncludeOrderVersion = EngineIncludeOrderVersion.Oldest;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("SunriseGame");
	}
}
