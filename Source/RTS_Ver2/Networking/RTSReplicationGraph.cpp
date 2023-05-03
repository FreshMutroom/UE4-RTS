// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSReplicationGraph.h"
#include "Engine/NetConnection.h"
#include "Public/GameplayDebuggerCategoryReplicator.h"
#include "Net/RepLayout.h"
#include "Engine/ChildConnection.h"

#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSPlayerController.h"
#include "Miscellaneous/PlayerCamera.h"
#include "MapElements/Building.h"
#include "MapElements/Infantry.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"


int32 CVar_RepGraph_Pause = 0;
static FAutoConsoleVariableRef CVarRepGraphPause(TEXT("Net.RepGraph.Pause"), CVar_RepGraph_Pause, TEXT("Pauses actor replication in the Replication Graph."), ECVF_Default);

int32 CVar_RepGraph_Frequency = 0.00f;
static FAutoConsoleVariableRef CVarRepGraphFrequency(TEXT("Net.RepGraph.Frequency"), CVar_RepGraph_Frequency, TEXT("Enabled Replication Manager. 0 will fallback to legacy NetDriver implementation."), ECVF_Default);

int32 CVar_RepGraph_PrintTrackClassReplication = 0;
static FAutoConsoleVariableRef CVarRepGraphPrintTrackClassReplication(TEXT("Net.RepGraph.PrintTrackClassReplication"), CVar_RepGraph_PrintTrackClassReplication, TEXT(""), ECVF_Default);

DECLARE_STATS_GROUP(TEXT("ReplicationDriver"), STATGROUP_RepDriver, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Actor Channels Closed"), STAT_NetActorChannelsClosed, STATGROUP_RepDriver);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Processed Connections"), STAT_NumProcessedConnections, STATGROUP_RepDriver);

// Helper CVar for debugging. Set this string to conditionally log/breakpoint various points in the repgraph pipeline. Useful for bugs like "why is this actor channel closing"
static TAutoConsoleVariable<FString> CVarRepGraphConditionalBreakpointActorName(TEXT("Net.RepGraph.ConditionalBreakpointActorName"), TEXT(""), TEXT(""), ECVF_Default);

// Variable that can be programatically set to a specific actor/connection 
FActorConnectionPair DebugActorConnectionPair;

FORCEINLINE bool RepGraphConditionalActorBreakpoint(AActor* Actor, UNetConnection* NetConnection)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarRepGraphConditionalBreakpointActorName.GetValueOnGameThread().Len() > 0 && GetNameSafe(Actor).Contains(CVarRepGraphConditionalBreakpointActorName.GetValueOnGameThread()))
	{
		return true;
	}

	// Alternatively, DebugActorConnectionPair can be set by code to catch a specific actor/connection pair 
	if (DebugActorConnectionPair.Actor.Get() == Actor && (DebugActorConnectionPair.Connection == nullptr || DebugActorConnectionPair.Connection == NetConnection))
	{
		return true;
	}
#endif
	return false;
}


URTSReplicationGraph::URTSReplicationGraph()
{
	ReplicationConnectionManagerClass = URTSReplicationGraphConnection::StaticClass();
}

void URTSReplicationGraph::InitGlobalActorClassSettings()
{
	Super::InitGlobalActorClassSettings();

	// ReplicationGraph stores internal associative data for actor classes. 
	// We build this data here based on actor CDO values.
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
		
		// Never cull based on distance
		ClassInfo.CullDistanceSquared = 0.f;

		GlobalActorReplicationInfoMap.SetClassInfo(Class, ClassInfo);
	}

	/* Explicity set class routing policies */
	ClassRepNodePolicies.Set(AActor::StaticClass(), EClassRepNodeMapping::NotRouted);
	ClassRepNodePolicies.Set(ARTSGameState::StaticClass(), EClassRepNodeMapping::RelevantAllConnections);
	// As at time of writing this player states only have replicated variables only the owning 
	// player needs to know about... except bIsABot. It's sent on initial bunch only, do we 
	// still need to make it replicate for all connections to have it sent? For now have made 
	// it relevant to all connections but may be able to reduce it to just owner
	ClassRepNodePolicies.Set(ARTSPlayerState::StaticClass(), EClassRepNodeMapping::RelevantAllConnections);
	ClassRepNodePolicies.Set(ARTSPlayerController::StaticClass(), EClassRepNodeMapping::RelevantOwnerOnly);
	ClassRepNodePolicies.Set(APlayerCamera::StaticClass(), EClassRepNodeMapping::RelevantOwnerOnly);
	ClassRepNodePolicies.Set(ABuilding::StaticClass(), EClassRepNodeMapping::PlayerOwnedSelectable_Building);
	ClassRepNodePolicies.Set(AInfantry::StaticClass(), EClassRepNodeMapping::PlayerOwnedSelectable_Infantry);
}

void URTSReplicationGraph::InitGlobalGraphNodes()
{
	// Do this because UShooterReplicationGraph does it
	PreAllocateRepList(3, 12);
	PreAllocateRepList(6, 12);
	PreAllocateRepList(128, 64);
	PreAllocateRepList(512, 16);

	//-------------------------------------------------------
	//	Always relevant (to all connections) node
	//-------------------------------------------------------

	AlwaysRelevantNode = CreateNewNode<URTSReplicationGraphNode_AlwaysRelevant>();
	AlwaysRelevantNode->SetupNode();
	AddGlobalGraphNode(AlwaysRelevantNode);
}

void URTSReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection * ConnectionManager)
{
	Super::InitConnectionGraphNodes(ConnectionManager);

	//--------------------------------------------------------
	//	Always relevant for connection node
	//--------------------------------------------------------

	URTSReplicationGraphNode_AlwaysRelevantForConnection * AlwaysRelevantNodeForConnection = CreateNewNode<URTSReplicationGraphNode_AlwaysRelevantForConnection>();
	AlwaysRelevantNodeForConnection->SetupNode();
	AddConnectionGraphNode(AlwaysRelevantNodeForConnection, ConnectionManager);

	AlwaysRelevantForConnectionList.Emplace(ConnectionManager->NetConnection, AlwaysRelevantNodeForConnection);
}

void URTSReplicationGraph::NotifyOfNumTeams(uint8 NumTeams, ARTSGameState * GameState)
{
	//-------------------------------------------------------
	//	Team nodes. Create one for each team
	//-------------------------------------------------------

	TeamBuildingNodes.Reserve(NumTeams);
	for (uint8 i = 0; i < NumTeams; ++i)
	{
		TeamBuildingNodes.Emplace(CreateNewNode<URTSReplicationGraphNode_TeamBuildings>());
		TeamBuildingNodes[i]->SetupNode(Statics::ArrayIndexToTeam(i), GameState);
		AddGlobalGraphNode(TeamBuildingNodes[i]);
	}

	TeamInfantryNodes.Reserve(NumTeams);
	for (uint8 i = 0; i < NumTeams; ++i)
	{
		TeamInfantryNodes.Emplace(CreateNewNode<URTSReplicationGraphNode_TeamInfantry>());
		TeamInfantryNodes[i]->SetupNode(Statics::ArrayIndexToTeam(i), GameState);
		AddGlobalGraphNode(TeamInfantryNodes[i]);
	}

	//-------------------------------------------------
	//	Team visibility info references
	//-------------------------------------------------

	TeamVisibilityInfos.Reserve(NumTeams);
	for (int32 i = 0; i < NumTeams; ++i)
	{
		TeamVisibilityInfos.Emplace(&GameState->GetTeamVisibilityInfo(Statics::ArrayIndexToTeam(i)));
	}
}

void URTSReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalInfo)
{
	const EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);

	switch (Policy)
	{
	case EClassRepNodeMapping::NotRouted:
	{
		break;
	}

	case EClassRepNodeMapping::RelevantAllConnections:
	{
		AlwaysRelevantNode->NotifyAddNetworkActor(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::RelevantOwnerOnly:
	{
		/* Doing it how UBasicReplicationGraph does it. We assume it's owner may not be
		known yet, take it safe by adding it to another list and deal with it later
		during ServerReplicateActors. Isn't there some way to check now though? */
		ActorsWithoutNetConnection.Add(ActorInfo.Actor);
		break;
	}

	case EClassRepNodeMapping::PlayerOwnedSelectable_Building:
	{
		BuildingsWithoutNetConnection.Add(ActorInfo.Actor);
		break;
	}

	case EClassRepNodeMapping::PlayerOwnedSelectable_Infantry:
	{
		InfantryWithoutNetConnection.Add(ActorInfo.Actor);
		break;
	}
	}
}

void URTSReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo & ActorInfo)
{
	const EClassRepNodeMapping Policy = GetMappingPolicy(ActorInfo.Class);
	switch (Policy)
	{
	case EClassRepNodeMapping::NotRouted:
	{
		break;
	}

	case EClassRepNodeMapping::RelevantAllConnections:
	{
		AlwaysRelevantNode->NotifyRemoveNetworkActor(ActorInfo);
		break;
	}

	case EClassRepNodeMapping::RelevantOwnerOnly:
	{
		if (URTSReplicationGraphNode_AlwaysRelevantForConnection * Node = GetAlwaysRelevantNodeForConnection(ActorInfo.Actor->GetNetConnection()))
		{
			Node->NotifyRemoveNetworkActor(ActorInfo);
		}
		break;
	}

	case EClassRepNodeMapping::PlayerOwnedSelectable_Building:
	{
		const ETeam ActorsTeam = CastChecked<ABuilding>(ActorInfo.Actor)->GetTeam();
		if (ActorsTeam == ETeam::Uninitialized)
		{
			const bool bRemovedSomething = BuildingsWithoutNetConnection.RemoveSingleSwap(ActorInfo.Actor);
			assert(bRemovedSomething);
		}
		else
		{
			GetBuildingsNodeForTeam(ActorsTeam)->NotifyRemoveNetworkActor(ActorInfo);
		}
		break;
	}

	case EClassRepNodeMapping::PlayerOwnedSelectable_Infantry:
	{
		const ETeam ActorsTeam = CastChecked<AInfantry>(ActorInfo.Actor)->GetTeam();
		if (ActorsTeam == ETeam::Uninitialized)
		{
			const bool bRemovedSomething = InfantryWithoutNetConnection.RemoveSingleSwap(ActorInfo.Actor);
			assert(bRemovedSomething);
		}
		else
		{
			GetInfantryNodeForTeam(ActorsTeam)->NotifyRemoveNetworkActor(ActorInfo);
		}
		break;
	}
	}
}

FORCEINLINE bool ReadyForNextReplication(FConnectionReplicationActorInfo& ConnectionData, FGlobalActorReplicationInfo& GlobalData, const uint32 FrameNum)
{
	return (ConnectionData.NextReplicationFrameNum <= FrameNum || GlobalData.ForceNetUpdateFrame > ConnectionData.LastRepFrameNum);
}

FNativeClassAccumulator ChangeClassAccumulator;
FNativeClassAccumulator NoChangeClassAccumulator;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
bool bTrackClassReplication = false;
#else
const bool bTrackClassReplication = false;
#endif

int32 URTSReplicationGraph::ServerReplicateActors(float DeltaSeconds)
{
	// Route Actors needing owning net connections to appropriate nodes
	for (int32 Index = ActorsWithoutNetConnection.Num() - 1; Index >= 0; --Index)
	{
		bool bRemove = false;
		if (AActor * Actor = ActorsWithoutNetConnection[Index])
		{
			/* Here we check if connection is now valid but also check if they're owned by server 
			player. If yes then we want to remove them from 
			the array otherwise they will stay here for the remaininer of the match */
			UNetConnection * Connection = Actor->GetNetConnection();
			if (Connection != nullptr || Actor->GetNetOwningPlayer() == GetWorld()->GetFirstLocalPlayerFromController())
			{
				bRemove = true;
				if (URTSReplicationGraphNode_AlwaysRelevantForConnection * Node = GetAlwaysRelevantNodeForConnection(Actor->GetNetConnection()))
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

	// Route buildings to appropriate nodes
	for (int32 Index = BuildingsWithoutNetConnection.Num() - 1; Index >= 0; --Index)
	{
		bool bRemove = false;
		if (ABuilding * Actor = static_cast<ABuilding*>(BuildingsWithoutNetConnection[Index]))
		{
			if (HasSelectableSetUp<ABuilding>(Actor))
			{
				bRemove = true;

				const ETeam ActorsTeam = Actor->GetTeam();
				if (URTSReplicationGraphNode_TeamBuildings * Node = GetBuildingsNodeForTeam(ActorsTeam))
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
			BuildingsWithoutNetConnection.RemoveAtSwap(Index, 1, false);
		}
	}

	// Route infantry to appropriate nodes
	for (int32 Index = InfantryWithoutNetConnection.Num() - 1; Index >= 0; --Index)
	{
		bool bRemove = false;
		if (AInfantry * Actor = static_cast<AInfantry*>(InfantryWithoutNetConnection[Index]))
		{
			if (HasSelectableSetUp<AInfantry>(Actor))
			{
				bRemove = true;

				const ETeam ActorsTeam = Actor->GetTeam();
				if (URTSReplicationGraphNode_TeamInfantry * Node = GetInfantryNodeForTeam(ActorsTeam))
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
			InfantryWithoutNetConnection.RemoveAtSwap(Index, 1, false);
		}
	}

	 
	//----------------------------------------------------------------------------------
	//	Begin mostly copy of UReplicationGraph::ServerReplicateActors
	//----------------------------------------------------------------------------------
	//	Some notable changes:
	//	- removed the pawn correction code - pawn is the player camera and we don't care 
	//	where it is.
	//	- removed the code that closes stale channels. This prevents non-relevants from 
	//	being destroyed on clients. Instead they just do not receive updates. What 
	//	impact this has on performance I do not know, I hope it is not too much or 
	//	is even better for server performance. But I don't think it will be.
	//	A better approach is to let the server close the channels which helps its 
	//	performance but when the message gets to the clients they do not destroy the 
	//	actor. Wonder if that is possible.
	//----------------------------------------------------------------------------------

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVar_RepGraph_Pause)
	{
		return 0;
	}
	
	// Temp Hack for frequency throttling
	static float TimeLeft = CVar_RepGraph_Frequency;
	TimeLeft -= DeltaSeconds;
	if (TimeLeft > 0.f)
	{
		return 0;
	}
	TimeLeft = CVar_RepGraph_Frequency;
#endif

	SCOPED_NAMED_EVENT(UReplicationGraph_ServerReplicateActors, FColor::Green);

	++NetDriver->ReplicationFrame;	// This counter is used by RepLayout to utilize CL/serialization sharing. We must increment it ourselves, but other places can increment it too, in order to invalidate the shared state.
	const uint32 FrameNum = ++RTSReplicationGraphFrame;

	// -------------------------------------------------------
	//	PREPARE (Global)
	// -------------------------------------------------------

	{
		QUICK_SCOPE_CYCLE_COUNTER(NET_PrepareReplication);

		for (UReplicationGraphNode * Node : PrepareForReplicationNodes)
		{
			Node->PrepareForReplication();
		}
	}

	// -------------------------------------------------------
	// For Each Connection
	// -------------------------------------------------------

	FGatheredReplicationActorLists GatheredReplicationListsForConnection;

	for (UNetReplicationGraphConnection * ConnManager : Connections)
	{
		URTSReplicationGraphConnection * ConnectionManager = CastChecked<URTSReplicationGraphConnection>(ConnManager);
		
		if (ConnectionManager->PrepareForReplication() == false)
		{
			// Connection is not ready to replicate
			continue;
		}

		UNetConnection * const NetConnection = ConnectionManager->NetConnection;
		FPerConnectionActorInfoMap& ConnectionActorInfoMap = ConnectionManager->ActorInfoMap;
		
		repCheckf(NetConnection->GetReplicationConnectionDriver() == ConnectionManager, TEXT("NetConnection %s mismatch rep driver. %s vs %s"), *GetNameSafe(NetConnection), *GetNameSafe(NetConnection->GetReplicationConnectionDriver()), *GetNameSafe(ConnectionManager));
		
		FBitWriter& ConnectionSendBuffer = NetConnection->SendBuffer;
		
		// --------------------------------------------------------------------------------------------------------------
		// GATHER list of ReplicationLists for this connection
		// --------------------------------------------------------------------------------------------------------------
		 
		FNetViewer Viewer(NetConnection, 0.f);
		const FVector ConnectionViewLocation = Viewer.ViewLocation;
		GatheredReplicationListsForConnection.Reset();
		
		const FConnectionGatherActorListParameters Parameters(Viewer, *ConnectionManager, NetConnection->ClientVisibleLevelNames, FrameNum, GatheredReplicationListsForConnection);
		
		{
			QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_GatherForConnection);
		
			for (UReplicationGraphNode * Node : GlobalGraphNodes)
			{
				Node->GatherActorListsForConnection(Parameters);
			}
		
			for (UReplicationGraphNode * Node : ConnectionManager->GetConnectionGraphNodes())
			{
				Node->GatherActorListsForConnection(Parameters);
			}
		
			if (GatheredReplicationListsForConnection.NumLists() == 0)
			{
				// No lists were returned, kind of weird but not fatal. Early out because code below assumes at least 1 list
				UE_LOG(RTSLOG, Warning, TEXT("No Replication Lists were returned for connection"));
				continue; // Engine source has return 0 here. I feel like it should be continue instead
			}
		}
		
		// --------------------------------------------------------------------------------------------------------------
		// PROCESS gathered replication lists
		// --------------------------------------------------------------------------------------------------------------
		{
			QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_ProcessGatheredLists);
		
			ReplicateActorListsForConnection_Default(ConnectionManager, GatheredReplicationListsForConnection, Viewer);
			ReplicateActorListsForConnection_FastShared(ConnectionManager, GatheredReplicationListsForConnection, Viewer);
		}
		
		{
			QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_PostProcessGatheredLists);
		
			// ------------------------------------------
			// Handle stale, no longer relevant, actor channels.
			// ------------------------------------------			
			{ 
				QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_LookForNonRelevantChannels);
				
				//for (auto MapIt = ConnectionActorInfoMap.CreateChannelIterator(); MapIt; ++MapIt)
				//{
				//	FConnectionReplicationActorInfo& ConnectionActorInfo = *MapIt.Value().Get();
				//	UActorChannel* Channel = MapIt.Key();
				//	checkSlow(Channel != nullptr);
				//	ensure(Channel == ConnectionActorInfo.Channel);
				//
				//	if (ConnectionActorInfo.ActorChannelCloseFrameNum > 0 && ConnectionActorInfo.ActorChannelCloseFrameNum <= FrameNum)
				//	{
				//		if (ConnectionActorInfo.Channel->Closing)
				//		{
				//			UE_LOG(RTSLOG, Verbose, TEXT("NET_ReplicateActors_LookForNonRelevantChannels Channel %s is closing. Skipping."), *GetNameSafe(ConnectionActorInfo.Channel));
				//			continue;
				//		}
				//
				//		AActor * Actor = Channel->Actor;
				//		if (ensure(Actor))
				//		{
				//			if (Actor->IsNetStartupActor())
				//				continue;
				//
				//			//UE_CLOG(DebugConnection, LogReplicationGraph, Display, TEXT("Closing Actor Channel:0x%x 0x%X0x%X, %s %d <= %d"), ConnectionActorInfo.Channel, Actor, NetConnection, *GetNameSafe(ConnectionActorInfo.Channel->Actor), ConnectionActorInfo.ActorChannelCloseFrameNum, FrameNum);
				//			if (RepGraphConditionalActorBreakpoint(Actor))
				//			{
				//				UE_LOG(RTSLOG, Display, TEXT("Closing Actor Channel due to timeout: %s. %d <= %d "), *Actor->GetName(), ConnectionActorInfo.ActorChannelCloseFrameNum, FrameNum);
				//			}
				//
				//			INC_DWORD_STAT_BY(STAT_NetActorChannelsClosed, 1);
				//			ConnectionActorInfo.Channel->Close();
				//		}
				//	}
				//}
			}
			
			// ------------------------------------------
			// Handle Destruction Infos. These are actors that have been destroyed on the server but that we need to tell the client about.
			// ------------------------------------------
			{
				QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_ReplicateDestructionInfos);
				ConnectionManager->ReplicateDestructionInfos(ConnectionViewLocation, DestructInfoMaxDistanceSquared);
			}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			{
				RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_ReplicateDebugActor);
				if (ConnectionManager->DebugActor)
				{
					FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(ConnectionManager->DebugActor);
					FConnectionReplicationActorInfo& ActorInfo = ConnectionActorInfoMap.FindOrAdd(ConnectionManager->DebugActor);
					ReplicateSingleActor(ConnectionManager->DebugActor, ActorInfo, GlobalInfo, ConnectionActorInfoMap, NetConnection, FrameNum);
				}
			}
#endif

		}
	}

	SET_DWORD_STAT(STAT_NumProcessedConnections, Connections.Num());

	/// Comment this stuff because I cannot compile with it
	//if (CVar_RepGraph_PrintTrackClassReplication)
	//{
	//	CVar_RepGraph_PrintTrackClassReplication = 0;
	//	UE_LOG(RTSLOG, Display, TEXT("Changed Classes: %s"), *ChangeClassAccumulator.BuildString());
	//	UE_LOG(RTSLOG, Display, TEXT("No Change Classes: %s"), *NoChangeClassAccumulator.BuildString());
	//}
	//
	//CSVTracker.EndReplicationFrame();
	return 0;
}

