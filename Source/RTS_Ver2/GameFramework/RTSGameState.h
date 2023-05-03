// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "Statics/Structs_1.h"
#include "Statics/Structs_2.h"
#include "Statics/Structs_3.h"
#include "Statics/Structs_4.h"
#if MULTITHREADED_FOG_OF_WAR
#include "Managers/MultithreadedFogOfWar.h"
#endif
#include "RTSGameState.generated.h"

class ARTSPlayerState;
class AFogOfWarManager;
class ARTSPlayerController;
class AObjectPoolingManager;
class AProjectileBase;
class ACPUPlayerAIController;
class URTSGameInstance;
class FOnlineSessionSettings;
class ULobbySlot;
struct FMapInfo;
struct FMatchInfo;
class UParticleSystemComponent;
class AResourceSpot;
class ISelectable;
class AInfantry;
class AInventoryItem;
class AInventoryItem_SM;
class AInventoryItem_SK;
class UFogObeyingAudioComponent;


/* Array of resource spots */
USTRUCT()
struct RTS_VER2_API FResourcesArray
{
	GENERATED_BODY()

protected:

	TArray < AResourceSpot * > Array;

public:

	const TArray < AResourceSpot * > & GetArray() const { return Array; }

	void AddResourceSpot(AResourceSpot * InResourceSpot) { Array.Emplace(InResourceSpot); }
};


/** 
 *	TMap and TArray of UAudioComponents. TMap and TArray contain the same elements. 
 *	Did it like this for O(1) contains, add and remove and array speed iteration. 
 */
USTRUCT()
struct FAudioComponentContainer
{
	GENERATED_BODY()

protected:

	/* Key = index in Array component is */
	TMap <UFogObeyingAudioComponent *, int32> Map;
	
	UPROPERTY()
	TArray <UFogObeyingAudioComponent *> Array;

public:

	int32 Num() const;

	bool Contains(UFogObeyingAudioComponent * AudioComp) const;

	/* Assumes the element isn't already in the container */
	void Add(UFogObeyingAudioComponent * AudioComp);

	/* Remove element given we know it's in container */
	void RemoveChecked(UFogObeyingAudioComponent * AudioComp);

	const TArray<UFogObeyingAudioComponent *> & GetArray() const;
};


/**
 *	Same game state used all the time. Handles lobby state aswell
 */
UCLASS()
class RTS_VER2_API ARTSGameState : public AGameState
{
	GENERATED_BODY()

public:

	ARTSGameState();

	virtual void Tick(float DeltaTime) override;

private:

	/* TODO: set net update frequency in lobby to like 2, and change it to something higher
	when match starts */

	/* BeginPlay was getting called too late */
	//virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	/* [Server] Amount of time towards incrementing TickCounter */
	float AccumulatedTimeTowardsNextGameTick;

	/* A counter that tracks number of ticks. To reduce bandwidth it is kept as 1 byte but can 
	be increased if needed. It will overflow often and that is ok. This should never equal 
	255 */
	UPROPERTY(ReplicatedUsing = OnRep_TickCounter)
	uint8 TickCounter;

	/* [Server] Stores a pointer to selectables that have a selectable 
	resource that thas a regen rate != 0 */
	TArray < ISelectable * > Server_SelectableResourceUsersThatRegen;

	/* [Client] The last value of TickCounter */
	uint8 PreviousTickCounterValue;

	/* [Client] Stores pointers to selectables that regen a selectable resource over time */
	TArray < TWeakObjectPtr <AActor> > Client_SelectableResourceUsersThatRegen;

	UFUNCTION()
	void OnRep_TickCounter();

	void Server_RegenSelectableResources();
	void Client_RegenSelectableResources(uint8 NumGameTicksWorth);

	/* [Server] The time when match started. Used to create a custom match timer */
	float TimeWhenMatchStarted;

	/* The unique ID to assign to the next player. These will be assigned when loading a match
	from lobby or testing with PIE. APlayerState::PlayerId used to work but now always shows
	0 in PIE */
	uint8 NextUniquePlayerID;

	/* The team the local player is on */
	ETeam LocalPlayersTeam;

	/* Number of different teams in the match excluding neutral */
	uint32 NumTeams;

	/* Array of the player states of each team. */
	UPROPERTY()
	TArray < FPlayerStateArray > Teams;

	/* Holds the FNames to appear in actor tags for each team. Use UStatics::TeamToArrayIndex
	to convert an ETeam to its index in this array */
	UPROPERTY()
	TArray < FName > TeamTags;

	/* Array of player controllers server-side only */
	UPROPERTY()
	TArray < ARTSPlayerController * > PlayerControllers;

	/* Array of all player states excluding observers, usable by both server and client */
	UPROPERTY()
	TArray <ARTSPlayerState *> PlayerStates;

	/* Array of CPU player AI controllers */
	UPROPERTY()
	TArray < ACPUPlayerAIController * > CPUPlayers;

	/* Array of players that have not been defeated in the current match */
	UPROPERTY()
	TArray <ARTSPlayerState *> UndefeatedPlayers;

	/* Array of player states for players that are match observers */
	UPROPERTY()
	TArray <ARTSPlayerState *> Observers;

	/* Maps RTS player ID (uint8) to the player */
	UPROPERTY()
	TMap <uint8, ARTSPlayerState *> PlayerIDMap;

	/* Reference to fog of war manager once a match has started */
	UPROPERTY()
	AFogOfWarManager * FogManager;

	/* Modified on server only. Accessed by the fog of war manager
	on the server. Holds all info about the visibiliy of all
	selectables for all teams. Used to avoid sending updates over
	the wire when a selectables visibility has not changed */
	UPROPERTY()
	TArray < FVisibilityInfo > VisibilityInfo;

