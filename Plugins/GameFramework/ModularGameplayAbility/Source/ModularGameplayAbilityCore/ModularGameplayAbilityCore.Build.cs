// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ModularGameplayAbilityCore : ModuleRules
{
	public ModularGameplayAbilityCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
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
