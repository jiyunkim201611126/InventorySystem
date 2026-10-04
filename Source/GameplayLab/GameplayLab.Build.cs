// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameplayLab : ModuleRules
{
	public GameplayLab(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"StructUtils",
			"UMG",
			"Slate",
			"SlateCore",
			"CommonUI",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GameplayLab",
		});
	}
}
