// Fill out your copyright notice in the Description page of Project Settings.

#include "TestReplicationGraph.h"
#include "Engine/NetConnection.h"

#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSPlayerController.h"
#include "Miscellaneous/PlayerCamera.h"
#include "MapElements/Building.h"
#include "MapElements/Infantry.h"
#include "Statics/DevelopmentStatics.h"


UTestReplicationGraph::UTestReplicationGraph()
{
	ClassRepNodePolicies.Set(AActor::StaticClass(),					ETestRepPolicy::NotRouted);
	ClassRepNodePolicies.Set(ARTSGameState::StaticClass(),			ETestRepPolicy::RelevantAllConnections);
	ClassRepNodePolicies.Set(ARTSPlayerState::StaticClass(),		ETestRepPolicy::RelevantAllConnections);
	ClassRepNodePolicies.Set(ARTSPlayerController::StaticClass(),	ETestRepPolicy::RelevantOwnerOnly);
	ClassRepNodePolicies.Set(APlayerCamera::StaticClass(),			ETestRepPolicy::RelevantOwnerOnly);
	ClassRepNodePolicies.Set(ABuilding::StaticClass(),				ETestRepPolicy::RelevantAllConnections);
	ClassRepNodePolicies.Set(AInfantry::StaticClass(),				ETestRepPolicy::RelevantAllConnections);
}

void UTestReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	for (TObjectIterator<UClass> Iter; Iter; ++Iter)
	{
		UClass * Class = *Iter;

		AActor * ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (ActorCDO == nullptr || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Skip SKEL and REINST classes.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		FClassReplicationInfo ClassInfo;

		// Replication Graph is frame based. Convert NetUpdateFrequency to ReplicationPeriodFrame based on Server MaxTickRate.
		ClassInfo.ReplicationPeriodFrame = FMath::Max<uint32>((uint32)FMath::RoundToFloat(NetDriver->NetServerMaxTickRate / ActorCDO->NetUpdateFrequency), 1);

		GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
	}
}

void UTestReplicationGraph::InitGlobalGraphNodes()
{
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);
	PreAllocateRepList(512, 16);

	//-------------------------------------------------------
	//	Always relevant (to all connections) node
	//-------------------------------------------------------

	AlwaysRelevantNode = CreateNewNode<UTestReplicationGraphNode_AlwaysRelevant>();
	AlwaysRelevantNode->SetupNode();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UTestReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection * ConnectionManager)
{
	Super::InitConnectionGraphNodes(ConnectionManager);

	//--------------------------------------------------------
	//	Always relevant for connection node
	//--------------------------------------------------------

	UTestReplicationGraphNode_AlwaysRelevantForConnection * AlwaysRelevantNodeForConnection = CreateNewNode<UTestReplicationGraphNode_AlwaysRelevantForConnection>();
	AlwaysRelevantNodeForConnection->SetupNode();
	AddConnectionGraphNode(AlwaysRelevantNodeForConnection, ConnectionManager);

	AlwaysRelevantForConnectionList.Emplace(ConnectionManager->NetConnection, AlwaysRelevantNodeForConnection);
}

void UTestReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalInfo)
{
	const ETestRepPolicy Policy = *ClassRepNodePolicies.Get(ActorInfo.Class);

	switch (Policy) 
	{
		case ETestRepPolicy::NotRouted:
		{
			break;
		}
	
		case ETestRepPolicy::RelevantAllConnections:
		{
			AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
			break;
		}

		case ETestRepPolicy::RelevantOwnerOnly:
		{	
			ActorsWithoutNetConnection.Add(ActorInfo.Actor);
			break;
		}
	}
}

void UTestReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo)
{
	const ETestRepPolicy Policy = *ClassRepNodePolicies.Get(ActorInfo.Class);

	switch (Policy)
	{
		case ETestRepPolicy::NotRouted:
		{
			break;
		}

		case ETestRepPolicy::RelevantAllConnections:
		{
			AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
			break;
		}
		
		case ETestRepPolicy::RelevantOwnerOnly:
		{
			if (UTestReplicationGraphNode_AlwaysRelevantForConnection * Node = GetAlwaysRelevantNodeForConnection(ActorInfo.Actor->GetNetConnection()))
			{
				Node->NotifyRemoveNetworkActor(ActorInfo);
			}
			break;
		}
	}
}