void URTSReplicationGraph::ReplicateActorListsForConnection_Default(UNetReplicationGraphConnection * ConnectionManager, FGatheredReplicationActorLists & GatheredReplicationListsForConnection, FNetViewer & Viewer)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bEnableFullActorPrioritizationDetails = DO_REPGRAPH_DETAILS(bEnableFullActorPrioritizationDetailsAllConnections || ConnectionManager->bEnableFullActorPrioritizationDetails);
	const bool bDoDistanceCull = (/*CVar_RepGraph_SkipDistanceCull*/ 1 == 0);
	const bool bDoCulledOnConnectionCount = (/*CVar_RepGraph_PrintCulledOnConnectionClasses*/ 0 == 1);
	bTrackClassReplication = (/*CVar_RepGraph_TrackClassReplication*/ 0 > 0 || CVar_RepGraph_PrintTrackClassReplication > 0);
	if (!bTrackClassReplication)
	{
		ChangeClassAccumulator.Reset();
		NoChangeClassAccumulator.Reset();
	}

#else
	const bool bEnableFullActorPrioritizationDetails = false;
	const bool bDoDistanceCull = true;
	const bool bDoCulledOnConnectionCount = false;
#endif

	// Debug accumulators
	FNativeClassAccumulator DormancyClassAccumulator;
	FNativeClassAccumulator DistanceClassAccumulator;

	int32 NumGatheredListsOnConnection = 0;
	int32 NumGatheredActorsOnConnection = 0;
	int32 NumPrioritizedActorsOnConnection = 0;

	UNetConnection * const NetConnection = ConnectionManager->NetConnection;
	FPerConnectionActorInfoMap& ConnectionActorInfoMap = ConnectionManager->ActorInfoMap;
	const uint32 FrameNum = RTSReplicationGraphFrame;

	const FVector ConnectionViewLocation = Viewer.ViewLocation;

	// --------------------------------------------------------------------------------------------------------------
	// PRIORITIZE Gathered Actors For Connection
	// --------------------------------------------------------------------------------------------------------------
	{
		QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_PrioritizeForConnection);

		// We will make a prioritized list for each item in the packet budget. (Each item may accept multiple categories. Each list has one category)
		// This means, depending on the packet budget, a gathered list could end up in multiple prioritized lists. This would not be desirable in most cases but is not explicitly forbidden.

		PrioritizedReplicationList.Reset();
		TArray<FPrioritizedRepList::FItem>* SortingArray = &PrioritizedReplicationList.Items;

		NumGatheredListsOnConnection += GatheredReplicationListsForConnection.NumLists();

		const float MaxDistanceScaling = PrioritizationConstants.MaxDistanceScaling;
		const uint32 MaxFramesSinceLastRep = PrioritizationConstants.MaxFramesSinceLastRep;

		for (FActorRepListRawView& List : GatheredReplicationListsForConnection.GetLists(EActorRepListTypeFlags::Default))
		{
			// Add actors from gathered list
			NumGatheredActorsOnConnection += List.Num();
			for (AActor* Actor : List)
			{
				RG_QUICK_SCOPE_CYCLE_COUNTER(Prioritize_InnerLoop);

				// -----------------------------------------------------------------------------------------------------------------
				//	Prioritize Actor for Connection: this is the main block of code for calculating a final score for this actor
				//		-This is still pretty rough. It would be nice if this was customizable per project without suffering virtual calls.
				// -----------------------------------------------------------------------------------------------------------------

				if (RepGraphConditionalActorBreakpoint(Actor, NetConnection))
				{
					UE_LOG(RTSLOG, Display, TEXT("UReplicationGraph PrioritizeActor: %s"), *Actor->GetName());
				}

				FConnectionReplicationActorInfo& ConnectionData = ConnectionActorInfoMap.FindOrAdd(Actor);

				RG_QUICK_SCOPE_CYCLE_COUNTER(Prioritize_InnerLoop_ConnGlobalLookUp);

				// Skip if dormant on this connection. We want this to always be the first/quickest check.
				if (ConnectionData.bDormantOnConnection)
				{
					DO_REPGRAPH_DETAILS(PrioritizedReplicationList.GetNextSkippedDebugDetails(Actor)->bWasDormant = true);
					if (bDoCulledOnConnectionCount)
					{
						DormancyClassAccumulator.Increment(Actor->GetClass());
					}
					continue;
				}

				FGlobalActorReplicationInfo& GlobalData = GlobalActorReplicationInfoMap.Get(Actor);

				RG_QUICK_SCOPE_CYCLE_COUNTER(Prioritize_InnerLoop_PostGlobalLookUp);

				// Skip if its not time to replicate on this connection yet. We have to look at ForceNetUpdateFrame here. It would be possible to clear
				// NextReplicationFrameNum on all connections when ForceNetUpdate is called. This probably means more work overall per frame though. Something to consider.
				if (!ReadyForNextReplication(ConnectionData, GlobalData, FrameNum))
				{
					DO_REPGRAPH_DETAILS(PrioritizedReplicationList.GetNextSkippedDebugDetails(Actor)->FramesTillNextReplication = (FrameNum - ConnectionData.LastRepFrameNum));
					continue;
				}

				RG_QUICK_SCOPE_CYCLE_COUNTER(Prioritize_InnerLoop_PostReady);

				// Output record for full debugging. This is not used in the actual sorting/prioritization of the list, just for logging/debugging purposes
				FPrioritizedActorFullDebugDetails* DebugDetails = nullptr;
				if (DO_REPGRAPH_DETAILS(UNLIKELY(bEnableFullActorPrioritizationDetails)))
				{
					DO_REPGRAPH_DETAILS(DebugDetails = PrioritizedReplicationList.GetNextFullDebugDetails(Actor));
				}

				float AccumulatedPriority = 0.f;

				// -------------------
				// Distance Scaling
				// -------------------
				if (GlobalData.Settings.DistancePriorityScale > 0.f)
				{
					const float DistSq = (GlobalData.WorldLocation - ConnectionViewLocation).SizeSquared();

					if (bDoDistanceCull && ConnectionData.CullDistanceSquared > 0.f && DistSq > ConnectionData.CullDistanceSquared)
					{
						DO_REPGRAPH_DETAILS(PrioritizedReplicationList.GetNextSkippedDebugDetails(Actor)->DistanceCulled = FMath::Sqrt(DistSq));
						if (bDoCulledOnConnectionCount)
						{
							DistanceClassAccumulator.Increment(Actor->GetClass());
						}
						continue;
					}

					const float DistanceFactor = FMath::Clamp<float>((DistSq) / MaxDistanceScaling, 0.f, 1.f) * GlobalData.Settings.DistancePriorityScale;
					AccumulatedPriority += DistanceFactor;

					if (DO_REPGRAPH_DETAILS(UNLIKELY(DebugDetails)))
					{
						DebugDetails->DistanceSq = DistSq;
						DebugDetails->DistanceFactor = DistanceFactor;
					}
				}

				RG_QUICK_SCOPE_CYCLE_COUNTER(Prioritize_InnerLoop_PostCull);

				// Update the timeout frame number here. (Since this was returned by the graph, regardless if we end up replicating or not, we bump up the timeout frame num. This has to be done here because Distance Scaling can cull the actor
				UpdateActorChannelCloseFrameNum(Actor, ConnectionData, GlobalData, FrameNum, NetConnection);

				//UE_CLOG(DebugConnection, LogReplicationGraph, Display, TEXT("0x%X0x%X ConnectionData.ActorChannelCloseFrameNum=%d on %d"), Actor, NetConnection, ConnectionData.ActorChannelCloseFrameNum, FrameNum);

				// -------------------
				// Starvation Scaling
				// -------------------
				if (GlobalData.Settings.StarvationPriorityScale > 0.f)
				{
					const uint32 FramesSinceLastRep = (FrameNum - ConnectionData.LastRepFrameNum);
					const float StarvationFactor = 1.f - FMath::Clamp<float>((float)FramesSinceLastRep / (float)MaxFramesSinceLastRep, 0.f, 1.f);

					AccumulatedPriority += StarvationFactor;

					if (DO_REPGRAPH_DETAILS(UNLIKELY(DebugDetails)))
					{
						DebugDetails->FramesSinceLastRap = FramesSinceLastRep;
						DebugDetails->StarvationFactor = StarvationFactor;
					}
				}

				// -------------------
				//	Game code priority
				// -------------------

				if (GlobalData.ForceNetUpdateFrame > 0)
				{
					uint32 ForceNetUpdateDelta = GlobalData.ForceNetUpdateFrame - ConnectionData.LastRepFrameNum;
					if (ForceNetUpdateDelta > 0)
					{
						// Note that in legacy ForceNetUpdate did not actually bump priority. This gives us a hard coded bump if we haven't replicated since the last ForceNetUpdate frame.
						AccumulatedPriority += 1.f;

						if (DO_REPGRAPH_DETAILS(UNLIKELY(DebugDetails)))
						{
							DebugDetails->GameCodeScaling = 1.f;
						}
					}
				}

				SortingArray->Emplace(FPrioritizedRepList::FItem(AccumulatedPriority, Actor, &GlobalData, &ConnectionData));
			}
		}

		{
			// Sort the merged priority list. We could potentially move this into the replicate loop below, this could potentially save use from sorting arrays that don't fit into the budget
			RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_PrioritizeForConnection_Sort);
			NumPrioritizedActorsOnConnection += SortingArray->Num();
			SortingArray->Sort();
		}
	}

	// --------------------------------------------------------------------------------------------------------------
	// REPLICATE Actors For Connection
	// --------------------------------------------------------------------------------------------------------------
	{
		QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_ReplicateActorsForConnection);

		for (int32 ActorIdx = 0; ActorIdx < PrioritizedReplicationList.Items.Num(); ++ActorIdx)
		{
			const FPrioritizedRepList::FItem& RepItem = PrioritizedReplicationList.Items[ActorIdx];

			AActor* Actor = RepItem.Actor;
			FConnectionReplicationActorInfo& ActorInfo = *RepItem.ConnectionData;

			// Always skip if we've already replicated this frame. This happens if an actor is in more than one replication list
			if (ActorInfo.LastRepFrameNum == FrameNum)
			{
				continue;
			}

			FGlobalActorReplicationInfo& GlobalActorInfo = *RepItem.GlobalData;

			int64 BitsWritten = ReplicateSingleActor(Actor, ActorInfo, GlobalActorInfo, ConnectionActorInfoMap, NetConnection, FrameNum);

			// --------------------------------------------------
			//	Update Packet Budget Tracking
			// --------------------------------------------------

			if (IsConnectionReady(NetConnection) == false)
			{
				// We've exceeded the budget for this category of replication list.
				RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_PartialStarvedActorList);
				HandleStarvedActorList(PrioritizedReplicationList, ActorIdx + 1, ConnectionActorInfoMap, FrameNum);
				GNumSaturatedConnections++;
				break;
			}
		}
	}

	// Broadcast the list we just handled. This is intended to be for debugging/logging features.
	ConnectionManager->OnPostReplicatePrioritizeLists.Broadcast(ConnectionManager, &PrioritizedReplicationList);

	if (bDoCulledOnConnectionCount)
	{
		UE_LOG(RTSLOG, Display, TEXT("Dormant Culled classes: %s"), *DormancyClassAccumulator.BuildString());
		UE_LOG(RTSLOG, Display, TEXT("Dist Culled classes: %s"), *DistanceClassAccumulator.BuildString());
		UE_LOG(RTSLOG, Display, TEXT("Saturated Connections: %d"), GNumSaturatedConnections);
		UE_LOG(RTSLOG, Display, TEXT(""));

		UE_LOG(RTSLOG, Display, TEXT("Gathered Lists: %d Gathered Actors: %d  PrioritizedActors: %d"), NumGatheredListsOnConnection, NumGatheredActorsOnConnection, NumPrioritizedActorsOnConnection);
		UE_LOG(RTSLOG, Display, TEXT("Connection Loaded Streaming Levels: %d"), NetConnection->ClientVisibleLevelNames.Num());
	}
}


