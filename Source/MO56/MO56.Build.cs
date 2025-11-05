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
                        "MOItems",
                        "Slate",
                        "SlateCore",
                        "PhysicsCore"
                });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MO56"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
