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
            "Slate",
            "SlateCore",
            "InputCore",
            "ToolMenus",
            "ContentBrowser",
            "BlueprintGraph",      // ← ДОБАВИТЬ для работы с BP
            "Kismet",              // ← ДОБАВИТЬ для Blueprint
            "KismetCompiler",      // ← ДОБАВИТЬ для анализа BP
            "GraphEditor"          // ← ДОБАВИТЬ для EdGraph
        });
    }
}