struct FScopedQueuedBits
{
	FScopedQueuedBits(int32& InQueuedBits, int32& InTotalBits) : QueuedBits(InQueuedBits), TotalBits(InTotalBits) { }
	~FScopedQueuedBits() { QueuedBits -= TotalBits; }
	int32& QueuedBits;
	int32& TotalBits;
};


// Tracks total bits/cpu and pushes to the CSV profiler

struct FScopedFastPathTracker
{
	FScopedFastPathTracker(UClass* InActorClass, FReplicationGraphCSVTracker& InTracker, int32& InBitsWritten)
#if CSV_PROFILER
		: ActorClass(InActorClass), Tracker(InTracker), BitsWritten(InBitsWritten)
#endif
	{

#if CSV_PROFILER
#if STATS
		bEnabled = true;
#else
		bEnabled = FCsvProfiler::Get()->IsCapturing();
#endif
		if (bEnabled)
		{
			StartTime = FPlatformTime::Seconds();
		}
#endif
	}

#if CSV_PROFILER
	~FScopedFastPathTracker()
	{
		if (bEnabled)
		{
			const double FinalTime = FPlatformTime::Seconds() - StartTime;
			Tracker.PostFastPathReplication(ActorClass, FinalTime, BitsWritten);
		}
	}

	UClass* ActorClass;
	FReplicationGraphCSVTracker& Tracker;
	double StartTime;
	int32& BitsWritten;
	bool bEnabled;
#endif
};


