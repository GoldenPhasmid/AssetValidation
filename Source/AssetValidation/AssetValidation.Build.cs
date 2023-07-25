// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AssetValidation : ModuleRules
{
	public AssetValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"DataValidation",
				"EditorSubsystem",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"Engine",
			}
		);
	}
}
