// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/Structs_1.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "ObjectPoolingManager.generated.h"

class ARTSGameState;
struct FBasicDamageInfo;
class AInventoryItem;
class AFogOfWarManager;
class AInventoryItem_SM;
class AInventoryItem_SK;


#if WITH_EDITOR
/* Contains data about the time SuggestProjectileVelocity failed to find a solution */
struct FSuggestProjectileVelocityEntry
{
public:

	FSuggestProjectileVelocityEntry(const FVector & StartLoc, const FVector & TargetLoc)
		: StartLocation(StartLoc)
		, TargetLocation(TargetLoc)
		, Distance((StartLoc - TargetLoc).Size())
	{
	}

	/* Get the distance between start location and target location */
	float GetDistance() const { return Distance; }

protected:

	FVector StartLocation;
	FVector TargetLocation;
	/* Distance between StartLocation and EndLocation */
	float Distance;

	friend bool operator<(const FSuggestProjectileVelocityEntry & Elem_1, const FSuggestProjectileVelocityEntry & Elem_2)
	{
		return Elem_1.Distance < Elem_2.Distance;
	}
};
#endif


struct FSuggestProjectileVelocityEntryContainer
{
	FSuggestProjectileVelocityEntryContainer()
	{ 
		Array.Reserve(16);
	}

	TArray<FSuggestProjectileVelocityEntry> Array;

	static const int32 MAX_CONTAINER_ENTRIES = 50;
};


/* Workaround for non 2D TArrays */
USTRUCT()
struct FProjectileArray
{
	GENERATED_BODY()

private:

	/* Holds created projectiles. Will be used like a stack */
	UPROPERTY()
	TArray < AProjectileBase * > Array;

public:

	FProjectileArray(int32 ReserveSize);
	FProjectileArray();

	void AddToStack(AProjectileBase * NewProjectile);
	
	bool IsPoolEmpty() const { return Array.Num() == 0; }

	// Remove projectile from container
	AProjectileBase * Pop();

	// Return a pointer to a projectile in pool. Does not remove it or anything
	AProjectileBase * Last() const;
};


/**
 *	Handles requests for adding and removing actors from an object pool. 
 *
 *	Some things that are pooled:
 *	- projectiles 
 *	- inventory items
 *	- ghost buildings (although I don't think they're handled by this class, PC does it instead)
 *	
 *	TODO I do not think there is any need for this to be an AActor; a UObject is good enough
 */
UCLASS()
class RTS_VER2_API AObjectPoolingManager final : public AInfo
{
	GENERATED_BODY()

private:

	AObjectPoolingManager();

	virtual void BeginPlay() override;

	/* Reference to gamestate for sending RPCs */
	UPROPERTY()
	ARTSGameState * GS;

	/* TODO: Populate this on setting up faction info. And then delete it once
	the BPs have been moved to Pools */
	/* Holds all BP for each projectile used by selectables */
	UPROPERTY()
	TSet <TSubclassOf <AProjectileBase>> Projectile_BPs;

	/* Maps blueprint to struct that holds array of already spawned objects for that
	blueprint*/
	UPROPERTY()
	TMap <TSubclassOf <AProjectileBase>, FProjectileArray> Pools;

#if WITH_EDITORONLY_DATA
	uint32 CurrentFrameNumber;

	/* Maps projectile type to how many of it have been spawned this frame */
	TMap<TSubclassOf<AProjectileBase>, int32> NumProjectilesSpawnedThisFrame;

	/** 
	 *	Maps projectile type to how many of it have been spawned this frame. This will contain 
	 *	what is considered the worst frame for performance in terms of projectiles spawned 
	 */
	TMap<TSubclassOf<AProjectileBase>, int32> WorstProjectileFrame;

	// Frame count of the worst frame
	uint32 WorstProjectileFrameNumber;