	/* Map elements that do not belong to a team but are selectable
	e.g. resource spots. Updated server-side only */
	UPROPERTY()
	TArray < AActor * > NeutralSelectables;

	/* Actors that have been spawned into the world such as projectiles that
	follow the rules of fog of war */
	UPROPERTY()
	TArray < AProjectileBase * > TemporaryFogProjectiles;

	/* Particle systems components that have been spawned into the world
	that follow the rules of fog of war */
	UPROPERTY()
	TArray < UParticleSystemComponent * > TemporaryFogParticles;

	/* Maps context action type to a actor that handles that context action */
	UPROPERTY()
	TMap < EContextButton, AAbilityBase * > ContextActionEffects;

	/* Maps commander ability type to its effect object */
	UPROPERTY()
	TMap <ECommanderAbility, UCommanderAbilityBase *> CommanderAbilityEffects;

	/* [Performance] Array of all inventory items that are in the world. Maintained on both 
	server and clients. This just basically exists because it's faster iterating a TArray than 
	a TMap. It was added here for the fog of war manager. We could remove this. */
	UPROPERTY()
	TArray < AInventoryItem * > InventoryItemsInWorldArray;

	/* Maps ID to item for it. Replica of InventoryItemsInWorldArray. Maintained on both 
	server and clients. */
	UPROPERTY()
	TMap < FInventoryItemID, AInventoryItem * > InventoryItemsInWorldTMap;

	/* Maps team to their trace channel as a uint64. Key = static_cast<uint8>(Team). */
	UPROPERTY()
	TArray<uint64> TeamTraceChannels;

	/* Maps team to array of every collision channel of their enemies.
	Key = static_cast<uint8>(Team). */
	UPROPERTY()
	TArray<FUint64Array> EnemyTraceChannels;

	/* Maps team to all the trace channels for their enemies 
	Key = Statics::TeamToArrayIndex(Team) */
	TArray<FCollisionObjectQueryParams> EnemyQueryParams;

	/* The trace channel for neutrals that want to be a part of queries. An example of 
	neutrals that might want to use this is neutral item shops */
	ECollisionChannel NeutralTraceChannel;

	/* Calculate the EObjectTypeQuery for each team and put them in TeamTraceChannels */
	void InitTeamTraceChannels();

	/* Query params to use when querying for all teams selectables */
	FCollisionObjectQueryParams AllTeamsQueryParams;

	/* Collsion struct that is set to overlap with all team channels and ignore everything else */
	FCollisionResponseContainer AllTeams_Overlap;

	/* Reference to object pooling manager */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	/* List of resource spots on map. Should be populated when the map loads */
	UPROPERTY()
	TMap < EResourceType, FResourcesArray > ResourceSpots;

	/* Array of all resource spots on map */
	UPROPERTY()
	TArray <AResourceSpot *> ResourceSpotsArray;

	void SetPoolingManager(AObjectPoolingManager * NewValue);

	/* Spawn the effects actors which will handle creating context action effects during a match
	(e.g. artillery strikes) */
	void SetupEffectsActors();

	/* Spawn the eeffect UObjects for each commander ability */
	void SetupCommanderAbilityEffects();

	/* Setup the FNames that corrispond to each team */
	void SetupTeamTags();

public:

	/* Register a player controller. Call when joining */
	void AddController(ARTSPlayerController * NewController);

	void SetupForMatch(AObjectPoolingManager * InPoolingManager, uint8 InNumTeams);

	/* Get array of all resource spots on map */
	const TArray <AResourceSpot *> & GetAllResourceSpots() const;

	/* Get all the resource spots on map for a resource type */
	const TArray < AResourceSpot * > & GetResourceSpots(EResourceType ResourceType) const;

	/* Add resource spot to ResourceSpots */
	void AddToResourceSpots(EResourceType ResourceType, AResourceSpot * Spot);

	/* Add a player state to a team */
	void AddToTeam(ARTSPlayerState * NewMember, ETeam Team);

	/* Register a CPU controller */
	void AddCPUController(ACPUPlayerAIController * CPUController);

	/* Before match starts, sets up which collision channel each team will use + their enemies.
	for things like capsule sweeps */
	void SetupCollisionChannels();

	/* Get the current value of the game tick counter. It overflows a lot so this cannot be used 
	to derive the total length of the match */
	uint8 GetGameTickCounter() const;

	AObjectPoolingManager * GetObjectPoolingManager() const;

	/* Called when a selectable is built. Updates fog of war visiblity map
	@param Selectable - the selectable created
	@param Team - the team the selectable belongs to 
	@param bIsServer - whether it is the server that is calling this */
	void OnBuildingPlaced(ABuilding * Building, ETeam Team, bool bIsServer);
	void OnBuildingConstructionCompleted(ABuilding * Building, ETeam Team, bool bIsServer);
	void OnInfantryBuilt(AInfantry * Infantry, ETeam Team, bool bIsServer);

	void OnBuildingZeroHealth(ABuilding * Building, bool bIsServer);
	void OnInfantryZeroHealth(AInfantry * Infantry, bool bIsServer);

	/* Functions to let us know a selectable can/cannot regenerate its selectable resource 
	over time */
	void Server_RegisterSelectableResourceRegener(ISelectable * Selectable);
	void Client_RegisterSelectableResourceRegener(AActor * Selectable);
	void Server_UnregisterSelectableResourceRegener(ISelectable * Selectable);
	void Client_UnregisterSelectableResourceRegener(AActor * Selectable);

