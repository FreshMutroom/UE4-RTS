// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorPlaySessionStructs.h"


FPIEPlayerInfo::FPIEPlayerInfo()
	: Team(ETeam::Team1) 
	, Faction(static_cast<EFaction>(1)) 
	, StartingSpot(-1)
{
}

FPIEPlayerInfo::FPIEPlayerInfo(ETeam InTeam, EFaction InFaction)
{
	Team = InTeam;
	Faction = InFaction;
	StartingSpot = -1;
}

FPIECPUPlayerInfo::FPIECPUPlayerInfo() 
	: FPIEPlayerInfo() 
	, Difficulty(ECPUDifficulty::DoesNothing)
{
}

FPIECPUPlayerInfo::FPIECPUPlayerInfo(ETeam InTeam, EFaction InFaction, ECPUDifficulty InDifficulty)
{
	Team = InTeam;
	Faction = InFaction;
	Difficulty = InDifficulty;
}