	/* Every time SuggestProjectileVelocity fails to find a solution an entry is added here */
	TMap<TSubclassOf<AProjectileBase>, FSuggestProjectileVelocityEntryContainer> SuggestProjectileVelocityFails;
#endif

	/* Pool for inventory item actors that use static meshes. 
	If pool is empty and we request an object from it then 
	we will spawn a new actor to fulfill the request as opposed to crashing */
	UPROPERTY() // Could try getting away without this UPROPERTY 
	TArray <AInventoryItem_SM *> InventoryItems_SM;
	/* This array is for the ones with skeletal meshes */
	UPROPERTY() // Could try getting away without this UPROPERTY 
	TArray <AInventoryItem_SK *> InventoryItems_SK;

	/* Get how many projectiles should be in the pool for a certain BP */
	int32 GetNumForPool(TSubclassOf <AProjectileBase> ProjectileBP) const;

	/* Return how many items we should put in the inventory item pool at the start of the match. 
	Make sure to change this to whatever you want. 
	A thing to keep in mind: items that start on the map also enter the object pool when they 
	disappear. */
	int32 GetInventoryItemInitialPoolSize_SM(URTSGameInstance * GameInst) const;
	int32 GetInventoryItemInitialPoolSize_SK(URTSGameInstance * GameInst) const;

	/* Verify an inventory item actor is fit for entering the object pool i.e. it's tick is 
	turned off, it's hidden, etc */
	static bool IsFitForEnteringPool(AInventoryItem_SM * InventoryItemActor);
	static bool IsFitForEnteringPool(AInventoryItem_SK * InventoryItemActor);

	/* To make sure we aren't calling SetupPools more than once accidentally */
	bool bHasCreatedPools;

public:

	//-----------------------------------------------------------------------------
	//	Mostly Projectiles ( except CreatePools() )
	//-----------------------------------------------------------------------------

	/** 
	 *	Takes an object from pool, and then fires it at target. Call from server only
	 *	
	 *	@param Firer - the selectable that fired this projectile
	 *	@param AttackInfo - information such as projectile damage and blueprint
	 *	@param AttackRange - attack range of firer. 0 means infinite
	 *	@param Team - team firer is on
	 *	@param MuzzleLoc - the muzzle location of the person firing the projectile
	 *	@param Target - target selectable 
	 *	@param Roll - roll rotation projectile can use 
	 */
	void Server_FireProjectileAtTarget(AActor * Firer, const TSubclassOf<AProjectileBase> & ProjectileBP, 
		const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & MuzzleLoc, 
		AActor * Target, float Roll);

	/** 
	 *	Fires a projectile client-side at target
	 *	
	 *	@param ProjectileBP - the blueprint of the projectile
	 *	@param MuzzleLoc - the muzzle location of the person firing the projectile
	 *	@param Target - target selectable 
	 */
	void Client_FireProjectileAtTarget(const TSubclassOf<AProjectileBase> & ProjectileBP, const FBasicDamageInfo & DamageInfo, 
		float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * Target, float Roll);

	/** 
	 *	Functions for firing a projectile at a location, not a target.
	 *	
	 *	@param InInstigator - actor firing projectile for damage reaction purposes
	 *	@param DamageInfo - attack attributes of actor firing projectile
	 *	@param Team - team of InInstigator
	 *	@param StartLoc - where to "spawn" projectile
	 *	@param TargetLoc - where to fire projectile at
	 *	@param Roll - a roll rotation value the projectile might want to use
	 *	@param ListeningAbility - ability that will receive a notification when the projectile 
	 *	hits something.
	 *	@param ListeningAbilityUniqueID - the unique ID for ListeningAbility to identify which 
	 *	instance of that ability it is.
	 */
	void Server_FireProjectileAtLocation(AActor * InInstigator, const TSubclassOf<AProjectileBase> & ProjectileBP, 
		const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc,
		float Roll, AAbilityBase * ListeningAbility = nullptr, int32 ListeningAbilityUniqueID = 0);
	void Client_FireProjectileAtLocation(const TSubclassOf<AProjectileBase> & ProjectileBP,
		const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc,
		float Roll, AAbilityBase * ListeningAbility = nullptr, int32 ListeningAbilityUniqueID = 0);

