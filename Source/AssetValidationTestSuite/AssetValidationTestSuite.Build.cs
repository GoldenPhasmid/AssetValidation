using UnrealBuildTool;

public class AssetValidationTestSuite : ModuleRules
{
    public AssetValidationTestSuite(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "AssetValidation",
                "AIModule",
                "StructUtils",
                "GameplayAbilities",
                "GameplayTags",
                "UnrealEd",
            }
        );
    }
}