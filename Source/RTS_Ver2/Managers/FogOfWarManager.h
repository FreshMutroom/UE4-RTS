// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "Statics/Structs/Structs_6.h"
#include "FogOfWarManager.generated.h"

class ARTSPlayerState;
class ARTSGameState;
class APostProcessVolume;
struct FUpdateTextureRegion2D;
class ARTSLevelVolume;
class ABuilding;
class AProjectileBase;


/* Workaround for non-multidimension TArrays */
USTRUCT()
struct FTileArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray < EFogStatus > Tiles;

	FORCEINLINE const TArray < EFogStatus > & GetTiles() const { return Tiles; }
	FORCEINLINE TArray < EFogStatus > & GetTiles() { return Tiles; }
	FORCEINLINE void SetTileVisibility(EFogStatus NewVisibility, int32 Index) { Tiles[Index] = NewVisibility; }
};


/* Array of FVector2D */
USTRUCT()
struct FIntegerArray
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	TArray < int32 > Array;

public:

	void Reserve(int32 Amount) { Array.Reserve(Amount); }
	void Emplace(int32 Elem) { Array.Emplace(Elem); }
	const TArray < int32 > & GetArray() const { return Array; }

	FIntegerArray() { }
	FIntegerArray(int32 Amount)
	{
		Reserve(Amount);
	}
};


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Fog Manager Class -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

/**
 *------------------------------------------------------------------------------------------
 *	This class if for single threaded fog of war that runs on the game thread only. 
 *	Look at MultithreadedFogOfWar.h for an implementation that runs on its own thread.
 *------------------------------------------------------------------------------------------
 *
 *	On clients:
 *	Responsible for deciding what tiles your team can see, hiding/revealing things on those
 *	tiles and rendering the fog of war.
 *
 *	On server:
 *	Responsible for deciding which tiles every team can see and storing this information for
 *	2 reasons:
 *	- Knowing what can see what for AI behavior purposes
 *	- Stopping replication updates on a per connection basis for some things inside fog
 *
 *	Server also hides/reveals the things the local player can/can't see and renders its own fog
 *
 *	Currently relies on fog of war volume having equal width and length
 */
UCLASS()
class RTS_VER2_API AFogOfWarManager : public AInfo
{
	GENERATED_BODY()

public:

	AFogOfWarManager();

	/** 
	 *	Setup for match
	 *	
	 *	@param FogVolume - fog volume for current map
	 *	@param NumTeams - number of teams in match
	 *	@param InLocalPlayersTeam - team of the player that spawned this 
	 */
	void Initialize(ARTSLevelVolume * FogVolume, uint8 NumTeams, ETeam InLocalPlayersTeam, 
		UMaterialInterface * InFogOfWarMaterial);

protected:

	void SetupReferences();

	/* Setup information about map
	@param FogVolume - fog volume for current map */
	void SetupMapInfo(ARTSLevelVolume * FogVolume);

	/* Setup tile info */
	void SetupTileInfo(uint8 InNumTeams);

	void SetupRenderingReferences(UMaterialInterface * FogOfWarMaterial);

	void SetupTeamTempRevealEffects();

	virtual void Tick(float DeltaTime) override;

	virtual void BeginDestroy() override;

	/* Number of teams in match */
	uint8 NumTeams;

	/* The team of the local player */
	ETeam LocalPlayersTeam;

	/* Reference to player state this is for */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	FIntPoint GetGridCoords(const AActor * Actor) const;
	FIntPoint GetGridCoords(const FVector & WorldCoords) const;
	FIntPoint GetGridCoords(const FVector2D & World2DCoords) const;
	EFogStatus GetFogStatusForLocation(const AActor * Actor, const TArray <EFogStatus> & TilesArray) const;
	EFogStatus GetFogStatusForLocation(const FVector & WorldCoords, const TArray <EFogStatus> & TilesArray) const;
	EFogStatus GetFogStatusForLocation(const FVector2D & World2DCoords, const TArray <EFogStatus> & TilesArray) const;

	FORCEINLINE int32 GetTileIndex(int32 X, int32 Y) const;

	void ResetTeamVisibility(ETeam Team);

	void ComputeTeamVisibility(ETeam Team, float DeltaTime);

	/* Check what selectables a team can see */
	void Client_HideAndRevealSelectables(ETeam Team);

	/* Server version that does what client version does but also stores all
	visibility info in GS for AI purposes (and eventually to pause replication
	on a pre connection basis) */
	void Server_HideAndRevealSelectables(ETeam Team);

	// For resource spots
	void Server_StoreResourceSpotVisInfo(ETeam Team);

	/* Check what temporary actors/components spawned into the world such as
	projectiles or particle effects a team can see */
	void HideAndRevealTemporaries(ETeam Team);

	// For inventory items in the world
	void Server_HideAndRevealInventoryItems(ETeam Team);
	void Server_StoreNonLocalTeamsInventoryItemVisInfo(ETeam Team);
	void Client_HideAndRevealInventoryItems(ETeam Team);