void URTSReplicationGraph::ReplicateActorListsForConnection_FastShared(UNetReplicationGraphConnection * ConnectionManager, FGatheredReplicationActorLists & GatheredReplicationListsForConnection, FNetViewer & Viewer)
{
	if (/*CVar_RepGraph_EnableFastSharedPath*/ 1 == false)
	{
		return;
	}

	if (GatheredReplicationListsForConnection.ContainsLists(EActorRepListTypeFlags::FastShared) == false)
	{
		return;
	}

	FPerConnectionActorInfoMap& ConnectionActorInfoMap = ConnectionManager->ActorInfoMap;
	UNetConnection* const NetConnection = ConnectionManager->NetConnection;
	const uint32 FrameNum = RTSReplicationGraphFrame;
	const FVector ConnectionViewLocation = Viewer.ViewLocation;
	const FVector ConnectionViewDir = Viewer.ViewDir;
	const float FastSharedDistanceRequirementPct = FastSharedPathConstants.DistanceRequirementPct;
	const int64 MaxBits = FastSharedPathConstants.MaxBitsPerFrame;
	const int32 StartIdx = FrameNum * FastSharedPathConstants.ListSkipPerFrame;

	int32 TotalBitsWritten = 0;

	// Fast shared path "doesn't count" towards our normal net send rate. This will subtract the bits we send in this function out of the queued bits on net connection.
	// This really isn't ideal. We want to have better ways of tracking and limiting network traffic. This feels pretty hacky in implementation but conceptually is good.
	FScopedQueuedBits ScopedQueuedBits(NetConnection->QueuedBits, TotalBitsWritten);

	TArray< FActorRepListRawView>& GatheredLists = GatheredReplicationListsForConnection.GetLists(EActorRepListTypeFlags::FastShared);
	for (int32 ListIdx = 0; ListIdx < GatheredLists.Num(); ++ListIdx)
	{
		FActorRepListRawView& List = GatheredLists[(ListIdx + FrameNum) % GatheredLists.Num()];
		for (int32 i = 0; i < List.Num(); ++i)
		{
			// Round robin through the list over multiple frames. We want to avoid sorting this list based on 'time since last rep'. This is a good balance
			AActor* Actor = List[(i + StartIdx) % List.Num()];

			int32 BitsWritten = 0;

			if (RepGraphConditionalActorBreakpoint(Actor, NetConnection))
			{
				UE_LOG(RTSLOG, Display, TEXT("UReplicationGraph FastShared Path Replication: %s"), *Actor->GetName());
			}

			FConnectionReplicationActorInfo& ConnectionData = ConnectionActorInfoMap.FindOrAdd(Actor);

			// Don't fast path rep if we already repped in the default path this frame
			if (UNLIKELY(ConnectionData.LastRepFrameNum == FrameNum))
			{
				continue;
			}

			if (UNLIKELY(ConnectionData.bTearOff))
			{
				continue;
			}

			// Actor channel must already be established to rep fast path
			UActorChannel* ActorChannel = ConnectionData.Channel;
			if (ActorChannel == nullptr || ActorChannel->Closing)
			{
				continue;
			}

			FGlobalActorReplicationInfo& GlobalActorInfo = GlobalActorReplicationInfoMap.Get(Actor);
			if (GlobalActorInfo.Settings.FastSharedReplicationFunc == nullptr)
			{
				// This actor does not support fastshared replication
				// FIXME: we should avoid this by keeping these actors on separate lists
				continue;
			}

			// Simple dot product rejection: only fast rep actors in front of this connection
			const FVector DirToActor = GlobalActorInfo.WorldLocation - ConnectionViewLocation;
			if (FVector::DotProduct(DirToActor, ConnectionViewDir) < 0.f)
			{
				continue;
			}

			// Simple distance cull
			const float DistSq = DirToActor.SizeSquared();
			if (DistSq > (ConnectionData.CullDistanceSquared * FastSharedDistanceRequirementPct))
			{
				continue;
			}

			BitsWritten = (int32)ReplicateSingleActor_FastShared(Actor, ConnectionData, GlobalActorInfo, NetConnection, FrameNum);
			TotalBitsWritten += BitsWritten;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			static bool SkipCheck = false;
			if (SkipCheck)
			{
				continue;
			}
#endif
			if (TotalBitsWritten > MaxBits)
			{
				GNumSaturatedConnections++;
				return;
			}
		}
	}
}

