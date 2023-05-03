// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Statics/CommonEnums.h"
#include "StartingGrid.generated.h"

class UStaticMeshComponent;
class UStartingGridComponent;
class UArrowComponent;
class ARTSPlayerState;
class ABuilding;
class AInfantry;
struct FBuildingInfo;
class ARTSGameState;


/**
 *	An actor that only gets spawned at the very start of the match. It defines what buildings/units
 *	a faction starts with and how they are layed out.
 *
 *	To add buildings/units to what is spawned at the start of match for this faction add
 *	StartingGridComponents via editor. Then change their child actor class to whatever you want
 *	spawned. Move the StartingGridComponent around the grid to adjust the its spawn location
 *	(Z axis will be ignored and objects should be spawned with correct Z axis value, provided
 *	the Z values aren't insanely large). (0, 0, 0) is where the players camera will start at match
 *	start.
 *
 *	Assumptions about the terrain below are made such as that it is flat. Also right now no faction
 *	checking is done for what is spawned meaning you can have one faction start with buildings/units
 *	from another faction. This may cause crashes during gameplay so should not be done.
 */
UCLASS(Abstract)
class RTS_VER2_API AStartingGrid : public AActor
{
	GENERATED_BODY()

public:

	AStartingGrid();

	virtual void PostLoad() override;

protected:

	// Whether already placed in map
	bool bStartedInMap;

	virtual void BeginPlay() override;

	/* Dummy root component for mesh positioning */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USceneComponent * DummyRoot;

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	UArrowComponent * Arrow;

	/* The floor to make visualizing easier */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UStaticMeshComponent * Floor;

#endif

	/* Reference to player state that wants this stuff spawned  */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Maps each custom child actor component to what to spawn there */
	UPROPERTY()
	TArray < UStartingGridComponent * > GridComponents;

	/* Array of all the building types of buildings this grid spawned */
	UPROPERTY()
	TArray < EBuildingType > SpawnedBuildingTypes;

	/* Array of all the unit types of units this grid spawned */
	UPROPERTY()
	TArray < EUnitType > SpawnedUnitTypes;

	/* Figure out the spawn transform for a building/unit */
	static FVector GetSpawnLocation(UWorld * World, const FTransform & DesiredTransform);
	static FRotator GetSpawnRotation(UWorld * World, const FTransform & DesiredTransform);

#if WITH_EDITORONLY_DATA

	/* Whether to use human player index or CPU player index */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development")
	bool PIE_bUseCPUPlayerOwnerIndex;

	/* PIE player to use for each selectable that is a part of this grid. Overrides whatever is
	set on the actor itself.
	0 = server player , 1 = 1st client, 2 = 2nd client, and so on */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = bCanEditHumanOwnerIndex, ClampMin = 0))
	int32 PIE_HumanPlayerOwnerIndex;

	/* Same as above but for CPU players */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = PIE_bUseCPUPlayerOwnerIndex, ClampMin = 0))
	int32 PIE_CPUPlayerOwnerIndex;

#endif

public:

#if WITH_EDITOR

	bool PIE_IsForCPUPlayer() const;

	/* Get index of owner when testing in PIE */
	int32 PIE_GetCPUOwnerIndex() const;
	int32 PIE_GetHumanOwnerIndex() const;

	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;

#endif

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	bool bCanEditHumanOwnerIndex;

#endif

	/* Given a transform, figure out where the best place to spawn building is and spawn it there
	@param DesiredTransform - world coords of where to spawn building. Final spawn location
	will be chosen by GetSpawnLocation
	@param - BuildingBP - blueprint of building to spawn
	@return - reference to spawned building */
	static ABuilding * SpawnStartingBuilding(ARTSGameState * GameState, UWorld * World, const FTransform & DesiredTransform,
		const FBuildingInfo * BuildingInfo, ARTSPlayerState * BuildingOwner);

	/* Given a transform, figure out best place to spawn a unit and spawn it there
	@param DesiredTransform - world coords of where to spawn unit. Final spawn location
	will be chosen by GetSpawnLocation
	@param - UnitBP - blueprint of unit to spawn
	@return - reference to spawned unit */
	static AInfantry * SpawnStartingInfantry(ARTSGameState * GameState, UWorld * World, const FTransform & DesiredLocation,
		TSubclassOf <AActor> UnitBP, ARTSPlayerState * UnitOwner);

	/* Get the types of selectables this grid spawned. One for each actor so can have 
	duplicates of the same type. Will likely need to change them to return value not ref 
	because the grid gets destroyed */
	const TArray < EBuildingType > & GetSpawnedBuildingTypes() const;
	const TArray < EUnitType > & GetSpawnedUnitTypes() const;

	void RegisterGridComponent(UStartingGridComponent * GridComponent);
};