	/* Called when a selectable is destroyed. Updates fog of war visibility map
	@param Selectable - the selectable destroyed
	@param Team - the team the selectable belongs to 
	@param bIsServer - whether it is the server that is calling this */
	void OnSelectableDestroyed(AActor * Selectable, ETeam Team, bool bIsServer);

	/* Should be called by a neutral selectable at some point during
	its initialization */
	void Server_RegisterNeutralSelectable(AActor * Selectable);

	/* Register a projectile to be affected by fog of war */
	void RegisterFogProjectile(AProjectileBase * Projectile);

	void UnregisterFogProjectile(AProjectileBase * Projectile);

	/* Register a particle system to be affected by fog of war */
	void RegisterFogParticles(UParticleSystemComponent * Particles);

	/** 
	 *	Multicast that a selectable has leveled/ranked up 
	 *	
	 *	@param NewRank - the new level/rank the selectable has achieved 
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnSelectableLevelUp(uint8 SelectableID, uint8 SelectablesOwnerID, uint8 NewRank);

	/** 
	 *	Multicast that an upgrade has completed. 
	 *	
	 *	Doing this through GS instead of calling on the upgrade manager because we may need 
	 *	ordering guarantees for these 
	 *
	 *	@param PlayerID - ID of the player who completed the upgrade
	 *	@param UpgradeType - the upgrade the player completed
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnUpgradeComplete(uint8 PlayerID, EUpgradeType UpgradeType);

	/* Get the effect actor for a certain ability */
	AAbilityBase * GetAbilityEffectActor(EContextButton AbilityType) const;

	/** 
	 *	Create the effect of an ability server-side, then notify all clients
	 *	
	 *	@param UsageCaseAuxilleryData - if this is an inventory slot usage then this should be 
	 *	the slot index
	 *	@param AbilityInfo - type of ability to activate
	 *	@param bOutIsRandom - whether the ability has random behavior
	 *	@pararm EffectInstigator - actor using the ability
	 *	@param InstigatorAsSelectable - EffectInstigator as a ISelectable
	 *	@param InstigatorsTeam - team of EffectInstigator
	 *	@param AbilityLocation - where to create ability
	 *	@param AbilityTarget - target of ability or null if not a targeting ability
	 *	@param TargetAsSelectable - Target as a ISelectable
	 *	@return - random number seed to send to client. Can be ignored if ability does not have random
	 *	behavior 
	 */
	void Server_CreateAbilityEffect(EAbilityUsageType UsageCase, uint8 UsageCaseAuxilleryData, 
		const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, ISelectable * InstigatorAsSelectable, 
		ETeam InstigatorsTeam, const FVector & AbilityLocation,
		AActor * AbilityTarget = nullptr, ISelectable * TargetAsSelectable = nullptr);

protected:

	//===========================================================================================
	//	A lot of RPCs for sending the event of the use of an ability to clients
	//===========================================================================================
	//	Note to self: could go a step further and distinguish between whether the ability needs 
	//	the result byte sent. If it only has one outcome then can avoid sending this
	//
	//	Also if replication graph allows then we can choose to make these unreliable for 
	//	match observers
	//-------------------------------------------------------------------------------------------

	// First some helper functions for these RPCs
	
	/* Return true if we are executing code on the server */
	bool IsServer() const;
	
	/* Checks if the instigator is valid or whatever: in a state that the ability can go through. 
	If false then this func will add the function data to a container to have it executed later. 
	See the first ability RPC for kind of how this should be done, but it probably needs changing 
	quite a bit */
	bool AbilityMulticasts_PassedInstigatorValidChecks(AActor * InInstigator);

	/* Checks that both the instigator and target are both valid or whatever and the ability 
	can go through. If false then this function will handle adding the RPC to whatever 
	container or whatever that processes them later */
	bool AbilityMulticasts_PassedInstigatorAndTargetValidChecks(AActor * InInstigator, AActor * InTarget);

	//===========================================================================================
	//	Context menu ability usage RPCs
	//===========================================================================================

	/* Except for the first one which I have already added to, the other 47 need the param
	TickCounterOnServerAtTimeOfAbility added to them */

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(EContextButton AbilityType, 
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator);

	//===========================================================================================
	//	RPCs for inventory slot usage. 
	//-------------------------------------------------------------------------------------------
	//	These are exactly the same except they just replace the EContextButton param with the 
	//	slot index instead. Using that we can derive what ability type it is.
	//===========================================================================================

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed,
		uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom(uint8 ServerInventorySlotIndex,
		FSelectableIdentifier AbilityInstigator);


	//===========================================================================================
	//	Another 96 multicast RPCs... these are all the 96 above except they also send the 
	//	server's RTS tick counter.
	//===========================================================================================

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed,
			uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget, 
			uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget, 
			uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location, 
			uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location, 
		uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits,
		FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
		int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(EContextButton AbilityType,
		FSelectableIdentifier AbilityInstigator, uint8 TickCounterOnServerAtTimeOfAbility);

	// The inventory ones

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed,
			uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FAbilityHitWithOutcome > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray < FSelectableIdentifier > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits,
			FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FAbilityHitWithOutcome > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const TArray < FSelectableIdentifier > & Hits, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location,
			int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility);
	UFUNCTION(NetMulticast, Reliable)
		void Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(uint8 ServerInventorySlotIndex,
			FSelectableIdentifier AbilityInstigator, uint8 TickCounterOnServerAtTimeOfAbility);

