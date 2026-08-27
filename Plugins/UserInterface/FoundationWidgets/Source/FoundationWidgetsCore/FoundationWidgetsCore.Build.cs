// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FoundationWidgetsCore : ModuleRules
{
	public FoundationWidgetsCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"CommonInput",
				"CommonUI",
				"GameplayTags",
				"CommonGame",
				"DeveloperSettings",
				"EnhancedInput",
				"ApplicationCore"
			}
		);

		if (Target.bBuildEditor)
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"BlueprintGraph"
				}
			);

		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);
	}
}