	void MuteAndUnmuteAudio(ETeam Team);

	/* Utility function which calculates which tiles should be visible for a location and 
	stores them in the team's visibility array 
	
	@param Tiles - team's visibility array */
	void RevealFogAroundLocation(FVector2D Location, float FogRevealRadius, float StealthRevealRadius, 
		TArray <EFogStatus> & Tiles);

	void StoreTeamVisibility(ETeam Team);

	void RenderFogOfWar(ETeam Team);

	void FillTextureBuffer(ETeam Team);

	/* Helper function. Actually gives command to render fog */
	void UpdateTextureRegions(UTexture2D * Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D * Regions, uint32 SrcPitch, uint32 SrcBpp, uint8 * SrcData, bool bFreeData);

	/* Temporary reveal effects for teams. Key = Statics::TeamToArrayIndex 
	e.g. command center scans, patch revealed by using nuke at it */
	TArray <FTemporaryFogRevealEffectsArray> TeamTempRevealEffects;

	/* Each teams visibility of each tile. On clients only their own teams array will be updated.
	On server every teams array will be updated */
	UPROPERTY()
	TArray < FTileArray > TeamTiles;

	/* Maps building to all the indices in Tiles that their visibility should be checked against */
	UPROPERTY()
	TMap < ABuilding *, FIntegerArray > BuildingTileIndices;

	/* Map center ignoring Z axis */
	FVector2D MapCenter;

	/* Width and height of map in Unreal Units. */
	FVector2D MapDimensions;

	/* Map dimensions in tiles. */
	FIntPoint MapTileDimensions;

	/* Post process fog of war material */
	UPROPERTY()
	UMaterialInstanceDynamic * FogOfWarMaterialInstance;

	/* Renders fog of war */
	UPROPERTY()
	APostProcessVolume * PostProcessVolume;

	/* Fog of war buffer for rendering. Array. TODO: try and just use
	Tiles array instead */
	uint8 * TextureBuffer;

	// TODO: these are not UPROPERTY but seems to work ok

	/* Fog of war texture */
	UTexture2D * FogTexture;

	/* For rendering fog of war */
	FUpdateTextureRegion2D * TextureRegions;


	/* To speed up calculations */

	/* 1.f / MapDimensions. Used to avoid division */
	FVector2D MapDimensionsInverse;

	/*
	@param TileIndex - index in Tiles
	@param Team - team to check for
	@return - non-zero value if the tile is visible i.e. it is either Revealed
	or StealthRevealed */
	FORCEINLINE uint8 IsTileVisible(int32 TileIndex, ETeam Team) const;

	/**
	 *	Return the visibility of a tile for a team. If TileIndex is not a valid index i.e. the location 
	 *	is outside the fog grid then this will return not visible 
	 */
	FORCEINLINE uint8 IsTileVisibleNotChecked(int32 TileIndex, ETeam Team) const;

	FORCEINLINE EFogStatus BitwiseOrOperator(const EFogStatus & Enum1, const EFogStatus & Enum2) const;
	FORCEINLINE EFogStatus BitwiseOrOperator(const EFogStatus & Enum1, const uint8 & Enum2AsInt) const;

public:

	/* Return whether a world location is revealed by fog for a particular team. Assumes 
	the location is one inside the RTS level bounds */
	bool IsLocationVisible(const FVector & Location, ETeam Team) const;

	/* Return whether a world location is revealed by fog of war for a particular team. 
	It is OK to pass in values that are outside the RTS level bounds. Behavior for that 
	case is this will return false */
	bool IsLocationVisibleNotChecked(const FVector & Location, ETeam Team) const;

	/* Version that uses the local players team */
	bool IsLocationLocallyVisible(const FVector & Location) const;

	/* Version that uses the local players team. Can handle tiles outside the fog tile grid */
	bool IsLocationLocallyVisibleNotChecked(const FVector & Location) const;

	EFogStatus GetLocationVisibilityStatus(const FVector & Location, ETeam Team) const;

	/* Get the fog status of a location for the local player */
	EFogStatus GetLocationVisibilityStatusLocally(const FVector & Location) const;

	EFogStatus GetLocationVisibilityStatusNotChecked(const FVector & Location, ETeam Team) const;

	/* Get the fog status of a location for the local player */
	EFogStatus GetLocationVisibilityStatusLocallyNotChecked(const FVector & Location) const;

	/* Return whether a projectile should be visible or not
	@param Projectile - the projectile to check visibility for
	@param Team - the team of the player who is checking for visibility */
	bool IsProjectileVisible(const AProjectileBase * Projectile, ETeam Team) const;

	// Called by player state when a building is placed
	void OnBuildingPlaced(ABuilding * InBuilding, const FVector & BuildingLocation,
		const FRotator & BuildingRotation);

	// Called by player state when a building is destroyed
	void OnBuildingDestroyed(ABuilding * InBuilding);

	void CreateTeamTemporaryRevealEffect(const FTemporaryFogRevealEffectInfo & RevealEffect,
		FVector2D Location, ETeam Team);
};
