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
			"ModularGameplayActors",
			"ModularGameplay",
			"GameplayAbilities",
			"GameplayTags",
			"CommonGame",
			"AsyncMixin",
		});
		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"GameSettingsCore",
			"EnhancedInput",
			"CommonUser",
			"GameplayMessageRuntime",
			"GameFeatures",
			"DeveloperSettings",
			"NetCore",
			"NPC_Optimizator"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", });
		}
		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);
		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