int32 UTestReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{
	UPlayer * ServerPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	// Route Actors needing owning net connections to appropriate nodes
	for (int32 Index = ActorsWithoutNetConnection.Num() - 1; Index >= 0; --Index)
	{
		bool bRemove = false;
		if (AActor * Actor = ActorsWithoutNetConnection[Index])
		{
			UNetConnection * Connection = Actor->GetNetConnection();
			if (Connection != nullptr || Actor->GetNetOwningPlayer() == ServerPlayer)
			{
				bRemove = true;
				if (UTestReplicationGraphNode_AlwaysRelevantForConnection * Node = GetAlwaysRelevantNodeForConnection(Actor->GetNetConnection()))
				{
					Node->NotifyAddNetworkActor(FNewReplicatedActorInfo(Actor));
				}
			}
		}
		else
		{
			bRemove = true;
		}

		if (bRemove)
		{
			ActorsWithoutNetConnection.RemoveAtSwap(Index, 1, false);
		}
	}
	
	return Super::ServerReplicateActors(DeltaSeconds);
}

UTestReplicationGraphNode_AlwaysRelevantForConnection * UTestReplicationGraph::GetAlwaysRelevantNodeForConnection(UNetConnection * Connection)
{
	UTestReplicationGraphNode_AlwaysRelevantForConnection * Node = nullptr;
	if (Connection != nullptr)
	{
		if (FTestConnectionAlwaysRelevantNodePair * Pair = AlwaysRelevantForConnectionList.FindByKey(Connection))
		{
			if (Pair->Node != nullptr)
			{
				Node = Pair->Node;
			}
			else
			{
				UE_LOG(RTSLOG, Warning, TEXT("AlwaysRelevantNode for connection %s is null."), *GetNameSafe(Connection));
			}
		}
		else
		{
			UE_LOG(RTSLOG, Warning, TEXT("Could not find AlwaysRelevantNode for connection %s. This should have been created in UBasicReplicationGraph::InitConnectionGraphNodes."), *GetNameSafe(Connection));
		}
	}
	else
	{
		// Basic implementation requires owner is set on spawn that never changes. A more robust graph would have methods or ways of listening for owner to change
		UE_LOG(RTSLOG, Warning, TEXT("Actor: %s is bOnlyRelevantToOwner but does not have an owning Netconnection. It will not be replicated"));
	}

	return Node;
}


//================================================================================================

UTestReplicationGraphNode_ActorList::UTestReplicationGraphNode_ActorList()
{
}

void UTestReplicationGraphNode_ActorList::SetupNode()
{
	ReplicationActorList.Reset();
}

void UTestReplicationGraphNode_ActorList::NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo)
{
	Super::NotifyAddNetworkActor(ActorInfo);
}

bool UTestReplicationGraphNode_ActorList::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound)
{
	return Super::NotifyRemoveNetworkActor(ActorInfo, bWarnIfNotFound);
}

void UTestReplicationGraphNode_ActorList::NotifyResetAllNetworkActors()
{
	Super::NotifyResetAllNetworkActors();
}

void UTestReplicationGraphNode_ActorList::GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params)
{
	Super::GatherActorListsForConnection(Params);
}


//================================================================================================


UAnotherTestReplicationGraph::UAnotherTestReplicationGraph()
{
}

void UAnotherTestReplicationGraph::InitGlobalActorClassSettings()
{
	UReplicationGraph::InitGlobalActorClassSettings();

	// ReplicationGraph stores internal associative data for actor classes. 
	// We build this data here based on actor CDO values.
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		AActor* ActorCDO = Cast<AActor>(Class->GetDefaultObject());
		if (!ActorCDO || !ActorCDO->GetIsReplicated())
		{
			continue;
		}

		// Skip SKEL and REINST classes.
		if (Class->GetName().StartsWith(TEXT("SKEL_")) || Class->GetName().StartsWith(TEXT("REINST_")))
		{
			continue;
		}

		FClassReplicationInfo ClassInfo;

		// Replication Graph is frame based. Convert NetUpdateFrequency to ReplicationPeriodFrame based on Server MaxTickRate.
		ClassInfo.ReplicationPeriodFrame = FMath::Max<uint32>((uint32)FMath::RoundToFloat(NetDriver->NetServerMaxTickRate / ActorCDO->NetUpdateFrequency), 1);

		if (ActorCDO->bAlwaysRelevant || ActorCDO->bOnlyRelevantToOwner)
		{
			ClassInfo.CullDistanceSquared = 0.f;
		}
		else
		{
			ClassInfo.CullDistanceSquared = ActorCDO->NetCullDistanceSquared;
		}

		GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
	}
}

