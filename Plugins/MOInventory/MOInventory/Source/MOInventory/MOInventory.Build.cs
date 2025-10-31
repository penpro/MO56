using UnrealBuildTool;

//MOInventory.Build.cs


public class MOInventory : ModuleRules
{
    public MOInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore",
            "MOItems" // needed to see UItemData across modules
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "PhysicsCore"
        });
    }
}
