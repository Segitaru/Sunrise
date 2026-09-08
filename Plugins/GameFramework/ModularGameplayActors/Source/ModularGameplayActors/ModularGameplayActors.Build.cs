// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO; // for Path

public class ModularGameplayActors : ModuleRules
{
	public ModularGameplayActors(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Public/AbilitySystem"),
				Path.Combine(ModuleDirectory, "Public/AI"),
				Path.Combine(ModuleDirectory, "Public/GameMode"),
				Path.Combine(ModuleDirectory, "Public/Pawn"),
				Path.Combine(ModuleDirectory, "Public/Player"),
				Path.Combine(ModuleDirectory, "Public/Input"),
				Path.Combine(ModuleDirectory, "Public/System"),
				Path.Combine(ModuleDirectory, "Public/Cosmetics"),
			}
		);
		
		PublicDefinitions.AddRange(new string[]
		{
			"UE_VERSION_5_8_x=0"
		});
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"CommonLoadingScreen",
				"GameplayTags",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UMG",
				"Slate",
				"CoreOnline",
				"NetCore",
				"ModularGameplay",
				"AIModule",
				"GameplayAbilities",

				"GameplayTasks",
				"GameFeatures",
				"EnhancedInput",
				"GameplayMessageRuntime",
				"GameSettingsCore",
				"CommonGame",
				"CommonUser",
				"CommonUI",
				"UIExtension",
				"Hotfix",
				"PhysicsCore",
				"DeveloperSettings",
				"EngineSettings",
				"GameSettingsCore",
			}
		);
		
		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);
	}
}
