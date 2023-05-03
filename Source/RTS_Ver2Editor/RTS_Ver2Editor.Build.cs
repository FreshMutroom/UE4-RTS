// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class RTS_Ver2Editor : ModuleRules
{
	public RTS_Ver2Editor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // When I made this file I just copied my RTS_Ver2.Build.cs file but I commented out 
        // pretty much everything to see what I could get away with. In the future if I 
        // run into linker errors then I might need to add more modules here

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UnrealEd" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Blutility", "UMG" });
        PublicDependencyModuleNames.AddRange(new string[] { "RTS_Ver2" });
    }
}
