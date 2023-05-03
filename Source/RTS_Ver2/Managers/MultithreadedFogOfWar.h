// Fill out your copyright notice in the Description page of Project Settings.

/* A lot to do. Multithreaded fog is not even close to being finished. 
Using a test project to test out threads. 
For now I am just using game thread fog (by setting preproc MULTITHREADED_FOG_OF_WAR to 0 in 
ProjectSettings.h) */

// TODO Dynamic fog sounds

#pragma once

#if MULTITHREADED_FOG_OF_WAR

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

#include "Managers/MultithreadedFogOfWarTypes.h"

class MultithreadedFogOfWarManager;
class ServerFogOfWarThread;
class ClientFogOfWarThread;
class ARTSGameState;
class AInfantry;
class ABuilding;
enum class EFogStatus : uint8;
class UMaterialInstanceDynamic;
class APostProcessVolume;


/* Assert that runs on non-game threads. May need to be different than the regular assert hence 
this macro */
#define threadAssert(x) check(x)


/* These 2 preprocs are just for testing only. I should delete them when multithreaded fog 
of war is fully working. Provided multithreaded fog of war is enabled in ProjectSettings.h 
then these say which machines it is enabled on */
#define MULTITHREADED_ON_SERVER 0
#define MULTITHREADED_ON_CLIENTS 1


/* 

Planning: 

Options that can go in ProjectSettings.h:
- GAME_THREAD_FOG_OF_WAR: whether fog of war is calculated entierely on the game thread
- MULTITHREADED_FOG_OF_WAR_IMPLEMENTATION_VERSION: I will use this just in case I decide on a 
better way to do fog. 

Variables: 
- TArray preallocated with the size of MAX_NUM_SELECTABLES (255) that holds the fog info for 
infantry. That is: a FVector2D for world location, float for fog reveal radius and a float for 
stealth reveal radius. Need one of these arrays for each team. Could go with a non TArray for 
the team array.

- NumTeams locks which are for accessing the infantry TArrays, DeltaTime and the 'recently destroyed buildings' array
- NumTeams locks which are for accessing the infantry visibility info

*/


/* 
Diagram for CLIENTS of flow: 
Stuff on the same tier can be completed asynchronously

annnd I just realized the queries like IsLocationInFog() that happen in AI controllers or 
whatnot will need to query the tile info

										ResetTileInfo(OurTeam)
										        |
												|
												|
		  ____________________________________DONE____________________________________
		 |                                      |                                     |
		 |										|									  |
		 |										|									  |
 Reveal tiles from infantry			Reveal tiles from buildings            Reveal tiles from temporary reveal effects
		 |										|									  |
		 |		These cant actually be done     |									  |
		 |      in parallel	unless assume       |                                     |
		 |		relaxed atomics					|									  |
		DONE                                   DONE                                  DONE
		 |										|									  |
		 |										|									  |
		 |______________________________________|_____________________________________|
												| 
									            |
                  ______________________________|______________________________
				  |                             |                              |
				  |                             |                              |
				  |								|							   |
	Calc and store visibility info           Copy tiles array       Queue render command to render fog of war
	Can be done async for each team             |							
				  |                             |
				  |                             |
				  |                            DONE
				 DONE                           |
				  |                             |
				  |                   Tiles array can now be accessed by game thread
				  |
	Visibility info can now be accessed by game thread 


	So in regards to vis info I guess in each actor's tick it will query the Visibility info 
	and set its vis (both its actualy visibility and a EFogStatus variable on it saying what 
	its fog staus is). The plus side is we never need to query a fog manager to know whether 
	a selectable is visible - instead just check the EFogStatus value or its root comp 
	visibility maybe.
*/

//--------------------------------------------------------------------------------------------
//============================================================================================
//	------- Fog Manager Class -------
//============================================================================================
//--------------------------------------------------------------------------------------------