public:

	//===========================================================================================
	//	Commander Abilities
	//===========================================================================================

	TMap<ECommanderAbility, UCommanderAbilityBase *> & GetAllCommanderAbilityEffectObjects();

	UCommanderAbilityBase * GetCommanderAbilityEffectObject(ECommanderAbility AbilityType) const;

	/* [Server] Create an commander ability effect on the server and multicast it to all clients */
	void Server_CreateAbilityEffect(const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityInstigator,
		ETeam InstigatorsTeam, const FVector & AbilityLocation, AActor * AbilityTarget);

protected:

	/* This function is for development only. It calls the multicast RPC with every param possible 
	and is not very bandwidth efficient */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfCommanderAbilityUse_EverySingleParam(ECommanderAbility AbilityType,
		uint8 InstigatorsID, const FVector_NetQuantize & AbilityLocation, const FSelectableIdentifier & AbilityTargetInfo, 
		EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome> & Hits, int16 RandomNumberSeed16Bits, 
		uint8 ServerTickCountAtTimeOfAbility, float Direction);

public:

	//===========================================================================================
	//	Building Targeting Abilities
	//===========================================================================================
	//	Examples of building targeting abilities:
	//	- engineers capturing an enemy building or repairing a friendly one
	//	- spies revealing what is being produced in a building
	//-------------------------------------------------------------------------------------------

	/** 
	 *	[Server] Create a building targeting ability effect on the server and multicast it 
	 *	to all clients. 
	 *	
	 *	@param AbilityInfo - info struct for the ability that is being used 
	 *	@param AbilityTarget - building that is being targeted by the ability
	 */
	void Server_CreateAbilityEffect(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AInfantry * AbilityInstigator, ABuilding * AbilityTarget);

protected:

	/* This function is for development only. It calls the multicast RPC with every param possible
	and is not very bandwidth efficient */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyOfBuildingTargetingAbilityUse_EverySingleParam(EBuildingTargetingAbility AbilityType,
		const FSelectableIdentifier & AbilityInstigatorInfo, const FSelectableIdentifier & AbilityTargetInfo,
		EAbilityOutcome Outcome, int16 RandomNumberSeed16Bits);

public:

	//===========================================================================================
	//	Inventory Items
	//===========================================================================================

	/* Get array of all inventory items that are in the world. Does not include any item actors 
	that are in the object pool */
	const TArray <AInventoryItem *> & GetInventoryItemsInWorld() const;

	/* The ID to give to the next inventory item in actor form that requests one */
	FInventoryItemID CurrentInventoryItemUniqueID;

	/* Returns an ID to be used for an inventory item that has a presence in the world */
	FInventoryItemID GenerateInventoryItemUniqueID(AInventoryItem * InventoryItemActor);

	/* Put inventory item actor into relevant containers */
	void PutInventoryItemActorInArray(AInventoryItem * InventoryItemActor);

	AInventoryItem * GetInventoryItemFromID(FInventoryItemID ItemID) const;

	/** 
	 *	[Server] Try put item in inventory from a store purchase. 
	 *	
	 *	@return - "None" if successful 
	 */
	EGameWarning Server_TryPutItemInInventory(ISelectable * SelectableGettingItem, bool bIsInventoryOwnerSelected,
		bool bIsInventoryOwnerCurrentSelected, ARTSPlayerState * SelectablesOwner, 
		const FItemOnDisplayInShopSlot & ShopSlot, uint8 ShopSlotIndex, const FInventoryItemInfo & ItemsInfo, 
		URTSHUD * HUDWidget, ISelectable * ShopPurchasingFrom);

	/**
	 *	[Server] Put an item into a selectable's inventory. Broadcasts this to all clients. 
	 *	This function assumes that uniqueness checks and whatever have already happened. 
	 *	Handles warnings if unsuccessful.
	 *
	 *	@param SelectableGettingItem - the selectable getting the item 
	 *	@param bIsInventoryOwnerCurrentSelected - whether selectable getting item is selected by 
	 *	the local player. 
	 *	@param bIsInventoryOwnerCurrentSelected - whether selectable getting item is the current
	 *	@param SelectablesOwner - SelectableGettingItem's owner
	 *	selected for the local player
	 *	@param ItemType - the item to give to selectable
	 *	@param Quantity - how many of the item to give to the selectable. Must be at least 1.
	 *	@param ItemsInfo - item info for ItemType as a convenience 
	 *	@param ReasonForAquiring - how the item is being aquired
	 *	@param ItemOnGround - if item is being picked up off the ground then this should point to 
	 *	that item actor. Otherwise it can be null.
	 *	@return - None if item was successfully added to inventory, something else otherwise.
	 */
	EGameWarning Server_TryPutItemInInventory(ISelectable * SelectableGettingItem, bool bIsInventoryOwnerSelected, 
		bool bIsInventoryOwnerCurrentSelected, ARTSPlayerState * SelectablesOwner, 
		EInventoryItem ItemType, uint8 Quantity, const FInventoryItemInfo & ItemsInfo, 
		EItemAquireReason ReasonForAquiringItem, URTSHUD * HUDWidget, AInventoryItem * ItemOnGround);
	
