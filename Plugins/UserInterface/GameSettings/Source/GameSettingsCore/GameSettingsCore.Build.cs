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

		PublicDefinitions.AddRange(new string[]
		{
			"UE_VERSION_5_8_x=0"
		});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"PropertyPath",
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
		
#if UE_VERSION_5_8_x
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"PlatformDLC",
			}
		);
#endif

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
