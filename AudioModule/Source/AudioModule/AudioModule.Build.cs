// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AudioModule : ModuleRules
{
	public AudioModule(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AudioMixer" });
		PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Projects" });

		bEnableExceptions = true;
	}
}