protected:

	/* Tell clients to put an item on the ground into inventory */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PutItemInInventoryFromGround(FSelectableIdentifier SelectableGettingItem,
		FInventoryItemID ItemID);

	/** 
	 *	Check that the parameters for good enough to carry out its logic. If not then this 
	 *	function will should handle noting down the function as unexecuted so it can be executed 
	 *	later. 
	 *	
	 *	@return - true if the params are in a state that they can be used. 
	 */
	bool PutItemInInventoryFromShopPurchase_PassedParamChecks(AActor * ItemPurchaser, AActor * Shop);

	/** 
	 *	Tells clients about a successful item purchase from a shop. 
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PutItemInInventoryFromShopPurchase(FSelectableIdentifier SelectableGettingItem,
		FSelectableIdentifier ShopThatSoldItem, uint8 ShopSlotIndex);

	/** 
	 *	Tell clients to put an item into a selectable's inventory. Assumes it has already 
	 *	been checked that item can go in inventory.
	 *	
	 *	@param Quantity - how many of the item they are getting 
	 *	@param ReasonForAquiringItem - how they are aquiring the item 
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PutItemInInventory(FSelectableIdentifier SelectableGettingItem,
		EInventoryItem ItemType, uint8 Quantity, EItemAquireReason ReasonForAquiringItem);

public:

	/* Broadcast that an item was sold */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnInventoryItemSold(FSelectableIdentifier Seller, uint8 ServerInvSlotIndex);

	/* Put an inventory item actor into object pool */
	void PutInventoryItemInObjectPool(AInventoryItem_SM * InventoryItem);
	void PutInventoryItemInObjectPool(AInventoryItem_SK * InventoryItem);

protected:

	/* Return whether we should play a sound for a selectable aquiring an item */
	bool ShouldPlayInventoryItemAquireSound(ISelectable * SelectableThatGotItem, EInventoryItem ItemAquired,
		const FInventoryItemInfo & ItemsInfo) const;

	/* Return whether we should show some particles for a selectable aquiring an item */
	bool ShouldShowInventoryItemAquireParticles(ISelectable * SelectableThatGotItem, EInventoryItem ItemAquired,
		const FInventoryItemInfo & ItemsInfo) const;

	/* Play particles for a selectable aquiring an item */
	void PlayItemAquireParticles(ISelectable * SelectableThatGotItem, const FParticleInfo_Attach & Particles);

public:

	/** 
	 *	Puts an inventory item actor into the world at the specified location and rotation. 
	 *	
	 *	@param ItemStackQuantity - how many of the item to be in the stack
	 *	@param SpawnReason - the reason this item is appearing in the world
	 */
	void PutInventoryItemInWorld(EInventoryItem ItemType, int32 ItemStackQuantity, int16 ItemNumCharges,
		const FVector & Location, const FRotator & Rotation, EItemEntersWorldReason SpawnReason);


protected:

	/** 
	 *	Not actually inventory item related. Send an RPC to say a selectable has reached zero 
	 *	health. 
	 *	Important: there is no guarantee this will be sent for every selectable that reaches zero 
	 *	health. Currently it is only sent when a selectable drops items on death. 
	 *
	 *	The X, Y, Z params are split up because they need to be exact. FVector might compress 
	 *	each axis, I don't know, but want to be safe by sending the float seperately.
	 *
	 *	@param Selectable - selectable that reached zero health
	 *	@param Location_X - the X of the selectable's location
	 *	@param Location_Y - the Y of the selectable's location
	 *	@param Location_Z - the Z of the selectable's location
	 *	@param Yaw - the yaw of selectable's rotation
	 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnSelectableZeroHealth(FSelectableIdentifier Selectable, float Location_X,
		float Location_Y, float Location_Z, float Yaw);


	//==========================================================================================
	//	Unexecuted Multicast RPC Functions
	//==========================================================================================
	//	Functions that note down an RPC could not be executed because the target (which is a 
	//	selectable) has not called its ISelectable::Setup() yet. 
	//==========================================================================================

	void NoteDownUnexecutedRPC_Ability();

	void NoteDownUnexecutedRPC_CommanderAbility();

	void NoteDownUnexecutedRPC_BuildingTargetingAbility();

	void NoteDownUnexecutedRPC_PutItemInInventory(FSelectableIdentifier SelectableGettingItem,
		EInventoryItem ItemType, uint8 Quantity, EItemAquireReason ReasonForAquiringItem);

	void NoteDownUnexecutedRPC_PutItemInInventoryFromGround(FSelectableIdentifier SelectableGettingItem,
		FInventoryItemID ItemID);

	void NoteDownUnexecutedRPC_OnSelectableZeroHealth(FSelectableIdentifier Selectable,
		const FVector & Location, float Yaw);

	void NoteDownUnexecutedRPC_OnInventoryItemSold(FSelectableIdentifier Selectable,
		uint8 ServerInvSlotIndex);

public:

	//==========================================================================================
	//	------- Sound Component Containers ------- 
	//==========================================================================================
	//	These containers contain sounds the fog manager will iterate and mute/unmute if required
	//	They may be better suited on the fog manager class instead of here.

	// Sounds in the world with EFogSoundRules::Dynamic
	UPROPERTY()
	FAudioComponentContainer FogSoundsContainer_Dynamic;

	// Unheard AlwaysKnownOnceHeard sounds
	UPROPERTY()
	FAudioComponentContainer FogSoundsContainer_AlwaysKnownOnceHeard;

	// This will only contain the the sounds of hostiles
	UPROPERTY()
	FAudioComponentContainer FogSoundsContainer_DynamicExceptForInstigatorsTeam;


	//==========================================================================================
	//	Getters and setters 
	//==========================================================================================

public:

	void SetGI(URTSGameInstance * InGameInstance);

	/* [Server] Get how long match has been going for. Is stopped when game is paused. Negative 
	value means match has not started. Server-side only */
	float Server_GetMatchTime() const;

	void SetLocalPlayersTeam(ETeam InLocalPlayersTeam);
	ETeam GetLocalPlayersTeam() const;

	/* Get all players player controllers */
	const TArray <ARTSPlayerController *> & GetPlayers() const;

	/* Get all player states */
	const TArray < ARTSPlayerState * > & GetPlayerStates() const;
	TArray <ARTSPlayerState *> GetPlayerStatesByValue() const;

	/* Get all CPU AI controllers */
	const TArray <ACPUPlayerAIController *> & GetCPUControllers() const;

	/* Get a reference to the Teams array */
	const TArray < FPlayerStateArray > & GetTeams() const;

	/* Get a reference to the player state array for a team */
	const TArray <ARTSPlayerState *> & GetTeamPlayerStates(ETeam Team) const;

	/* Get all players that haven't been defeated in the current match yet */
	TArray < ARTSPlayerState * > & GetUndefeatedPlayers();

	/* Get the player state given a player's uint8 ID */
	ARTSPlayerState * GetPlayerFromID(uint8 InRTSPlayerID) const;

	/* Get reference to the visibility info of a team */
	FVisibilityInfo & GetTeamVisibilityInfo(ETeam Team);
	const FVisibilityInfo & GetTeamVisibilityInfo(ETeam Team) const;

	/* Get all team's visibility info containers */
	TArray<FVisibilityInfo> & GetAllTeamsVisibilityInfo();

	/* Get number of different teams in match */
	uint32 GetNumTeams() const;

	const TArray < AActor * > & GetNeutrals() const;

	/* For a given team get its FName tag to appear in actor tags */
	const FName & GetTeamTag(ETeam InTeam) const;

	TArray <AProjectileBase *> & GetTemporaryFogProjectiles();
	TArray <UParticleSystemComponent *> & GetTemporaryFogParticles();

	const TArray <uint64> & GetAllTeamCollisionChannels() const;

	/* Get a collision object query params that is setup to query against all teams channels */
	const FCollisionObjectQueryParams & GetAllTeamsQueryParams() const;

	/* This this struct that is set up to overlap all team's channels and ignore everything else */
	const FCollisionResponseContainer & GetAllTeamsCollisionResponseContainer_Overlap() const;

	/* Get the trace channel for a team as a ECollisionChannel */
	ECollisionChannel GetTeamCollisionChannel(ETeam Team) const;

	/* Get array of collision channels of enemy teams as uint8 */
	const TArray <uint64> & GetEnemyChannels(ETeam Team) const;

	/* For a certain team return query params that will give all enemy teams */
	const FCollisionObjectQueryParams & GetAllEnemiesQueryParams(ETeam Team) const;

	ECollisionChannel GetNeutralTeamCollisionChannel() const;

