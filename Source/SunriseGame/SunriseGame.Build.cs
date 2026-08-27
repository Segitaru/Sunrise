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
			"InputCore",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"ExperienceFrameworkCore",
			"TeamFrameworkCore",
			"GameSettingsCore",
			"GameplayAbilities",
			"GameplayTags",
			"ModularGameplay",
			"EnhancedInput",
			"ModularGameplayActors"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
