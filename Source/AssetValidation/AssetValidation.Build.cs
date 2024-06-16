// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AssetValidation : ModuleRules
{
	public AssetValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;
		bUseUnity = false;
		
		PrivateDefinitions.AddRange(new string[]
		{
            "WITH_ASSET_VALIDATION_TESTS=1"
		});
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
				"DataValidation",
				"EditorSubsystem",
				"GameplayAbilities",
				"GameplayTags",
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
				"DeveloperToolSettings",
				"SourceControl",
				"DeveloperSettings",
				"Kismet",
				"SubobjectDataInterface", 
				"SubobjectEditor",
				"AdvancedWidgets", 
				"StudioTelemetry",
				"TraceLog",
				"UMG",
				"UMGEditor", 
				"AIModule",
				"StructUtils",
			}
		);
	}
}
