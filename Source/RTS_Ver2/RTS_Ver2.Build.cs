// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RTS_Ver2 : ModuleRules
{
	public RTS_Ver2(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Added NavigationSystem for 4.20 to support vehicle AI controller
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "RHI", "RenderCore", "PhysXVehicles", "NavigationSystem" });

        PrivateDependencyModuleNames.AddRange(new string[] { "ReplicationGraph" }); 

        // Uncomment if you are using Slate UI. ETextCommit needed this 
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // if means "Exclude from packaged builds"
        if (Target.Type == TargetRules.TargetType.Editor)
        {
            // UnrealEd added for ULevelEditorPlaySettings
            // UMGEditor added for UWidgetBlueprint
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor" }); 
        }

        // Uncomment if you are using online features
        PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
