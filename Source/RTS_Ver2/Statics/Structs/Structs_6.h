// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Settings/ProjectSettings.h"
#include "Structs_6.generated.h"

class AFogOfWarManager;
enum class ETeam : uint8;
enum class EFogStatus : uint8;


//================================================================================================
/* Include list: the following files include this file:
	- FogOfWarManager.h 
	- Ability_Nuke.h 
	- CAbility_Barrage.h */
//================================================================================================


/**
 *	A temporary fog reveal effect reveals the fog of war around a location for a certain
 *	amount of time. They can optionally reveal stealthed selectables too.
 *	Examples of these effects:
 *	- in C&C Generals: USA command center scan
 *	- in SCII: Terran's command center scan
 *
 *	They may not happen instantly but after a small delay. The reason for this is that suddenly
 *	revealing a location might look bad because replication would usually be turned off for actors
 *	in fog. Instead we 'prepare' the area we're about to reveal by turning on replication for
 *	the actors in the area first and then revealing the location. Note: I have not actually
 *	implemented any of this logic yet. It will probably come later when I choose to implement
 *	it for units/buildings etc. FIXME: implement this 'pre revealing' effect on this effect if 
 *	I end up implementing it for units/buildings etc 
 */
USTRUCT()
struct FTemporaryFogRevealEffectInfo
{
	GENERATED_BODY()

public:

	FTemporaryFogRevealEffectInfo()
		: Duration(14.f)
		, RevealRateCurve(nullptr)
		, RevealSpeed(2000.f)
		, FogRevealRadius(1800.f)
		, StealthRevealRadius(0.f)
	{
#if WITH_EDITOR
		ConstructorOnPostEdit();
#endif
	}

	FTemporaryFogRevealEffectInfo(const FTemporaryFogRevealEffectInfo & Other, FVector2D InLocation)
		: Duration(Other.Duration)
		, RevealRateCurve(Other.RevealRateCurve)
		, RevealSpeed(Other.RevealSpeed)
		, FogRevealRadius(Other.FogRevealRadius)
		, StealthRevealRadius(Other.StealthRevealRadius)
		, TimeExisted(0.f)
		, Location(InLocation)
	{
	}

	/**
	 *	Creates this effect at a location in the world for a single player, so only Player will
	 *	have the area of fog revealed.
	 *
	 *	@param WorldLocation - 2D world coords of where to create effect
	 *	@param FogManager - fog of war manager
	 *	@param Player - player state of the player this effect is for
	 */
	//void CreateForPlayer(AFogOfWarManager * FogManager, FVector2D WorldLocation, ARTSPlayerState * Player) const;

#if GAME_THREAD_FOG_OF_WAR
	/**
	 *	Creates this effect at a location in the world for a team, so everyone on the team will
	 *	have the area of fog revealed.
	 */
	void CreateForTeam(AFogOfWarManager * FogManager, FVector2D WorldLocation, ETeam Team, 
		bool bIsServer, bool bTeamIsLocalPlayersTeam) const;
#elif MULTITHREADED_FOG_OF_WAR
	/* This version is for if using multithreaded fog of war manager */
	void CreateForTeam(FVector2D WorldLocation, ETeam Team, bool bIsServer, bool bTeamIsLocalPlayersTeam) const;
#endif

	void Tick(float DeltaTime, float & OutFogRevealRadius, float & OutStealthRevealRadius);

	/** 
	 *	Set the duration of the reveal. Calling this after the effect has been sent to the fog 
	 *	manager will not do anything 
	 */
	void SetDuration(float InDuration) { Duration = InDuration; }

	/* Returns the world location where the effect is */
	FVector2D GetLocation() const;

	/* Returns true if the effect has been around long enough that it can expire */
	bool HasCompleted() const;

protected:

	//---------------------------------------------------------------
	//	Data
	//---------------------------------------------------------------

	/** How long this effect lasts for */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float Duration;

	/**
	 *	The curve that defines how fast fog is revealed. This should be normalized on each axis.
	 *	X axis = time (normalized to range [0, 1])
	 *	Y axis = % of fog revealed (normalized to range [0, 1]). So 1 implies the full amount
	 *	of fog is being revealed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * RevealRateCurve;

	/**
	 *	How fast fog is revealed measured in unreal units per second. Only relevant if not using 
	 *	a curve.
	 *
	 *	0 is code for 'all fog is revealed instantly'
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditNotUsingCurveProperties))
	float RevealSpeed;

	/** Radius of circle of fog that is revealed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float FogRevealRadius;

	/**
	 *	The radius that stealth mode units are revealed
	 *	0 = will not reveal stealth mode units
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float StealthRevealRadius;

	/* [State variable] How long this effect has been running for */
	float TimeExisted;

	/* [State variable] Location where the effect was spawned. 2D because fog manager does not 
	care about height */
	FVector2D Location;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditNotUsingCurveProperties;
#endif

public:

#if WITH_EDITOR
	/* Currently this only modifies EditCondition bools so isn't strictly required to be called */
	void OnPostEdit()
	{
		bCanEditNotUsingCurveProperties = (RevealRateCurve == nullptr);
	}

	void ConstructorOnPostEdit()
	{
		OnPostEdit();
	}
#endif
};


/* Array of FTemporaryFogRevealEffectInfo */
USTRUCT()
struct FTemporaryFogRevealEffectsArray
{
	GENERATED_BODY()

	TArray <FTemporaryFogRevealEffectInfo> Array;

	FTemporaryFogRevealEffectsArray() { }

	FTemporaryFogRevealEffectsArray(int32 ReserveAmount)
	{
		Array.Reserve(ReserveAmount);
	}
};


/** 
 *	Info to spawn a decal at a location for a period of time. 
 *	Examples of use: launching a nuke and want the area where it is landing to have a decal there 
 */
USTRUCT()
struct FSpawnDecalInfo
{
	GENERATED_BODY()

public:

	FSpawnDecalInfo()
		: Decal(nullptr)
		, Size(FVector(100.f, 512.f, 512.f))
		, Rotation(FRotator(-90.f, 0.f, 0.f))
		, Duration(10.f)
	{
	}

	UMaterialInterface * GetDecal() const { return Decal; }
	const FVector & GetSize() const { return Size; }
	const FRotator & GetRotation() const { return Rotation; }
	float GetDuration() const { return Duration; }

protected:

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UMaterialInterface * Decal;

	/* Remember: Z axis value (which is actually X here after rotation happens) 
	might significantly affect performance. High values means the decal 
	draws up/down hills but will cost more in performance. If your landscape is completely flat 
	then this can be something like 1 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector Size;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FRotator Rotation;

	/* How long the decal lasts before disappearing */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Duration;
};
