// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TeamFrameworkCore : ModuleRules
{
	public TeamFrameworkCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"AIModule",
				"Niagara"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				
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
