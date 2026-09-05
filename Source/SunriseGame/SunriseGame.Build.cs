// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SunriseGame : ModuleRules
{
	public SunriseGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			"SunriseGame"
		});
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			
		});
		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"SunriseTeamFrameworkCore",
			"GameSettingsCore",
			"GameplayAbilities",
			"GameplayTags",
			"ModularGameplay",
			"EnhancedInput",
			"ModularGameplayActors",
			"GameplayMessageRuntime",
			"GameFeatures",
			"DeveloperSettings",
			"NetCore"
		});
		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);
		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
