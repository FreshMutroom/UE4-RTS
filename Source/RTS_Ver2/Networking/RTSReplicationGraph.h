// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ReplicationGraph.h"
#include "RTSReplicationGraph.generated.h"

class URTSReplicationGraphNode_AlwaysRelevant;
class URTSReplicationGraphNode_AlwaysRelevantForConnection;
class URTSReplicationGraphNode_TeamBuildings;
class URTSReplicationGraphNode_TeamInfantry;
enum class ETeam : uint8;
class ARTSGameState;
struct FVisibilityInfo;
class ABuilding;
class AInfantry;


UENUM()
enum class EClassRepNodeMapping : uint8
{
	NotRouted,
	RelevantAllConnections,
	RelevantOwnerOnly,

	// e.g. units, buildings, etc built by player. NOT neutral selectables
	PlayerOwnedSelectable_Building,
	PlayerOwnedSelectable_Infantry
};


USTRUCT()
struct FRTSConnectionAlwaysRelevantNodePair
{
	GENERATED_BODY()

	FRTSConnectionAlwaysRelevantNodePair() { }
	FRTSConnectionAlwaysRelevantNodePair(UNetConnection * InConnection, URTSReplicationGraphNode_AlwaysRelevantForConnection * InNode) : NetConnection(InConnection), Node(InNode) { }
	bool operator==(const UNetConnection* InConnection) const { return InConnection == NetConnection; }

	UPROPERTY()
	UNetConnection * NetConnection = nullptr;

	UPROPERTY()
	URTSReplicationGraphNode_AlwaysRelevantForConnection * Node = nullptr;
};


/**
 *	Have used UBasicReplicationGraph as a guide for how to implement this
 *
 *	To enable this via ini:
 *	[/Script/OnlineSubsystemUtils.IpNetDriver]
 *	ReplicationDriverClassName="/Script/ReplicationGraph.RTSReplicationGraph"
 *
 *	The function AActor::IsReplicationPausedForConnection can be removed from selectables
 *
 *	---------------------------------------------------------------------------------------------
 *
 *	TODO:
 *	- the infantry and building nodes: when they suddenly become revealed by fog what is actually
 *	happening is a new actor is being spawned. That's crazy! But if you add breakpoints in
 *	BeginPlay you can see it firing. This is unexpected. I did not know unreal creates a
 *	completely new actor when one transitions from being not relevant to relevant. What I actually
 *	want is just to stop the updates coming through. So try and make this happen.
 *
 *	---------------------------------------------------------------------------------------------
 *	Few things with buildings: 
 *	- in the time between TearOff and Destroy being called TornOff gets spammed. Have tried 
 *	UBasicReplicationGraph and it does not happen with that. Oddly if I create an exact replica 
 *	of UBasicReplicationGraph and use that it still happens. Possibly OnRep functions can 
 *	spam too
 *	- I get a GUID error sometimes when a building is destroyed. Get a massive like 5sec hitch 
 *	when it happens. Perhaps see if it happens with basic replication graph.
 *	
 *	Update: UBasicReplicationGraph does not actually load if set in config file. This explains 
 *	why my replica of basic rep graph was acting differently! So even with basic replication 
 *	graph I get TornOff spam. So the problem is possibly with the default implementation 
 *	of replication graph. I was under the impression that tear off is supported by it.
 *
 *	Summary: TornOff spam happens even with basic rep graph but the GUID error does not. 
 *	TornOff spam is probably a problem with engine's rep graph implementation while the 
 *	GUID error could be something to do with my RTS rep graph implementation.
 *
 *	---------------------------------------------------------------------------------------------
 */
UCLASS(Transient, config = Engine)
class RTS_VER2_API URTSReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:

	URTSReplicationGraph();

protected:

	/** Initialize the per-class data for replication */
	virtual void InitGlobalActorClassSettings() override;

	/** Init/configure Global Graph */
	virtual void InitGlobalGraphNodes() override;

	/** Init/configure graph for a specific connection. Note they do not all have to be unique: connections can share nodes (e.g, 2 nodes for 2 teams) */
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection * ConnectionManager) override;

