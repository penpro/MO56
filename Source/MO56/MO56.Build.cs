// Copyright Epic Games, Inc. All Rights Reserved.
// MO56.Build.cs

using UnrealBuildTool;

public class MO56 : ModuleRules
{
	public MO56(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
            "MOInventory",

            "Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MO56",
			"MO56/Variant_Platforming",
			"MO56/Variant_Platforming/Animation",
			"MO56/Variant_Combat",
			"MO56/Variant_Combat/AI",
			"MO56/Variant_Combat/Animation",
			"MO56/Variant_Combat/Gameplay",
			"MO56/Variant_Combat/Interfaces",
			"MO56/Variant_Combat/UI",
			"MO56/Variant_SideScrolling",
			"MO56/Variant_SideScrolling/AI",
			"MO56/Variant_SideScrolling/Gameplay",
			"MO56/Variant_SideScrolling/Interfaces",
			"MO56/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