/* TODO: implement the whole acknowledging of selectable IDs becoming freed due to a selectable 
becoming destroyed. So basically what this is is the fog manager will need to ack that the 
selectable has been destroyed because otherwise it may send some vision info back to the game 
thread: it could be a huge reveal radius but the ID has now been assigned to some small radius 
revealing selectable so it would not look right. 

TODO: fix up names in this file, and very thouroughly check code */
class RTS_VER2_API MultithreadedFogOfWarManager
{
public:

	/* Fog of war manager */
	static MultithreadedFogOfWarManager FogManager;

	static MultithreadedFogOfWarManager & Get()
	{
		return FogManager;
	}

	/**
	 *	Setup for match. 
	 *	
	 *	@param InNumTeams - how many teams are in the match 
	 *	@param InLocalPlayersTeamIndex - team index of the local player  
	 */
	void Setup(UWorld * World, ARTSLevelVolume * FogVolume, uint8 InNumTeams, 
		uint8 InLocalPlayersTeamIndex, UMaterialInterface * FogOfWarMaterial);

	/**
	 *	Return how many fog of war threads to create
	 */
	int32 GetNumFogOfWarThreadsToCreate() const;

	/** 
	 *	This function can be called from any game thread tick. It essentially handles all the 
	 *	behavior required for doing fog of war. 
	 *
	 *	@param DeltaTime - AActor::Tick's DeltaTime param
	 */
	void OnGameThreadTick(float DeltaTime);
	
	/* Called by game thread when a building has been destroyed */
	void OnBuildingDestroyed(ABuilding * Building);

	void RegisterTeamTemporaryRevealEffect(const FTemporaryFogRevealEffectInfo & Effect, FVector2D WorldLocation, ETeam Team);

	/**
	 *	Utility function just like AFogOfWarManager's version 
	 *	
	 *	@param Location - 2D location to reveal fog around 
	 *	@param SightRadius - radius of fog revealing 
	 *	@param StealthRevealRadius - radius of stealth revealing 
	 *	@param Tiles - the team's tiles array 
	 */
	void RevealTilesAroundLocation(const FVector2D & Location, float SightRadius, float StealthRevealRadius,
		TArray<EFogStatus> & Tiles);

	/** 
	 *	Utility function that returns the fog status of a location 
	 *	
	 *	@param Location - location to query 
	 *	@param Tiles - tiles array belonging to the team you want to query for 
	 */
	EFogStatus GetLocationFogStatus(const FVector2D & Location, const TArray<EFogStatus> & Tiles) const;

	/* @param TeamIndex - team that completed all its tasks */
	void OnAllTasksComplete(uint8 TeamIndex);

	void AddRecentlyCreatedBuilding(ABuilding * Building);

protected:

	int32 WorldLocationToTilesIndex(const FVector2D & WorldLocation) const;

	/* Convert a world location to a fog of war grid location */
	FIntPoint GetFogGridCoords(const FVector & WorldLocation) const;
	FIntPoint GetFogGridCoords(const FVector2D & WorldLocation) const;

	FORCEINLINE static EFogStatus BitwiseOrOperator(const EFogStatus & Enum1, const EFogStatus & Enum2);
	FORCEINLINE static EFogStatus BitwiseOrOperator(const EFogStatus & Enum1, const uint8 & Enum2AsInt);

public:

	//-------------------------------------------------------------------------------------------
	//	Common queries the game thread might have. 
	//	These functions do not aquire any mutexes so do not feel guilty querying them at any time.
	//-------------------------------------------------------------------------------------------

	/** 
	 *	Returns true if a world location is visible to the local player 
	 *	
	 *	@param Location - world location 
	 *	@return - true if the location is outside fog of war for the local player 
	 */
	bool IsLocationLocallyVisible(const FVector & Location) const;

	/** 
	 *	Returns true if a world location is visible from the perspective of a team. This function will check 
	 *	if the location is within the fog volume bounds limits and if it is outside then it 
	 *	will return false 
	 *
	 *	@param Team - team we're checking visibility against
	 */
	bool IsLocationVisibleNotChecked(const FVector & Location, ETeam Team) const;

