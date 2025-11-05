using UnrealBuildTool;

public class MO56Editor : ModuleRules
{
    public MO56Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "MO56",
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "AssetTools",
                "AssetRegistry",
                "EditorScriptingUtilities",
                "EditorSubsystem",
                "Projects",
                "Slate",
                "SlateCore",
                "UMG",
                "Blutility",
                "PropertyEditor",
                "ContentBrowser",
                "DesktopPlatform",
                "ToolMenus"
            });
    }
}