int64 URTSReplicationGraph::ReplicateSingleActor(AActor * Actor, FConnectionReplicationActorInfo & ActorInfo, FGlobalActorReplicationInfo & GlobalActorInfo, FPerConnectionActorInfoMap & ConnectionActorInfoMap, UNetConnection * NetConnection, const uint32 FrameNum)
{
	RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_ReplicateSingleActor);

	if (RepGraphConditionalActorBreakpoint(Actor, NetConnection))
	{
		UE_LOG(RTSLOG, Display, TEXT("UReplicationGraph::ReplicateSingleActor: %s"), *Actor->GetName());
	}

	if (ActorInfo.Channel && ActorInfo.Channel->Closing)
	{
		// We are waiting for the client to ack this actor channel's close bunch.
		return 0;
	}

	ActorInfo.LastRepFrameNum = FrameNum;
	ActorInfo.NextReplicationFrameNum = FrameNum + ActorInfo.ReplicationPeriodFrame;

	UClass* const ActorClass = Actor->GetClass();

	/** Call PreReplication if necessary. */
	if (GlobalActorInfo.LastPreReplicationFrame != FrameNum)
	{
		RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_CallPreReplication);
		GlobalActorInfo.LastPreReplicationFrame = FrameNum;

		Actor->CallPreReplication(NetDriver);
	}

	const bool bWantsToGoDormant = GlobalActorInfo.bWantsToBeDormant;
	TActorRepListViewBase<FActorRepList*> DependentActorList(GlobalActorInfo.DependentActorList.RepList.GetReference());

	if (ActorInfo.Channel == nullptr)
	{
		// Create a new channel for this actor.
		ActorInfo.Channel = (UActorChannel*)NetConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
		if (!ActorInfo.Channel)
		{
			return 0;
		}

		CSVTracker.PostActorChannelCreated(ActorClass);

		//UE_LOG(LogReplicationGraph, Display, TEXT("Created Actor Channel:0x%x 0x%X0x%X, %d"), ActorInfo.Channel, Actor, NetConnection, FrameNum);

		// This will unfortunately cause a callback to this  UNetReplicationGraphConnection and will relook up the ActorInfoMap and set the channel that we already have set.
		// This is currently unavoidable because channels are created from different code paths (some outside of this loop)
		ActorInfo.Channel->SetChannelActor(Actor);
	}

	if (UNLIKELY(bWantsToGoDormant))
	{
		ActorInfo.Channel->StartBecomingDormant();
	}

	int64 BitsWritten = 0;
	const double StartingReplicateActorTimeSeconds = GReplicateActorTimeSeconds;

	if (UNLIKELY(ActorInfo.bTearOff))
	{
		// Replicate and immediately close in tear off case
		BitsWritten = ActorInfo.Channel->ReplicateActor();
		BitsWritten += ActorInfo.Channel->Close(EChannelCloseReason::TearOff);
	}
	else
	{
		// Just replicate normally
		BitsWritten = ActorInfo.Channel->ReplicateActor();
	}

	const double DeltaReplicateActorTimeSeconds = GReplicateActorTimeSeconds - StartingReplicateActorTimeSeconds;

	if (bTrackClassReplication)
	{
		if (BitsWritten > 0)
		{
			ChangeClassAccumulator.Increment(ActorClass);
		}
		else
		{
			NoChangeClassAccumulator.Increment(ActorClass);
		}
	}

	CSVTracker.PostReplicateActor(ActorClass, DeltaReplicateActorTimeSeconds, BitsWritten);

	// ----------------------------
	//	Dependent actors
	// ----------------------------
	if (DependentActorList.IsValid())
	{
		RG_QUICK_SCOPE_CYCLE_COUNTER(NET_ReplicateActors_DependentActors);

		const int32 CloseFrameNum = ActorInfo.ActorChannelCloseFrameNum;

		for (AActor * DependentActor : DependentActorList)
		{
			repCheck(DependentActor);

			FConnectionReplicationActorInfo& DependentActorConnectionInfo = ConnectionActorInfoMap.FindOrAdd(DependentActor);
			FGlobalActorReplicationInfo& DependentActorGlobalData = GlobalActorReplicationInfoMap.Get(DependentActor);

			// Dependent actor channel will stay open as long as the owning actor channel is open
			DependentActorConnectionInfo.ActorChannelCloseFrameNum = FMath::Max<uint32>(CloseFrameNum, DependentActorConnectionInfo.ActorChannelCloseFrameNum);

			if (!ReadyForNextReplication(DependentActorConnectionInfo, DependentActorGlobalData, FrameNum))
			{
				continue;
			}

			//UE_LOG(LogReplicationGraph, Display, TEXT("DependentActor %s %s. NextReplicationFrameNum: %d. FrameNum: %d. ForceNetUpdateFrame: %d. LastRepFrameNum: %d."), *DependentActor->GetPathName(), *NetConnection->GetName(), DependentActorConnectionInfo.NextReplicationFrameNum, FrameNum, DependentActorGlobalData.ForceNetUpdateFrame, DependentActorConnectionInfo.LastRepFrameNum);
			BitsWritten += ReplicateSingleActor(DependentActor, DependentActorConnectionInfo, DependentActorGlobalData, ConnectionActorInfoMap, NetConnection, FrameNum);
		}
	}

	return BitsWritten;
}

bool URTSReplicationGraph::ProcessRemoteFunction(AActor * Actor, UFunction * Function, void * Parameters, FOutParmRec * OutParms, FFrame * Stack, UObject * SubObject)
{
	// ----------------------------------
	// Setup
	// ----------------------------------

	if (RepGraphConditionalActorBreakpoint(Actor, nullptr))
	{
		UE_LOG(RTSLOG, Display, TEXT("UReplicationGraph::ProcessRemoteFunction: %s. Function: %s."), *Actor->GetName(), *GetNameSafe(Function));
	}

	if (IsActorValidForReplication(Actor) == false || Actor->IsActorBeingDestroyed())
	{
		UE_LOG(RTSLOG, Display, TEXT("Destroted or not ready!"));
		return true;
	}

	// get the top most function
	while (Function->GetSuperFunction())
	{
		Function = Function->GetSuperFunction();
	}

	// If we have a subobject, thats who we are actually calling this on. If no subobject, we are calling on the actor.
	UObject* TargetObj = SubObject ? SubObject : Actor;

	// Make sure this function exists for both parties.
	const FClassNetCache* ClassCache = NetDriver->NetCache->GetClassNetCache(TargetObj->GetClass());
	if (!ClassCache)
	{
		UE_LOG(RTSLOG, Warning, TEXT("ClassNetCache empty, not calling %s::%s"), *Actor->GetName(), *Function->GetName());
		return true;
	}

	const FFieldNetCache* FieldCache = ClassCache->GetFromField(Function);
	if (!FieldCache)
	{
		UE_LOG(RTSLOG, Warning, TEXT("FieldCache empty, not calling %s::%s"), *Actor->GetName(), *Function->GetName());
		return true;
	}


	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// FastShared Replication. This is ugly but the idea here is to just fill out the bunch parameters and return so that this bunch can be reused by other connections
	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (FastSharedReplicationBunch)
	{
		// We also cache off a channel so we can call some of the serialization functions on it. This isn't really necessary though and we could break those parts off
		// into a static function.
		if (ensureMsgf(FastSharedReplicationChannel, TEXT("FastSharedReplicationPath set but FastSharedReplicationChannel is not! %s"), *Actor->GetPathName()))
		{
			// It sucks we have to a temp writer like this, but we don't know how big the payload will be until we serialize it
			FNetBitWriter TempWriter(nullptr, 0);
			TSharedPtr<FRepLayout> RepLayout = NetDriver->GetFunctionRepLayout(Function);
			RepLayout->SendPropertiesForRPC(Function, FastSharedReplicationChannel, TempWriter, Parameters);

			FNetBitWriter TempBlockWriter(nullptr, 0);
			FastSharedReplicationChannel->WriteFieldHeaderAndPayload(TempBlockWriter, ClassCache, FieldCache, nullptr, TempWriter, true);

			FastSharedReplicationChannel->WriteContentBlockPayload(TargetObj, *FastSharedReplicationBunch, false, TempBlockWriter);

			FastSharedReplicationBunch = nullptr;
			FastSharedReplicationChannel = nullptr;
		}
		return true;
	}


	// ----------------------------------
	// Multicast
	// ----------------------------------

	if ((Function->FunctionFlags & FUNC_NetMulticast))
	{
		TSharedPtr<FRepLayout> RepLayout = NetDriver->GetFunctionRepLayout(Function);

		TOptional<FVector> ActorLocation;
	
		UNetDriver::ERemoteFunctionSendPolicy SendPolicy = UNetDriver::Default;
		if (/*CVar_RepGraph_EnableRPCSendPolicy*/ 1 > 0)
		{
			if (FRPCSendPolicyInfo * FuncSendPolicy = RPCSendPolicyMap.Find(FObjectKey(Function)))
			{
				if (FuncSendPolicy->bSendImmediately)
				{
					SendPolicy = UNetDriver::ForceSend;
				}
			}
		}

		RepLayout->BuildSharedSerializationForRPC(Parameters);
		FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(Actor);
		const float CullDistanceSquared = GlobalInfo.Settings.CullDistanceSquared;

		bool ForceFlushNetDormancy = false;

		// Cache streaming level name off
		FNewReplicatedActorInfo NewActorInfo(Actor);
		const FName ActorStreamingLevelName = NewActorInfo.StreamingLevelName;

		for (UNetReplicationGraphConnection* Manager : Connections)
		{
			FConnectionReplicationActorInfo& ConnectionActorInfo = Manager->ActorInfoMap.FindOrAdd(Actor);
			UNetConnection* NetConnection = Manager->NetConnection;

			// This connection isn't ready yet
			if (NetConnection->ViewTarget == nullptr)
			{
				continue;
			}

			// Streaming level actor that the client doesn't have loaded. Do not send.
			if (ActorStreamingLevelName != NAME_None && NetConnection->ClientVisibleLevelNames.Contains(ActorStreamingLevelName) == false)
			{
				continue;
			}

			if (ConnectionActorInfo.Channel == nullptr)
			{
				// There is no actor channel here. Ideally we would just ignore this but in the case of net dormancy, this may be an actor that will replicate on the next frame.
				// If the actor is dormant and is a distance culled actor, we can probably safely assume this connection will open a channel for the actor on the next rep frame.
				// This isn't perfect and we may want a per-function or per-actor policy that allows to dictate what happens in this situation.

				// Actors being destroyed (Building hit with rocket) will wake up before this gets hit. So dormancy really cant be relied on here.
				// if (Actor->NetDormancy > DORM_Awake)
				{
					if (CullDistanceSquared > 0)
					{
						FNetViewer Viewer(NetConnection, 0.f);
						if (ActorLocation.IsSet() == false)
						{
							ActorLocation = Actor->GetActorLocation();
						}

						const float DistSq = (ActorLocation.GetValue() - Viewer.ViewLocation).SizeSquared();
						if (DistSq <= CullDistanceSquared)
						{
							// We are within range, we will open a channel now for this actor and call the RPC on it
							ConnectionActorInfo.Channel = (UActorChannel *)NetConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
							ConnectionActorInfo.Channel->SetChannelActor(Actor);

							// Update timeout frame name. We would run into problems if we open the channel, queue a bunch, and then it timeouts before RepGraph replicates properties.
							UpdateActorChannelCloseFrameNum(Actor, ConnectionActorInfo, GlobalInfo, RTSReplicationGraphFrame + 1 /** Plus one to error on safe side. RepFrame num will be incremented in the next tick */, NetConnection);

							// If this actor is dormant on the connection, we will force a flushnetdormancy call.
							ForceFlushNetDormancy |= ConnectionActorInfo.bDormantOnConnection;
						}
					}
				}
			}

			if (ConnectionActorInfo.Channel)
			{
				NetDriver->ProcessRemoteFunctionForChannel(ConnectionActorInfo.Channel, ClassCache, FieldCache, TargetObj, NetConnection, Function, Parameters, OutParms, Stack, true, SendPolicy);

				if (SendPolicy == UNetDriver::ForceSend)
				{
					// Queue the send in an array that we consume in PostTickDispatch to avoid force flushing multiple times a frame on the same connection
					ConnectionsNeedingsPostTickDispatchFlush.AddUnique(NetConnection);
				}

			}
		}

		RepLayout->ClearSharedSerializationForRPC();

		if (ForceFlushNetDormancy)
		{
			Actor->FlushNetDormancy();
		}
		return true;
	}

	// ----------------------------------
	// Single Connection
	// ----------------------------------

	UNetConnection* Connection = Actor->GetNetConnection();
	if (Connection)
	{
		if (((Function->FunctionFlags & FUNC_NetReliable) == 0) && !IsConnectionReady(Connection))
		{
			return true;
		}

		// Route RPC calls to actual connection
		if (Connection->GetUChildConnection())
		{
			Connection = ((UChildConnection*)Connection)->Parent;
		}

		if (Connection->State == USOCK_Closed)
		{
			return true;
		}

		UActorChannel* Ch = Connection->FindActorChannelRef(Actor);
		if (Ch == nullptr)
		{
			if (Actor->IsPendingKillPending() || !NetDriver->IsLevelInitializedForActor(Actor, Connection))
			{
				// We can't open a channel for this actor here
				return true;
			}

			Ch = (UActorChannel *)Connection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
			Ch->SetChannelActor(Actor);
		}

		NetDriver->ProcessRemoteFunctionForChannel(Ch, ClassCache, FieldCache, TargetObj, Connection, Function, Parameters, OutParms, Stack, true);
	}
	else
	{
		UE_LOG(LogNet, Warning, TEXT("UReplicationGraph::ProcessRemoteFunction: No owning connection for actor %s. Function %s will not be processed."), *Actor->GetName(), *Function->GetName());
	}

	// return true because we don't want the net driver to do anything else
	return true;
}