	/** 
	 *	Return true if a selectable is outside fog of war from the perspective of a team 
	 *	
	 *	@param Team - team whose perspective we are using when determining whether the selectable is 
	 *	outside fog or not 
	 */
	bool IsSelectableOutsideFog(const ISelectable * Selectable, ETeam Team) const;

private:
	
	FORCEINLINE int32 GetTileIndex(int32 X, int32 Y) const;

	friend CalculationProgress;
	friend ServerFogOfWarThread;
	friend ClientFogOfWarThread;

	//-----------------------------------------------------------------------------
	//	Data the game thread can access at any time 
	//-----------------------------------------------------------------------------

	// Texture buffer for rendering fog of war. Local player only because we only render fog for them
	uint8 * TextureBuffer;

	/* Each team's buildings that have been recently placed */
	BuildingFogInfoArrayBasic RecentlyCreatedBuildings[ProjectSettings::MAX_NUM_TEAMS];

	/* Each team's buildings that have had a modification done to them that fog manager needs 
	to know about e.g. their sight radius is now bigger */
	BuildingFogInfoArrayBasic RecentlyModifiedBuildings[ProjectSettings::MAX_NUM_TEAMS];

	/* Each team's buildings that were recently destroyed */
	SelectableIDArray RecentlyDestroyedBuildings[ProjectSettings::MAX_NUM_TEAMS];

	/* Holds arrays of all the infantry on a team. 
	Make sure infantry add themselves to this when they start revealing fog and remove themselves 
	when they want to stop revealing fog (usually when they are destroyed) */
	InfantryArray TeamsInfantry[ProjectSettings::MAX_NUM_TEAMS];

	/* Each team's temporary reveal effects (such as a from placing a superweapon at a location) 
	that were recently created and yet to enter the fog system */
	TempRevealEffectArray_NoLock RecentlyCreatedTemporaryRevealEffects[ProjectSettings::MAX_NUM_TEAMS];

	/* Visibility of every tile */
	TilesArray TilesArrays[ProjectSettings::MAX_NUM_TEAMS];

	// [Team we are checking from][Team of selecable we are checking]
	TeamVisibilityInfoStruct TeamVisibilityInfo[ProjectSettings::MAX_NUM_TEAMS][ProjectSettings::MAX_NUM_TEAMS];

	//----------------------------------------------------------------------
	//	Data game thread should never access
	//----------------------------------------------------------------------

	/* Whether machine is server (listen or dedicated) */
	uint8 bIsServer : 1;
	uint8 bHasSetup : 1;

	uint8 LocalPlayersTeamIndex;
	uint8 NumTeams;

	/* Threads that are idle. If on server then the client version will be empty and vice versa */
	ServerThreadArray ServerIdleThreads; 
	ClientThreadArray ClientIdleThreads;

	/* Array of how far through calculations for each team we are.
	Key = Statics::TeamToArrayIndex */
	CalculationProgress TeamProgress[ProjectSettings::MAX_NUM_TEAMS];

	TilesArray_NoLock TeamTilesShared[ProjectSettings::MAX_NUM_TEAMS];

	/* Array of arrays. Contains each team's infantry state info. 
	Key = Statics::TeamToArrayIndex */
	InfantryFogInfoArray InfantryStatesShared[ProjectSettings::MAX_NUM_TEAMS];
	BuildingFogInfoArray BuildingStatesShared[ProjectSettings::MAX_NUM_TEAMS];
	TempRevealEffectArray TemporaryRevealEffectsShared[ProjectSettings::MAX_NUM_TEAMS];
	InfantryFogInfoBasicArray InfantryStatesBasicShared[ProjectSettings::MAX_NUM_TEAMS];
	/* Contains selectable IDs of buildings that want to be removed from fog calculations
	(probably because they were destroyed) */
	DestroyedBuildingArray DestroyedBuildingsShared[ProjectSettings::MAX_NUM_TEAMS];

