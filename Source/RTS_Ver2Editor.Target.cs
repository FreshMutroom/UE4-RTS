// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class RTS_Ver2EditorTarget : TargetRules
{
	public RTS_Ver2EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

        bIWYU = true;
        bEnforceIWYU = true;

        ExtraModuleNames.AddRange( new string[] { "RTS_Ver2" } );

        ExtraModuleNames.AddRange(new string[] { "RTS_Ver2Editor" });
    }
}
