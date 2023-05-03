// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if MULTITHREADED_FOG_OF_WAR

#include "CoreMinimal.h" 

#include "Statics/Structs/Structs_6.h"

class AInfantry;
class ABuilding;
class MultithreadedFogOfWarManager;
class ServerFogOfWarThread;
class ClientFogOfWarThread;

//--------------------------------------------------------------------------------------------
//============================================================================================
//	------- Enums and structs -------
//============================================================================================
//--------------------------------------------------------------------------------------------

enum class EMulthreadedFogTask : uint32
{
	ResetTiles										= 1 << 0,
	RevealTilesFromBuildings						= 1 << 1,
	RevealTilesFromInfantry							= 1 << 2,
	TickAndRevealTilesFromTemporaryRevealEffects	= 1 << 3,
	StoreTileVisibility								= 1 << 4,
	QueueUpRenderingFogOfWar						= 1 << 5,
	StoreHostileTeamSelectableVisibility			= 1 << 6,
	StoreProjectilesVisibility						= 1 << 7,
	StoreParticleSystemsVisibility					= 1 << 8,
	StoreInventoryItemVisibility					= 1 << 9,
	Finished										= 1 << 10
};


/* Wrapper for what I will use as a lock for critical sections */
struct FogLock
{
	FCriticalSection LockObject;

	void Lock() { LockObject.Lock(); }
	bool TryLock() { return LockObject.TryLock(); }
	void Unlock() { LockObject.Unlock(); }
};


struct CalculationProgress
{
	/* Lock to access the data in this struct */
	FogLock Lock;

	/* What the next thread to finish its task should work on next */
	EMulthreadedFogTask NextTask;

	uint8 NextTeamIndexForStoringSelectableVisibilityInfo;

	CalculationProgress()
		: NextTask(EMulthreadedFogTask::Finished) 
		, NextTeamIndexForStoringSelectableVisibilityInfo(0)
	{
	}

	/* @param TeamIndex - team index this task struct belongs to
	@param NumTeams - number of teams in the match */
	void AdvanceNextTask(uint8 TeamIndex, MultithreadedFogOfWarManager & FogManager);

	uint8 GetNextTeamIndexForStoringSelectableVisibilityInfo()
	{
		return NextTeamIndexForStoringSelectableVisibilityInfo++;
	}

	EMulthreadedFogTask GetNextTask() const { return NextTask; }
};


struct JobQueue
{
	// A 1 means that job is in the queue
	EMulthreadedFogTask BitField;

	// Will return "Finished" if no job in queue
	EMulthreadedFogTask GetJob()
	{

	}
};


struct TilesArray_NoLock
{
	TArray<EFogStatus> Array;
};


struct ServerThreadArray
{
	TArray<ServerFogOfWarThread*> Array;

	FogLock Lock;
};


struct ClientThreadArray
{
	TArray<ClientFogOfWarThread*> Array;

	FogLock Lock;
};


struct InfantryFogInfo
{
	/* The world location of the infantry in 2D coords (fog of war does not care about Z axis) */
	FVector2D WorldLocation;

	float SightRadius;
	float StealthRevealRadius;

	explicit InfantryFogInfo(AInfantry * Infantry);
};


struct BuildingFogInfo
{
	FVector2D WorldLocation;

	float SightRadius;
	float StealthRevealRadius;

	uint8 SelectableID;
	EFogStatus FogStatus;

	/* Indices into tiles array that this building is occupying */
	TArray<int32> FogLocationsIndices;

	explicit BuildingFogInfo(ABuilding *  Building);
};


struct InfantryFogInfoArray
{
	TArray<InfantryFogInfo> Array;

	/* Lock for accessing Array. Do not read/write to array without first holding this */
	FogLock ArrayLock;
};


struct InfantryFogInfoBasic
{
	FVector2D WorldLocation;

	/* Selectable ID of the infantry */
	uint8 SelectableID;

	/* Fog status of the tile the infantry is standing on */
	EFogStatus FogStatus;

	explicit InfantryFogInfoBasic(AInfantry * Infantry);
};


struct InfantryFogInfoFinal
{
	/* Fog status of the tile the infantry is standing on */
	EFogStatus TilesFogStatus;
};


struct InfantryArray
{
	TArray<AInfantry*> Array;
};


struct InfantryFogInfoBasicArray
{
	TArray<InfantryFogInfoBasic> Array;

	FogLock ArrayLock;
};


struct BuildingFogInfoArray
{
	TArray<BuildingFogInfo> Array;

	/* Lock for accessing Array. Do not read/write to array without first holding this */
	FogLock ArrayLock;
};


struct BuildingFogInfoArrayBasic
{
	TArray<BuildingFogInfo> Array;
};


/* This container will likely be used for checking if a building has been marked as recently
destroyed */
struct SelectableIDArray
{
	// Array of selectable IDs
	TArray<uint8> Array;
};


struct DestroyedBuildingArray
{
	/* Index = selectable ID */
	// TODO make this an array of bitfields instead, unless that's how TArray<bool> is implemented in the first place...
	bool Array[ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER];

	/* Return true if the building has been recently destroyed */
	bool WasRecentlyDestroyed(const BuildingFogInfo & Elem) const;

	void SetBitToRecentlyDestroyed(uint8 BuildingsSelectableID);

	void SetBitToNotRecentlyDestroyed(const BuildingFogInfo & Elem);

	/* For debugging. Verify all the flags in Array are set to false */
	bool AreAllFlagsReset() const;
};


struct TempRevealEffectArray
{
	TArray<FTemporaryFogRevealEffectInfo> Array;

	FogLock ArrayLock;
};


struct TempRevealEffectArray_NoLock
{
	TArray<FTemporaryFogRevealEffectInfo> Array;
};


struct TeamVisibilityInfoStruct
{
	/* Index = selectable's ID */
	EFogStatus Array[ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER];

	/* Lock for accessing Array. Do not read/write to array without first holding this */
	FogLock ArrayLock;
};


struct TilesArray
{
	TArray<EFogStatus> Array;

	FogLock Lock;
};

#endif // MULTITHREADED_FOG_OF_WAR