	/* Fires a projectile in a direction. It is very likely I have not implemented FireInDirection 
	in any projectile classes yet so this will likely do nothing */
	void Server_FireProjectileInDirection(AActor * InInstigator, const TSubclassOf<AProjectileBase> & ProjectileBP,
		const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility = nullptr, int32 ListeningAbilityUniqueID = 0);
	void Client_FireProjectileInDirection(const TSubclassOf<AProjectileBase> & ProjectileBP,
		const FBasicDamageInfo & DamageInfo, float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility = nullptr, int32 ListeningAbilityUniqueID = 0);

	/* Add projectile back into pool
	@param Projectile - projectile to add back to pool
	@param ProjectileBP - BP to know what pool to add to */
	void AddToPool(AProjectileBase * Projectile, TSubclassOf <AProjectileBase> ProjectileBP);

	/* Add a blueprint to Projectile_BPs if it is not already in there
	@param ProjectileBP - the BP to add */
	void AddProjectileBP(TSubclassOf <AProjectileBase> ProjectileBP);

	/* Create the pools */
	void CreatePools();

	/* For HUD. Get a reference to a projectile in a pool to display its damage attributes. 
	This will crash if pool is empty */
	const AProjectileBase * GetProjectileChecked(const TSubclassOf < AProjectileBase > ProjectileBP) const;

	/* Get pointer to a pooled projectile. Will spawn it if pool is empty */
	AProjectileBase * GetProjectileReference(const TSubclassOf<AProjectileBase> & ProjectileBP);

protected:

	/* Takes a projectile from object pool or if the pools are empty then spawn one */
	AProjectileBase * GetProjectileFromPoolOrSpawnIfNeeded(const TSubclassOf<AProjectileBase> & ProjectileBP);

public:

#if WITH_EDITOR
	/* Not really anything to do with object pooling */
	void NotifyOfSuggestProjectileVelocityFailing(AProjectileBase * Projectile, const FVector & StartLoc,
		const FVector & TargetLoc);
	
	/**
	 *	Print to log the worst frame the pooling manager had in terms of projectile spawns. 
	 *	If there were a lot of spawns in a single frame the user might want to consider increasing 
	 *	the pool size.
	 */
	void LogWorstProjectileFrame();

	/* Logs info about the times UGameplayStatics::SuggestProjectileVelocity failed to find a solution */
	void LogSuggestProjectileVelocityFails();
#endif

	//------------------------------------------------------------------------
	//	Inventory Items
	//------------------------------------------------------------------------

	void PutInventoryItemInPool(AInventoryItem_SM * InventoryItem);
	void PutInventoryItemInPool(AInventoryItem_SK * InventoryItem);

	/** 
	 *	Puts an inventory item actor at a specific location and rotation. This will take one 
	 *	from the object pool if it's not empty, otherwise it will spawn an actor.
	 *	
	 *	@param ItemType - the type of item to put in world 
	 *	@param ItemQuantity - how many items are in the stack 
	 *	@param ItemsInfo - info struct of ItemType for convenience
	 *	@param Location - where to put the item. Note that the mesh settings in FInventoryItemInfo 
	 *	may move it away from this location
	 *	@param Rotation - world rotation. Note that the mesh settings in FInventoryItemInfo 
	 *	may rotate it some more
	 *	@return - reference to inventory item actor 
	 */
	AInventoryItem * PutItemInWorld(EInventoryItem ItemType, int16 ItemQuantity, int16 ItemNumCharges, 
		const FInventoryItemInfo & ItemsInfo, const FVector & Location, const FRotator & Rotation, 
		EItemEntersWorldReason ReasonForSpawning, AFogOfWarManager * FogManager);
};