public:

	/**
	*	This function is called by the game state once the number of teams for the match is known.
	*	Calling it ourselves during InitGlobalGraphNodes() is too early
	*/
	void NotifyOfNumTeams(uint8 NumTeams, ARTSGameState * GameState);

protected:

	// Route actor spawning to the right node. (Or your nodes can gather the actors themselves)
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalInfo) override;

	// Route actor despawning to the right node. (Or your nodes can gather the actors themselves)
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo) override;

	virtual int32 ServerReplicateActors(float DeltaSeconds) override;
	
	// 2 functions only implemented to use the RTSReplicationGraphFrame
	void ReplicateActorListsForConnection_Default(UNetReplicationGraphConnection * ConnectionManager, FGatheredReplicationActorLists& GatheredReplicationListsForConnection, FNetViewer& Viewer);
	void ReplicateActorListsForConnection_FastShared(UNetReplicationGraphConnection * ConnectionManager, FGatheredReplicationActorLists& GatheredReplicationListsForConnection, FNetViewer& Viewer);

	int64 ReplicateSingleActor(AActor* Actor, FConnectionReplicationActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalActorInfo, FPerConnectionActorInfoMap& ConnectionActorInfoMap, UNetConnection * NetConnection, const uint32 FrameNum);

	// Only implemented to use the RTSReplicationGraphFrame
	virtual bool ProcessRemoteFunction(AActor * Actor, UFunction * Function, void * Parameters, FOutParmRec * OutParms, FFrame * Stack, UObject * SubObject) override;

	// 4.22: removed const for Class param because ClassRepNodePolicies.Get no longer takes const param
	EClassRepNodeMapping GetMappingPolicy(UClass * Class);

	URTSReplicationGraphNode_AlwaysRelevantForConnection * GetAlwaysRelevantNodeForConnection(UNetConnection * Connection);

	URTSReplicationGraphNode_TeamBuildings * GetBuildingsNodeForTeam(ETeam Team);

	URTSReplicationGraphNode_TeamInfantry * GetInfantryNodeForTeam(ETeam Team);

	template <typename TSelectable>
	static bool HasSelectableSetUp(TSelectable * Selectable)
	{
		return Selectable->GetTeam() != ETeam::Uninitialized;
	}

public:

	const FVisibilityInfo & GetTeamVisibilityInfo(ETeam Team) const;

	void NotifyOfBuildingDestroyed(ABuilding * Building);

protected:

	//-------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------

	UPROPERTY()
	URTSReplicationGraphNode_AlwaysRelevant * AlwaysRelevantNode;

	/* Nodes for a team
	Key = Statics::TeamToArrayIndex(Team) */
	UPROPERTY()
	TArray<URTSReplicationGraphNode_TeamBuildings *> TeamBuildingNodes;

	UPROPERTY()
	TArray<URTSReplicationGraphNode_TeamInfantry *> TeamInfantryNodes;

	UPROPERTY()
	TArray<FRTSConnectionAlwaysRelevantNodePair> AlwaysRelevantForConnectionList;

	TArray<FVisibilityInfo*> TeamVisibilityInfos;

	/** Actors that are only supposed to replicate to their owning connection, but that did not
	have a connection on spawn */
	UPROPERTY()
	TArray<AActor *> ActorsWithoutNetConnection;

	/* Selectables that did not have a connection on spawn */
	UPROPERTY()
	TArray<AActor*> BuildingsWithoutNetConnection;

	UPROPERTY()
	TArray<AActor*> InfantryWithoutNetConnection;

	/* Maps UClass to its mapping policy */
	TClassMap<EClassRepNodeMapping> ClassRepNodePolicies;

public:

	/* [Workaround] This exists because UReplicationGraph::ReplicationGraphFrame is private. 
	Note I have not modified GetReplicationGraphFrame() to use this but it appears it's *only* 
	used for debugging so I won't bother with it */
	uint32 RTSReplicationGraphFrame = 0;

	// Functions only overridden to change ReplicationGraphFrame to RTSReplicationGraphFrame
	virtual void ForceNetUpdate(AActor * Actor) override;
	virtual void FlushNetDormancy(AActor * Actor, bool bWasDormInitial) override;
};

