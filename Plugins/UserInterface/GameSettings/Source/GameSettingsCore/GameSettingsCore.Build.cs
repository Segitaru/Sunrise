// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameSettingsCore : ModuleRules
{
	public GameSettingsCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;				
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"CommonInput",
				"CommonUI",
				"CommonGame",
				"GameplayTags",
				"GameSubtitles"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"PropertyPath",
				"PlatformDLC",
				"AudioModulation",
				"FoundationWidgetsCore",
				"CommonLoadingScreen",
				"DeveloperSettings",
				"EnhancedInput",
				"AudioMixer",
				"RHI",
				"RenderCore",
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
