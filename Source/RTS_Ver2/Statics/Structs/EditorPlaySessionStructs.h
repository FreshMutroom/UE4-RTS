// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorPlaySessionStructs.generated.h"

enum class ETeam : uint8;
enum class EFaction : uint8;
enum class ECPUDifficulty : uint8;


//================================================================================================
//	Structs for when doing PIE/standalone
//================================================================================================


/** Info about a player for PIE/standalone */
USTRUCT()
struct FPIEPlayerInfo
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ETeam Team;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EFaction Faction;

	/** Use -1 to assign to no starting spot */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = -1))
	int16 StartingSpot;

public:

	FORCEINLINE ETeam GetTeam() const { return Team; }
	FORCEINLINE EFaction GetFaction() const { return Faction; }
	FORCEINLINE int16 GetStartingSpot() const { return StartingSpot; }

	FPIEPlayerInfo();
	FPIEPlayerInfo(ETeam InTeam, EFaction InFaction);
};


/** Info about CPU player for PIE */
USTRUCT()
struct FPIECPUPlayerInfo : public FPIEPlayerInfo
{
	GENERATED_BODY()

public:

	/* Difficulty of CPU player */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ECPUDifficulty Difficulty;

public:

	FORCEINLINE ECPUDifficulty GetDifficulty() const { return Difficulty; }

	FPIECPUPlayerInfo();
	FPIECPUPlayerInfo(ETeam InTeam, EFaction InFaction, ECPUDifficulty InDifficulty);
};