//------------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Connection Class -------
//==============================================================================================
//------------------------------------------------------------------------------------------------

/* This class so far is just an exact copy of UNetReplicationGraphConnection but some of the 
access specificers have been made usable by URTSReplicationGraph */
UCLASS(Transient)
class RTS_VER2_API URTSReplicationGraphConnection : public UNetReplicationGraphConnection
{
	GENERATED_BODY()

	friend URTSReplicationGraph;

private:

	bool PrepareForReplication();

	virtual void NotifyAddDestructionInfo(FActorDestructionInfo * DestructInfo) override;

	virtual void NotifyRemoveDestructionInfo(FActorDestructionInfo * DestructInfo) override;

	virtual void NotifyResetDestructionInfo() override;

	int64 ReplicateDestructionInfos(const FVector& ConnectionViewLocation, const float DestructInfoMaxDistanceSquared);

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	/** DestructionInfo handling. This is how we send "delete this actor" to clients when the actor is deleted on the server (placed in map actors) */
	struct FCachedDestructInfo
	{
		FCachedDestructInfo(FActorDestructionInfo* InDestructInfo) : DestructionInfo(InDestructInfo), CachedPosition(InDestructInfo->DestroyedPosition) { }
		bool operator==(const FActorDestructionInfo* InDestructInfo) const { return InDestructInfo == DestructionInfo; };

		FActorDestructionInfo* DestructionInfo;
		FVector CachedPosition;
	};

	/* [Workaround] Here because PendingDestructInfoList is private */
	TArray<FCachedDestructInfo> RTSPendingDestructInfoList;
};


//------------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Graph Nodes -------
//==============================================================================================
//------------------------------------------------------------------------------------------------


UCLASS()
class URTSReplicationGraphNode_ActorList : public UReplicationGraphNode
{
	GENERATED_BODY()

public:

	URTSReplicationGraphNode_ActorList();

	void SetupNode();

	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo) override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void NotifyResetAllNetworkActors() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params) override;
	virtual void LogNode(FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName) const override;
	virtual void GetAllActorsInNode_Debugging(TArray<FActorRepListType>& OutArray) const override;

protected:

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	// The list of actors
	FActorRepListRefView ReplicationActorList;
};


/* Actors that are always relevant to everyone */
UCLASS()
class URTSReplicationGraphNode_AlwaysRelevant : public URTSReplicationGraphNode_ActorList
{
	GENERATED_BODY()
};


/* Actors that are only relevant to a certain connection */
UCLASS()
class URTSReplicationGraphNode_AlwaysRelevantForConnection : public URTSReplicationGraphNode_ActorList
{
	GENERATED_BODY()
};


/* One of these exist for each team. Contains the buildings on that team */
UCLASS()
class URTSReplicationGraphNode_TeamBuildings : public URTSReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:

	void SetupNode(ETeam InTeam, ARTSGameState * GameState);

	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo) override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void NotifyResetAllNetworkActors() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params) override;

protected:

	//-------------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------------

	/* Kind of odd but during GatherActorListsForConnection only an array of elements can be added,
	not a single element, so this is the array to add */
	TArray<AActor*> Buildings;

	// Team this node is for
	ETeam Team;
};


/* One of these exist for each team. Contains the infantry on that team */
UCLASS()
class URTSReplicationGraphNode_TeamInfantry : public URTSReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:

	void SetupNode(ETeam InTeam, ARTSGameState * GameState);

	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo) override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void NotifyResetAllNetworkActors() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params) override;

protected:

	//-------------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------------

	TArray<AActor*> Infantry;

	// Team this node is for
	ETeam Team;
};


// What about match observers? 
 
// May need a "recently spawned node". This is to accomodate the reliable multicast system. 
// We need to make sure clients become aware of newly spawned actors so we don't have to 
// queue an unexewcuted RPC. Man this reliable RPC system... considering ditching it for 
// repped variables

// Alternatives ideas:
// - Just a crap ton of replicated variables.
// - an unreliable RPC queue I handle myself somehow.
// - something different

// So for upgrades: could use a single 64 bit bit field
