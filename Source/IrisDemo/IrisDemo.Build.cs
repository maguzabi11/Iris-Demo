// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IrisDemo : ModuleRules
{
	public IrisDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		SetupIrisSupport(Target);

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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"IrisCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"IrisDemo",
			"IrisDemo/Variant_Platforming",
			"IrisDemo/Variant_Platforming/Animation",
			"IrisDemo/Variant_Combat",
			"IrisDemo/Variant_Combat/AI",
			"IrisDemo/Variant_Combat/Animation",
			"IrisDemo/Variant_Combat/Gameplay",
			"IrisDemo/Variant_Combat/Interfaces",
			"IrisDemo/Variant_Combat/UI",
			"IrisDemo/Variant_SideScrolling",
			"IrisDemo/Variant_SideScrolling/AI",
			"IrisDemo/Variant_SideScrolling/Gameplay",
			"IrisDemo/Variant_SideScrolling/Interfaces",
			"IrisDemo/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