	/* Contains each team's visibility info */
	TeamVisibilityInfoStruct VisibilityInfoShared[ProjectSettings::MAX_NUM_TEAMS];

	float DeltaTime;
	float DeltaTimeNotThreadSafe;

	//------------------------------------------------------------------
	//	Map Data
	//------------------------------------------------------------------

	/* Map dimensions in tiles. */
	FIntPoint MapTileDimensions;

	/* Map center ignoring Z axis */
	FVector2D MapCenter;

	/* 1.f / MapDimensions. Used to avoid division */
	FVector2D MapDimensionsInverse;

	//------------------------------------------------------------------
	//	Rendering Data
	//------------------------------------------------------------------

	UTexture2D * FogTexture;
	
	FUpdateTextureRegion2D * TextureRegions;
	
	//UPROPERTY() // Hopefully this isn't required
	UMaterialInstanceDynamic * FogOfWarMaterialInstance;
};


//--------------------------------------------------------------------------------------------
//============================================================================================
//	------- Thread Classes -------
//============================================================================================
//--------------------------------------------------------------------------------------------

/* A fog of war thread that the server player will spawn */
class RTS_VER2_API ServerFogOfWarThread : public FRunnable
{
	/* Default constructor */
	ServerFogOfWarThread();	

	/* Ideas: Iterate backwards along the arrays and check if any of them have not completed. 
	But check the local players team first: check if they have done their buildings, infantry 
	and temp reveal effects. If they have then check if they have done their visibility info. 
	If they have NOW we can iterate backwards and check if other team's stuff needs doing. 
	Although we only need to do this if we are the server */
	void OnTaskComplete();
	// Pseudo code:
	// {
	//		if (Infantry[LocalPlayersTeamIndex].)
	// }
};

 
/* A fog of war thread that remote clients will spawn */
class RTS_VER2_API ClientFogOfWarThread : public FRunnable
{
public:
	
	ClientFogOfWarThread();
	~ClientFogOfWarThread(); // Virtual?

	// Begin FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	// End FRunnable interface

	void ResetTileInfo(uint8 TeamIndex);

protected:

	void CalculateTeamVisibilityFromBuildings(uint8 TeamIndex, TArray<EFogStatus> & Tiles);
	void CalculateTeamVisibilityFromInfantry(uint8 TeamIndex, TArray<EFogStatus> & Tiles);
	void TickAndCalculateTeamVisibilityFromTemporaryRevealEffects(uint8 TeamIndex, TArray<EFogStatus> & Tiles);

	void StoreTileVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus> & Tiles);
	void QueueRenderFogCommand(uint8 TeamIndex, const TArray<EFogStatus> & Tiles);
	/* @param TeamWithVisionIndex - team the param Tiles belongs to
	@param OtherTeamIndex - team index of hostile team
	@param Tiles - the array we are checking visibility against. TeamIndex can belong to one 
	team while Tiles can belong to another */
	void CalculateAndStoreSelectableVisibilityInfo(uint8 TeamWithVisionIndex, uint8 OtherTeamIndex, const TArray<EFogStatus> & Tiles);
	void CalculateAndStoreProjectileVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus> & Tiles);
	void CalculateAndStoreParticleSystemVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus> & Tiles);
	void CalculateAndStoreInventoryItemVisibilityInfo(uint8 TeamIndex, const TArray<EFogStatus> & Tiles);
};

#endif // MULTITHREADED_FOG_OF_WAR

// Haven't done anything with multithreading anywhere else in code. It only exists in this file

// Added ISelectable* param to Statics::IsOutsideFog. It is called from AInfantryController::Tick 
// and some minimap code. Inf controller should Consider caching the ISelectable* instead of 
// doing the cast each tick (if there is significant cast overhead). As for minimap... well it 
// doesn't really work anyway but may want to make a note of it at least above the calls
