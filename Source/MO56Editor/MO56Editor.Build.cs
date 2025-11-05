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
                "Slate",
                "SlateCore",
                "UMG",
                "Blutility",
                "UnrealEd",
                "AssetRegistry",
                "AssetTools",
                "EditorScriptingUtilities",
                "Projects",
                "PropertyEditor",
                "ToolMenus",
                "ContentBrowser",
                "DesktopPlatform",
                "InputCore",
                "ApplicationCore"
            });
    }
}