EClassRepNodeMapping URTSReplicationGraph::GetMappingPolicy(UClass * Class)
{
	EClassRepNodeMapping * Policy = ClassRepNodePolicies.Get(Class);

	UE_CLOG(Policy == nullptr, RTSLOG, Fatal, TEXT("Could not get a replication policy for class \"%s\" "), *Class->GetName());

	return *Policy;
}

URTSReplicationGraphNode_AlwaysRelevantForConnection * URTSReplicationGraph::GetAlwaysRelevantNodeForConnection(UNetConnection * Connection)
{
	URTSReplicationGraphNode_AlwaysRelevantForConnection * Node = nullptr;
	if (Connection != nullptr)
	{
		if (FRTSConnectionAlwaysRelevantNodePair * Pair = AlwaysRelevantForConnectionList.FindByKey(Connection))
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

URTSReplicationGraphNode_TeamBuildings * URTSReplicationGraph::GetBuildingsNodeForTeam(ETeam Team)
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	return TeamBuildingNodes[Index];
}

URTSReplicationGraphNode_TeamInfantry * URTSReplicationGraph::GetInfantryNodeForTeam(ETeam Team)
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	return TeamInfantryNodes[Index];
}

const FVisibilityInfo & URTSReplicationGraph::GetTeamVisibilityInfo(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	return *(TeamVisibilityInfos[Index]);
}

void URTSReplicationGraph::NotifyOfBuildingDestroyed(ABuilding * Building)
{
	// NOOP
}

void URTSReplicationGraph::ForceNetUpdate(AActor * Actor)
{
	if (FGlobalActorReplicationInfo * RepInfo = GlobalActorReplicationInfoMap.Find(Actor))
	{
		RepInfo->ForceNetUpdateFrame = RTSReplicationGraphFrame;
		RepInfo->Events.ForceNetUpdate.Broadcast(Actor, *RepInfo);
	}
}

void URTSReplicationGraph::FlushNetDormancy(AActor * Actor, bool bWasDormInitial)
{
	RG_QUICK_SCOPE_CYCLE_COUNTER(UReplicationGraph_FlushNetDormancy);

	if (Actor->IsActorInitialized() == false)
	{
		return;
	}

	FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap.Get(Actor);
	const bool bNewWantsToBeDormant = (Actor->NetDormancy > DORM_Awake);

	if (GlobalInfo.bWantsToBeDormant != bNewWantsToBeDormant)
	{
		UE_LOG(RTSLOG, Display, TEXT("UReplicationGraph::FlushNetDormancy %s. WantsToBeDormant is changing (%d -> %d) from a Flush! We expect NotifyActorDormancyChange to be called first."), *Actor->GetPathName(), (bool)GlobalInfo.bWantsToBeDormant, bNewWantsToBeDormant);
		GlobalInfo.bWantsToBeDormant = Actor->NetDormancy > DORM_Awake;
	}

	if (GlobalInfo.bWantsToBeDormant == false)
	{
		// This actor doesn't want to be dormant. Suppress the Flush call into the nodes. This is to prevent wasted work since the AActor code calls NotifyActorDormancyChange then Flush always.
		return;
	}

	if (RTSReplicationGraphFrame == GlobalInfo.LastFlushNetDormancyFrame)
	{
		// We already did this work this frame, we can early out
		return;
	}

	GlobalInfo.LastFlushNetDormancyFrame = RTSReplicationGraphFrame;


	if (bWasDormInitial)
	{
		AddNetworkActor(Actor);
	}
	else
	{
		GlobalInfo.Events.DormancyFlush.Broadcast(Actor, GlobalInfo);

		// Stinks to have to iterate through like this, especially when net driver is doing a similar thing.
		// Dormancy should probably be rewritten.
		for (UNetReplicationGraphConnection * ConnectionManager : Connections)
		{
			if (FConnectionReplicationActorInfo* Info = ConnectionManager->ActorInfoMap.Find(Actor))
			{
				Info->bDormantOnConnection = false;
			}
		}
	}
}


//===============================================================================================

URTSReplicationGraphNode_ActorList::URTSReplicationGraphNode_ActorList()
{
}

void URTSReplicationGraphNode_ActorList::SetupNode()
{
	/* Do this otherwise AddReplicationActorList calls later on cause crashes. Try it and see.
	This *may* be having a significant effect on PIE load times. */
	ReplicationActorList.Reset();
}

void URTSReplicationGraphNode_ActorList::NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo)
{
	ReplicationActorList.Add(ActorInfo.Actor);
}