#if !MULTITHREADED_FOG_OF_WAR
	void SetFogManager(AFogOfWarManager * InFogManager);
	AFogOfWarManager * GetFogManager() const;
#endif


	//==========================================================================================
	//	Lobby 
	//==========================================================================================

	/* Here we have replicated arrays that define the state of the lobby. Whenever the state of
	the lobby changes, one of these arrays will change and be replicated to clients.
	I would have liked to have the lobby widgets define the state instead e.g. there are no extra
	variables - matches are created using the variables actually inside the text fields of the
	widgets, but having replicated arrays instead makes it easier to bring new players that join
	lobby up to speed */

protected:

	/* True if playing game, false if in say lobby. Used to know whether to consider certain
	lobby-specific variables for replication */
	UPROPERTY()
	bool bIsInMatch;

	/* Name of lobby */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyName)
	FString LobbyName;

	/* Player states in lobby. Using this player name, team and faction can be derived */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayers)
	TArray < ARTSPlayerState * > LobbyPlayers;

	/* Whether lobby players are human or CPU players. Also whether slot is open or closed */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayerTypes)
	TArray < ELobbySlotStatus > LobbyPlayerTypes;

	/* Difficulty of CPU player if CPU player */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyCPUDifficulties)
	TArray < ECPUDifficulty > LobbyCPUDifficulties;

	/* What team each person in lobby is */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyTeams)
	TArray < ETeam > LobbyTeams;

	/* What faction each person in lobby is */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyFactions)
	TArray < EFaction > LobbyFactions;

	/* What player start location each player in lobby has. -1 == unassigned */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPlayerStarts)
	TArray < int16 > LobbyPlayerStarts;

	/* Amount of resources to start match with. Value is the value currently set in lobby */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyStartingResources)
	EStartingResourceAmount LobbyStartingResources;

	/* Defeat condition for match that is currently set in lobby */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyDefeatCondition)
	EDefeatCondition LobbyDefeatCondition;

	/* Index of lobby map in some array */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyMap)
	uint8 LobbyMapIndex;

	/* If true then no human players should be allowed to join lobby and players already in
	lobby are not allowed to change anything like their team, faction etc but can still leave.
	This gives host a chance to review the lobby state without sneaky changes before match starts */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyAreSlotsLocked)
	uint8 bLobbyAreSlotsLocked : 1;

public:

	void SetupSingleplayerLobby();

	void SetupNetworkedLobby(const TSharedPtr <FOnlineSessionSettings> SessionSettings);

	/* Called during game modes InitNewPlayer function when a new player joins, excluding ourselves
	@param NewPlayerController - player controller of the person who has joined
	@param DefaultFaction - faction to set them as when they join 
	@return - the lobby slot they joined into, or -1 if no open lobby slot could be found */
	int32 OnClientJoinsLobby(APlayerController * NewPlayerController, EFaction DefaultFaction);

protected:

	/* Returns the first open lobby slot or -1 if no slot is open */
	virtual int32 GetNextOpenLobbySlot() const;

	/* Empty all lobby arrays. Leave lobby name and map how it is */
	void ClearLobby();

	/* Put player in lobby slot with a specified faction
	@param Player - player controller for new player
	@param StartingFaction - faction they passed to server when joining
	@return - the lobby slot they joined into, or -1 if no slot could be found */
	int32 PopulateLobbySlot(APlayerController * Player, EFaction StartingFaction);

