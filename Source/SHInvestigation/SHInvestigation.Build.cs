// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SHInvestigation : ModuleRules
{
	public SHInvestigation(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SHInvestigation",
			"SHInvestigation/Variant_Platforming",
			"SHInvestigation/Variant_Platforming/Animation",
			"SHInvestigation/Variant_Combat",
			"SHInvestigation/Variant_Combat/AI",
			"SHInvestigation/Variant_Combat/Animation",
			"SHInvestigation/Variant_Combat/Gameplay",
			"SHInvestigation/Variant_Combat/Interfaces",
			"SHInvestigation/Variant_Combat/UI",
			"SHInvestigation/Variant_SideScrolling",
			"SHInvestigation/Variant_SideScrolling/AI",
			"SHInvestigation/Variant_SideScrolling/Gameplay",
			"SHInvestigation/Variant_SideScrolling/Interfaces",
			"SHInvestigation/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