bool URTSReplicationGraphNode_ActorList::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound)
{
	/* Would prefer RemoveAtSwap here instead of regular Remove. Is there a reason it's not
	available to use? */
	const bool bRemovedSomething = ReplicationActorList.Remove(ActorInfo.Actor);

	if (bWarnIfNotFound && !bRemovedSomething)
	{
		UE_LOG(RTSLOG, Warning, TEXT("Attempted to remove %s from list %s but it was not found"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *GetFullName());
	}

	return bRemovedSomething;
}

void URTSReplicationGraphNode_ActorList::NotifyResetAllNetworkActors()
{
	ReplicationActorList.Reset();

	for (UReplicationGraphNode * ChildNode : AllChildNodes)
	{
		ChildNode->NotifyResetAllNetworkActors();
	}
}

void URTSReplicationGraphNode_ActorList::GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params)
{
	Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);

	for (UReplicationGraphNode * ChildNode : AllChildNodes)
	{
		ChildNode->GatherActorListsForConnection(Params);
	}
}

void URTSReplicationGraphNode_ActorList::LogNode(FReplicationGraphDebugInfo & DebugInfo, const FString & NodeName) const
{
	DebugInfo.Log(NodeName);
	DebugInfo.PushIndent();

	LogActorRepList(DebugInfo, TEXT("World"), ReplicationActorList);

	for (UReplicationGraphNode * ChildNode : AllChildNodes)
	{
		DebugInfo.PushIndent();
		ChildNode->LogNode(DebugInfo, FString::Printf(TEXT("Child: %s"), *ChildNode->GetName()));
		DebugInfo.PopIndent();
	}
	DebugInfo.PopIndent();
}

void URTSReplicationGraphNode_ActorList::GetAllActorsInNode_Debugging(TArray<FActorRepListType>& OutArray) const
{
	ReplicationActorList.AppendToTArray(OutArray);

	for (UReplicationGraphNode * ChildNode : AllChildNodes)
	{
		ChildNode->GetAllActorsInNode_Debugging(OutArray);
	}
}


//===============================================================================================

void URTSReplicationGraphNode_TeamBuildings::SetupNode(ETeam InTeam, ARTSGameState * GameState)
{
	URTSReplicationGraphNode_ActorList::SetupNode();

	Team = InTeam;
}

void URTSReplicationGraphNode_TeamBuildings::NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo)
{
	/* Yes we do want to add it to this because at gather time if the node is for the connection's
	team then it will just use what's in this list */
	ReplicationActorList.Add(ActorInfo.Actor);

	Buildings.Emplace(ActorInfo.Actor);
}

bool URTSReplicationGraphNode_TeamBuildings::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound)
{
	const bool bFoundActor = Buildings.RemoveSingleSwap(ActorInfo.GetActor());

	if (bWarnIfNotFound && !bFoundActor)
	{
		UE_LOG(RTSLOG, Warning, TEXT("Attempted to remove %s from list %s but it was not found"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *GetFullName());
	}

	return bFoundActor;
}

void URTSReplicationGraphNode_TeamBuildings::NotifyResetAllNetworkActors()
{
	Buildings.Reset();

	Super::NotifyResetAllNetworkActors();
}

void URTSReplicationGraphNode_TeamBuildings::GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params)
{
	const ETeam ConnectionsTeam = CastChecked<ARTSPlayerController>(Params.Viewer.InViewer)->GetTeam();

	if (ConnectionsTeam == Team)
	{
		// Everything on a player's own team is relevant
		Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
	}
	else
	{
		ReplicationActorList.Reset();

		const FVisibilityInfo & TeamVisibilityInfo = CastChecked<URTSReplicationGraph>(GetOuter())->GetTeamVisibilityInfo(ConnectionsTeam);

		/* Anything:
		1. outside fog of war and
		2. not in stealth mode or in stealth mode but revealed by stealth detection
		is relevant. */
		for (const auto & Building : Buildings)
		{
			if (TeamVisibilityInfo.IsVisible(Building))
			{
				ReplicationActorList.Add(Building);
			}
		}

		Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
	}
}


//===============================================================================================

void URTSReplicationGraphNode_TeamInfantry::SetupNode(ETeam InTeam, ARTSGameState * GameState)
{
	URTSReplicationGraphNode_ActorList::SetupNode();

	Team = InTeam;
}

void URTSReplicationGraphNode_TeamInfantry::NotifyAddNetworkActor(const FNewReplicatedActorInfo & ActorInfo)
{
	ReplicationActorList.Add(ActorInfo.Actor);

	Infantry.Emplace(ActorInfo.Actor);
}

bool URTSReplicationGraphNode_TeamInfantry::NotifyRemoveNetworkActor(const FNewReplicatedActorInfo & ActorInfo, bool bWarnIfNotFound)
{
	const bool bFoundActor = Infantry.RemoveSingleSwap(ActorInfo.GetActor());

	if (bWarnIfNotFound && !bFoundActor)
	{
		UE_LOG(RTSLOG, Warning, TEXT("Attempted to remove %s from list %s but it was not found"), *GetActorRepListTypeDebugString(ActorInfo.Actor), *GetFullName());
	}

	return bFoundActor;
}

void URTSReplicationGraphNode_TeamInfantry::NotifyResetAllNetworkActors()
{
	Infantry.Reset();

	Super::NotifyResetAllNetworkActors();
}

void URTSReplicationGraphNode_TeamInfantry::GatherActorListsForConnection(const FConnectionGatherActorListParameters & Params)
{
	const ETeam ConnectionsTeam = CastChecked<ARTSPlayerController>(Params.Viewer.InViewer)->GetTeam();

	if (ConnectionsTeam == Team)
	{
		Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
	}
	else
	{
		ReplicationActorList.Reset();

		const FVisibilityInfo & TeamVisibilityInfo = CastChecked<URTSReplicationGraph>(GetOuter())->GetTeamVisibilityInfo(ConnectionsTeam);

		/* Anything:
		1. outside fog of war and
		2. not in stealth mode or in stealth mode but revealed by stealth detection
		is relevant. */
		for (const auto & Unit : Infantry)
		{
			if (TeamVisibilityInfo.IsVisible(Unit))
			{
				ReplicationActorList.Add(Unit);
			}
		}

		Params.OutGatheredReplicationLists.AddReplicationActorList(ReplicationActorList);
	}
}

bool URTSReplicationGraphConnection::PrepareForReplication()
{
	NetConnection->ViewTarget = NetConnection->PlayerController ? NetConnection->PlayerController->GetViewTarget() : NetConnection->OwningActor;
	return (NetConnection->ViewTarget != nullptr);
}

void URTSReplicationGraphConnection::NotifyAddDestructionInfo(FActorDestructionInfo * DestructInfo)
{
	if (DestructInfo->StreamingLevelName != NAME_None)
	{
		if (NetConnection->ClientVisibleLevelNames.Contains(DestructInfo->StreamingLevelName) == false)
		{
			// This client does not have this streaming level loaded. We should get notified again via UNetConnection::UpdateLevelVisibility
			// (This should be enough. Legacy system would add the info and then do the level check in ::ServerReplicateActors, but this should be unnecessary)
			return;
		}
	}

	RTSPendingDestructInfoList.Emplace(FCachedDestructInfo(DestructInfo));
}

void URTSReplicationGraphConnection::NotifyRemoveDestructionInfo(FActorDestructionInfo * DestructInfo)
{
	int32 RemoveIdx = RTSPendingDestructInfoList.IndexOfByKey(DestructInfo);
	if (RemoveIdx != INDEX_NONE)
	{
		RTSPendingDestructInfoList.RemoveAt(RemoveIdx, 1, false);
	}
}

void URTSReplicationGraphConnection::NotifyResetDestructionInfo()
{
	RTSPendingDestructInfoList.Reset();
}

int64 URTSReplicationGraphConnection::ReplicateDestructionInfos(const FVector & ConnectionViewLocation, const float DestructInfoMaxDistanceSquared)
{
	const float X = ConnectionViewLocation.X;
	const float Y = ConnectionViewLocation.Y;

	int64 NumBits = 0;
	for (int32 idx = RTSPendingDestructInfoList.Num() - 1; idx >= 0; --idx)
	{
		const FCachedDestructInfo& Info = RTSPendingDestructInfoList[idx];
		float DistSquared = FMath::Square(Info.CachedPosition.X - X) + FMath::Square(Info.CachedPosition.Y - Y);

		if (DistSquared < DestructInfoMaxDistanceSquared)
		{
			UActorChannel* Channel = (UActorChannel*)NetConnection->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);
			if (Channel)
			{
				NumBits += Channel->SetChannelActorForDestroy(Info.DestructionInfo);
			}

			RTSPendingDestructInfoList.RemoveAtSwap(idx, 1, false);
		}
	}

	return NumBits;
}
