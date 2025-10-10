using UnrealBuildTool;

public class OptimizationHelper : ModuleRules
{
    public OptimizationHelper(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        { 
            "UnrealEd",
            "AssetRegistry",
            "Slate",              // Added for UI
            "SlateCore",          // Added for UI
            "InputCore",          // Added for UI
            "ToolMenus"           // Added for menu integration
        });
    }
}