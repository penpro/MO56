// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MO56Target : TargetRules
{
	public MO56Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		CppStandard = CppStandardVersion.Cpp20;

		bUseUnityBuild = true;
		bUsePCHFiles = true;

		ExtraModuleNames.Add("MO56");
	}
}