public:

	/* Put a CPU player in a lobby slot */
	void PopulateLobbySlot(ECPUDifficulty CPUDifficulty, int32 SlotIndex);

	/* Remove a human player from the lobby
	@param SlotIndex - slot in lobby widget
	@param NewSlotStatus - what status to put the slot as, either open or closed */
	void RemoveFromLobby(int32 SlotIndex, ELobbySlotStatus NewSlotStatus);

protected:

	/* Call OnReps because not called automatically on server */
	void UpdateServerLobby();

	UFUNCTION()
	void OnRep_LobbyName();

	UFUNCTION()
	void OnRep_LobbyPlayers();

	UFUNCTION()
	void OnRep_LobbyPlayerTypes();

	UFUNCTION()
	void OnRep_LobbyCPUDifficulties();

	UFUNCTION()
	void OnRep_LobbyTeams();

	UFUNCTION()
	void OnRep_LobbyFactions();

	UFUNCTION()
	void OnRep_LobbyPlayerStarts();

	UFUNCTION()
	void OnRep_LobbyStartingResources();

	UFUNCTION()
	void OnRep_LobbyDefeatCondition();

	UFUNCTION()
	void OnRep_LobbyMap();

	UFUNCTION()
	void OnRep_LobbyAreSlotsLocked();

public:

	/* Among other possible cases this is used by game mode when a player leaves to determine
	what kind of behavior to take */
	bool IsInMatch() const;

	bool AreLobbySlotsLocked() const;

	// For Human players
	void ChangeTeamInLobby(ARTSPlayerController * Player, ETeam NewTeam);
	// For CPU players
	void ChangeTeamInLobby(uint8 SlotIndex, ETeam NewTeam);
	// For human players
	void ChangeFactionInLobby(ARTSPlayerController * Player, EFaction NewFaction);
	// For CPU players
	void ChangeFactionInLobby(uint8 SlotIndex, EFaction NewFaction);
	void ChangeStartingSpotInLobby(ARTSPlayerController * Player, int16 StartingSpotID);
	void ChangeCPUDifficultyInLobby(uint8 SlotIndex, ECPUDifficulty NewDifficulty);
	void ChangeStartingResourcesInLobby(EStartingResourceAmount NewAmount);
	void ChangeDefeatConditionInLobby(EDefeatCondition NewCondition);
	void ChangeMapInLobby(const FMapInfo & NewMap);
	void KickPlayerInLobby(uint8 SlotIndex, ARTSPlayerState * Player);
	void ChangeLockedSlotsStatusInLobby(bool bNewValue);

	/* Functions to set what our team/faction is when we change it on client but is not allowed
	on server, one reason for this is slots are locked server-sdie but haven't repped to client yet */
	void SetTeamFromServer(ARTSPlayerController * PlayerToCorrectFor, ETeam Team);
	void SetFactionFromServer(ARTSPlayerController * PlayerToCorrectFor, EFaction Faction);
	void SetStartingSpotFromServer(ARTSPlayerController * PlayerToCorrectFor, int16 StartingSpotID);

	/* Make a lobby slot open for players to join if there is enough room in lobby
	@return - true if successful */
	bool TryOpenLobbySlot();

	/* Make a lobby slot unable to be occupied */
	void CloseLobbySlot(uint8 SlotIndex);

	/* Broadcast chat message to clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendLobbyChatMessage(ARTSPlayerState * Sender, const FString & Message);

	/* Given the index of a player in lobby get their player start */
	int16 GetLobbyPlayerStart(uint8 LobbyIndexOfPlayer) const;

	/** 
	 *	For debugging only. Compares match info against values in replicated arrays to ensure
	 *	widgets had the right values displayed
	 *	
	 *	@param MatchInfo - match info struct to repped lobby arrays/variables to
	 *	@param NewTeamMap - map to put array of teams through before checking. This is because lobby
	 *	will change teams to lowest values possible, e.g. if players have teams 1 and 3 then they
	 *	will be changed to 1 and 2 respecively
	 *	@param bCheckPlayerStarts - whether to check if the player start locations are correct
	 *	@return - error string or FString() if no error 
	 */
	FString IsMatchInfoCorrect(const FMatchInfo & MatchInfo, const TMap < ETeam, ETeam > & NewTeamMap, 
		bool bCheckPlayerStarts = false) const;

	/* For debugging only. Verify match info state is the same as game state state */
	bool AreMatchInfoPlayerStartsCorrect(const FMatchInfo & MatchInfo) const;


	//==========================================================================================
	//	Inital Map Setup for Skip Main Menu for Testing
	//==========================================================================================

protected:

	/* Counter for the number of PCs that have completed Client_SetupForMatch when testing with PIE
	and skip main menu */
	int32 PIE_NumPCSetupCompleteAcks;

	/* Number of acks received for PS::Multicast_SetInitialValues */
	int32 PIE_NumPSSetInitialValuesAcks;

public:

	// GM will poll this during PIE + skip main menu setup
	int32 GetNumPCSetupAcksForPIE() const;

	// Get number of acks received from ARTSPlayerState::Multicast_SetInitialValues
	int32 GetNumPSSetupAcksForPIE() const;

	// Number of ARTSPlayerState::Client_FinalSetup have completed
	int32 GetNumFinalSetupAcks() const;


	//==========================================================================================
	//	Inital Match Setup
	//==========================================================================================

public:

	uint8 GenerateUniquePlayerID();

	/* Tell all players to load map for match */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_LoadMatchMap(const FName & MapName);

	/* [Server] Set expected number of players + CPU players to know number of acks needed
	to start match
	@param InNumPlayers - total number of players including CPU players
	@param InNumHumanPlayers - number of human players only */
	void SetNumPlayersForMatch(int32 InNumPlayers, int32 InNumHumanPlayers);

