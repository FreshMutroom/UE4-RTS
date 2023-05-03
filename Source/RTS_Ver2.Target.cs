// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class RTS_Ver2Target : TargetRules
{
	public RTS_Ver2Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        bIWYU = true;
        bEnforceIWYU = true;
        bUsesSteam = true;

        ExtraModuleNames.AddRange( new string[] { "RTS_Ver2" } );

        // Note I have Type not Target.Type cause I cannot compile otherwise. Just Type is ok right?
        if (Type == TargetType.Editor)
        {
            ExtraModuleNames.AddRange(new string[] { "RTS_Ver2Editor" });
        }
    }
}
