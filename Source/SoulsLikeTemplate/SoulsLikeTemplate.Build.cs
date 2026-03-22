// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SoulsLikeTemplate : ModuleRules
{
	public SoulsLikeTemplate(ReadOnlyTargetRules Target) : base(Target)
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
			"SoulsLikeTemplate",
			"SoulsLikeTemplate/Variant_Platforming",
			"SoulsLikeTemplate/Variant_Platforming/Animation",
			"SoulsLikeTemplate/Variant_Combat",
			"SoulsLikeTemplate/Variant_Combat/AI",
			"SoulsLikeTemplate/Variant_Combat/Animation",
			"SoulsLikeTemplate/Variant_Combat/Gameplay",
			"SoulsLikeTemplate/Variant_Combat/Interfaces",
			"SoulsLikeTemplate/Variant_Combat/UI",
			"SoulsLikeTemplate/Variant_SideScrolling",
			"SoulsLikeTemplate/Variant_SideScrolling/AI",
			"SoulsLikeTemplate/Variant_SideScrolling/Gameplay",
			"SoulsLikeTemplate/Variant_SideScrolling/Interfaces",
			"SoulsLikeTemplate/Variant_SideScrolling/UI",
			"SoulsLikeTemplate/Variant_SoulsLike",
			"SoulsLikeTemplate/Variant_SoulsLike/Interfaces",
			"SoulsLikeTemplate/Variant_SoulsLike/Components",
			"SoulsLikeTemplate/Variant_SoulsLike/Data",
			"SoulsLikeTemplate/Variant_SoulsLike/Weapons",
			"SoulsLikeTemplate/Variant_SoulsLike/Characters",
			"SoulsLikeTemplate/Variant_SoulsLike/Animation",
			"SoulsLikeTemplate/Variant_SoulsLike/AI",
			"SoulsLikeTemplate/Variant_SoulsLike/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
