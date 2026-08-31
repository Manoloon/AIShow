// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AIShow : ModuleRules
{
	public AIShow(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore","NavigationSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "SmartObjectsModule", "AIModule" , "GameplayTasks", "GameplayTags","AIModule","GameplayBehaviorSmartObjectsModule" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
