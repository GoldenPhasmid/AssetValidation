// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AssetValidation : ModuleRules
{
	public AssetValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		
		PrivateDefinitions.AddRange(new string[]
		{
            "WITH_ASSET_VALIDATION_TESTS=1"
		});
		
		if (Target.Version.MinorVersion >= 5)
		{
			PrivateDefinitions.Add("AUTOTEST_APPLICATION_MASK=EAutomationTestFlags_ApplicationContextMask");
		}
		else
		{
			PrivateDefinitions.Add("AUTOTEST_APPLICATION_MASK=EAutomationTestFlags::ApplicationContextMask");
		}
		
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
				"MessageLog",
				"BlueprintGraph", 
				// "ScriptPlugin",
				"AssetTools", 
				"AssetReferenceRestrictions", 
				"Blutility",
				"AssetManagerEditor", 
				// "ScriptPlugin",
			}
		);
	}
}