protected:

	/* The status of loading the match */
	UPROPERTY(ReplicatedUsing = OnRep_MatchLoadingStatus)
	ELoadingStatus MatchLoadingStatus;

	UFUNCTION()
	void OnRep_MatchLoadingStatus();

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Number of human players expected to be in the match */
	int32 ExpectedNumHumanPlayers;

	/* Expected number of players in match including CPU players */
	int32 ExpectedNumPlayers;

	/* Number of match player controllers that have completed their Client_SetupForMatch */
	int32 NumPCSetupAcks;

	/* Number of players that have finished streaming in the level */
	int32 NumLoadedLevelAcks;

	/* Number of player states that have had their inital values set up. This should be larger
	than the number of players because it is one ack for every player state */
	int32 NumInitialValueAcks;

	/* Number of acks from AMyPlayerState::Client_FinalSetup */
	int32 NumFinalSetupAcks;

	/* Stream in the match level
	@param MapPath - the name of the map as it appears in editor. The full path isn't necessary,
	just the name, but it must be in the /Game/Maps/ directory */
	void StreamInMatchLevel(const FName & MapPath);

	int32 NumLevelsStreamedOut;
	bool bHasStreamedInMatchLevel;

	UFUNCTION()
	void OnLevelStreamedOut();

	UFUNCTION()
	void OnMatchLevelStreamedIn();

	/* Destroy all selectables on the currently loaded map that are not things that should be 
	there at the start (so exclude things like resource spots) */
	void DestroyAllSelectablesOnMap();

	/* Check if all levels streamed out and match level streamed in */
	void CheckIfStreamingComplete();

	/* [Server] Check if all human players have connected, they have all acknowledged they have complete
	Client_PostLogin and they have all finished streaming in their map. If all true then move on
	to next part of match setup */
	void CheckIfLevelsLoadedAndPostLoginsComplete();

	// TODO remove before packaging if not needed
	/* True if check above was successful. Only here for debugging purposes */
	bool bHasAckedPostLoginsAndMaps;

public:

	ELoadingStatus GetMatchLoadingStatus() const;
	void SetMatchLoadingStatus(ELoadingStatus NewStatus);

	/* Called by a player controller when they have completed their Client_SetupForMatch */
	void Server_AckPCSetupComplete(ARTSPlayerController * PlayerController);

	/* Called when all levels have streamed out and match level has streamed in */
	void AckLevelStreamingComplete();

	/* Called by player state when it has received all replicated info about a player state */
	void Server_AckInitialValuesReceived();

	void Server_AckFinalSetupComplete();

	void StartMatch(const TMap < ARTSPlayerState *, FStartingSelectables > & StartingSelectables);

protected:

	/* Should only need to be called right before match begins */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnMatchStarted();

	void OnMatchStarted_Part2();

	/* Start the timer that says how long match has been going for */
	void StartMatchTimer();

	/* Start all CPU player behavior 
	@param StartingSelectables - maps player state to the selectables they started match with */
	void StartCPUPlayerBehavior(const TMap <ARTSPlayerState *, FStartingSelectables>& StartingSelectables);

public:

#if WITH_EDITOR
	/* Called when a play in editor test has has fully setup and is about to start 
	@param CPUStartingSelectables - maps each CPU player's player state to the selectables they 
	are starting the match with */
	void StartPIEMatch(const TMap < ARTSPlayerState *, FStartingSelectables > & CPUStartingSelectables);
#endif


	//==========================================================================================
	//	Players Leaving 
	//==========================================================================================

public:

	/* Called by game mode when a player logs out during a match */
	void HandleClientLoggingOutInMatch(ARTSPlayerController * Exiting);


	//==========================================================================================
	//	Match Ending
	//==========================================================================================

public:

	/* This is usually called when the host leaves the match either by choice or not. Tells
	all clients to disconnect gracefully */
	void DisconnectAllClients();

	/* Returns true if a winner has been found.
	@param DefeatedThisCheck - the player(s) that were previously not defeated but are now.
	@param OutWinningTeams - the team(s) that won. If Num() > 1 then it is a draw between some
	teams
	@return - true if match should end */
	bool IsMatchNowOver(const TArray < ARTSPlayerState * > & DefeatedThisCheck, TSet < ETeam > & OutWinningTeams);

	/* Send notification that a player has been defeated. Will not be sent if match ends at same
	time
	@param NewlyDefeatedPlayers - players defeated on most recent game mode defeated player check */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPlayersDefeated(const TArray < ARTSPlayerState * > & NewlyDefeatedPlayers);

	/* Called by the game mode when it finds a winning team for the match. If param .Num() is
	greater than 1 then the match is considered a draw and all the teams in the array should
	get a "draw" result while the rest will get a "lost" result. Otherwise if the param .Num() is
	1 then that team is the winner */
	void OnMatchWinnerFound(const TSet < ETeam > & LastStandingTeams);

protected:

	/* Send notification that match has ended */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnMatchWinnerFound(const TArray < ETeam > & LastStandingTeams);


	//==========================================================================================
	//	In-Match Messaging
	//==========================================================================================

public:

	void SendInMatchChatMessageToEveryone(ARTSPlayerState * Sender, const FString & Message);
	void SendInMatchChatMessageToTeam(ARTSPlayerState * Sender, const FString & Message);


	//==========================================================================================
	//	Utility Functions
	//==========================================================================================

protected:

	/** 
	 *	Call function that returns void after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ARTSGameState:: *Function)(), float Delay);
};
