// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MO56EditorTarget : TargetRules
{
    public MO56EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        CppStandard = CppStandardVersion.Cpp20;

        bUseUnityBuild = true;
        bUsePCHFiles = true;

        // Only the runtime game module here
        ExtraModuleNames.Clear();
        ExtraModuleNames.Add("MO56");
    }
}
