using UnrealBuildTool;

public class QuickNodeLayout : ModuleRules
{
    public QuickNodeLayout(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "Slate", "SlateCore",
            "ToolMenus", "Kismet", "BlueprintGraph", "GraphEditor"
        });
    }
}
