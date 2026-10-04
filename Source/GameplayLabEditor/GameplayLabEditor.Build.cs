// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameplayLabEditor : ModuleRules
{
	public GameplayLabEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayLab",
			"UnrealEd",
			"EditorSubsystem",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"ToolMenus"
		});
	}
}
