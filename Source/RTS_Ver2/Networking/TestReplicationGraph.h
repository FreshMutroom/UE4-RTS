// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasicReplicationGraph.h"
#include "TestReplicationGraph.generated.h"

class UTestReplicationGraphNode_AlwaysRelevantForConnection;
class UTestReplicationGraphNode_AlwaysRelevant;


UENUM()
enum class ETestRepPolicy : uint8
{
	NotRouted, 
	RelevantOwnerOnly,
	RelevantAllConnections,
};


USTRUCT()
struct FTestConnectionAlwaysRelevantNodePair
{
	GENERATED_BODY()

	FTestConnectionAlwaysRelevantNodePair() { }
	FTestConnectionAlwaysRelevantNodePair(UNetConnection * InConnection, UTestReplicationGraphNode_AlwaysRelevantForConnection * InNode) : NetConnection(InConnection), Node(InNode) { }
	bool operator==(const UNetConnection* InConnection) const { return InConnection == NetConnection; }

	UPROPERTY()
	UNetConnection * NetConnection = nullptr;

	UPROPERTY()
	UTestReplicationGraphNode_AlwaysRelevantForConnection * Node = nullptr;
};


/**
 *	This replication graph is for testing only. Mainly using this to try and track down problems 
 *	that are happening with URTSReplicationGraph
 * 
 *	To enable this via ini:
 *	[/Script/OnlineSubsystemUtils.IpNetDriver]
 *	ReplicationDriverClassName="/Script/ReplicationGraph.TestReplicationGraph"
 */
UCLASS(Transient, config=Engine)
class RTS_VER2_API UTestReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:

	UTestReplicationGraph();

	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection * ConnectionManager) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo) override;
	virtual int32 ServerReplicateActors(float DeltaSeconds) override;

	UTestReplicationGraphNode_AlwaysRelevantForConnection * GetAlwaysRelevantNodeForConnection(UNetConnection * Connection);

protected:

	//------------------------------------------------
	//	Data
	//------------------------------------------------

	UPROPERTY()
	UTestReplicationGraphNode_AlwaysRelevant * AlwaysRelevantNode;

	UPROPERTY()
	TArray<FTestConnectionAlwaysRelevantNodePair> AlwaysRelevantForConnectionList;

	UPROPERTY()
	TArray<AActor *> ActorsWithoutNetConnection;

	/* Maps UClass to its mapping policy */
	TClassMap<ETestRepPolicy> ClassRepNodePolicies;
};


//------------------------------------------------------------------------------------------------
//================================================================================================
//	------- Graph Nodes -------
//================================================================================================
//------------------------------------------------------------------------------------------------

/**
 *	---------- Note: ----------- 
 *	This inherits from UReplicationGraphNode_ActorList which differs from how I did it in 
 *	URTSReplicationGraph 
 */
UCLASS()
class UTestReplicationGraphNode_ActorList : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY()

public:

	UTestReplicationGraphNode_ActorList();

	void SetupNode();

	virtual void NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo) override;
	virtual bool NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound = true) override;
	virtual void NotifyResetAllNetworkActors() override;
	virtual void GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params) override;
};


UCLASS()
class UTestReplicationGraphNode_AlwaysRelevant : public UTestReplicationGraphNode_ActorList
{
	GENERATED_BODY()
};


UCLASS()
class UTestReplicationGraphNode_AlwaysRelevantForConnection : public UTestReplicationGraphNode_ActorList
{
	GENERATED_BODY()
};


//===============================================================================================


/**
 *	Another replication graph for testing only. 
 *	This is an exact replica of UBasicReplicationGraph yet it behaves differently. I think I'm done 
 *	with this. 
 */
UCLASS(transient, config=Engine)
class RTS_VER2_API UAnotherTestReplicationGraph : public UReplicationGraph
{
	GENERATED_BODY()

public:

	UAnotherTestReplicationGraph();

	virtual void InitGlobalActorClassSettings() override;
	virtual void InitGlobalGraphNodes() override;
	virtual void InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection) override;
	virtual void RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo) override;
	virtual void RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo) override;

	virtual int32 ServerReplicateActors(float DeltaSeconds) override;

	UPROPERTY()
	UReplicationGraphNode_GridSpatialization2D* GridNode;

	UPROPERTY()
	UReplicationGraphNode_ActorList* AlwaysRelevantNode;

	UPROPERTY()
	TArray<FConnectionAlwaysRelevantNodePair> AlwaysRelevantForConnectionList;

	/** Actors that are only supposed to replicate to their owning connection, but that did not have a connection on spawn */
	UPROPERTY()
	TArray<AActor*> ActorsWithoutNetConnection;

	UReplicationGraphNode_AlwaysRelevant_ForConnection* GetAlwaysRelevantNodeForConnection(UNetConnection * Connection);
};