void UAnotherTestReplicationGraph::InitGlobalGraphNodes()
{
	// Preallocate some replication lists.
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);

	// -----------------------------------------------
	//	Spatial Actors
	// -----------------------------------------------

	GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>();
	GridNode->CellSize = 10000.f;
	GridNode->SpatialBias = FVector2D(-WORLD_MAX, -WORLD_MAX);

	AddGlobalGraphNode(GridNode);

	// -----------------------------------------------
	//	Always Relevant (to everyone) Actors
	// -----------------------------------------------
	AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void UAnotherTestReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	UReplicationGraph::InitConnectionGraphNodes(RepGraphConnection);

	UReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantNodeForConnection = CreateNewNode<UReplicationGraphNode_AlwaysRelevant_ForConnection>();
	AddConnectionGraphNode(AlwaysRelevantNodeForConnection, RepGraphConnection);

	AlwaysRelevantForConnectionList.Emplace(RepGraphConnection->NetConnection, AlwaysRelevantNodeForConnection);
}

void UAnotherTestReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo)
{
	if (ActorInfo.Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
	}
	else if (ActorInfo.Actor->bOnlyRelevantToOwner)
	{
		ActorsWithoutNetConnection.Add(ActorInfo.Actor);
	}
	else
	{
		// Note that UReplicationGraphNode_GridSpatialization2D has 3 methods for adding actor based on the mobility of the actor. Since AActor lacks this information, we will
		// add all spatialized actors as dormant actors: meaning they will be treated as possibly dynamic (moving) when not dormant, and as static (not moving) when dormant.
		GridNode->AddActor_Dormancy(ActorInfo, GlobalInfo);
	}
}

void UAnotherTestReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	if (ActorInfo.Actor->bAlwaysRelevant)
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
	}
	else if (ActorInfo.Actor->bOnlyRelevantToOwner)
	{
		if (UReplicationGraphNode* Node = GetAlwaysRelevantNodeForConnection(ActorInfo.Actor->GetNetConnection()))
		{
			Node->NotifyRemoveNetworkActor(ActorInfo);
		}
	}
	else
	{
		GridNode->RemoveActor_Dormancy(ActorInfo);
	}
}

UReplicationGraphNode_AlwaysRelevant_ForConnection* UAnotherTestReplicationGraph::GetAlwaysRelevantNodeForConnection(UNetConnection* Connection)
{
	UReplicationGraphNode_AlwaysRelevant_ForConnection* Node = nullptr;
	if (Connection)
	{
		if (FConnectionAlwaysRelevantNodePair* Pair = AlwaysRelevantForConnectionList.FindByKey(Connection))
		{
			if (Pair->Node)
			{
				Node = Pair->Node;
			}
			else
			{
				UE_LOG(LogNet, Warning, TEXT("AlwaysRelevantNode for connection %s is null."), *GetNameSafe(Connection));
			}
		}
		else
		{
			UE_LOG(LogNet, Warning, TEXT("Could not find AlwaysRelevantNode for connection %s. This should have been created in UBasicReplicationGraph::InitConnectionGraphNodes."), *GetNameSafe(Connection));
		}
	}
	else
	{
		// Basic implementation requires owner is set on spawn that never changes. A more robust graph would have methods or ways of listening for owner to change
		UE_LOG(LogNet, Warning, TEXT("Actor: %s is bOnlyRelevantToOwner but does not have an owning Netconnection. It will not be replicated"));
	}

	return Node;
}

int32 UAnotherTestReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{
	// Route Actors needing owning net connections to appropriate nodes
	for (int32 idx = ActorsWithoutNetConnection.Num() - 1; idx >= 0; --idx)
	{
		bool bRemove = false;
		if (AActor* Actor = ActorsWithoutNetConnection[idx])
		{
			if (UNetConnection* Connection = Actor->GetNetConnection())
			{
				bRemove = true;
				if (UReplicationGraphNode_AlwaysRelevant_ForConnection* Node = GetAlwaysRelevantNodeForConnection(Actor->GetNetConnection()))
				{
					Node->NotifyAddNetworkActor(FNewReplicatedActorInfo(Actor));
				}
			}
		}
		else
		{
			bRemove = true;
		}

		if (bRemove)
		{
			ActorsWithoutNetConnection.RemoveAtSwap(idx, 1, false);
		}
	}


	return UReplicationGraph::ServerReplicateActors(DeltaSeconds);
}
