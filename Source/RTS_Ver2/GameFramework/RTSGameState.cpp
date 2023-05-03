// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameState.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"
#include "Online.h"
#include "Public/TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/NetDriver.h"

#include "Statics/DevelopmentStatics.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSPlayerController.h"
#include "GameFramework/Selectable.h"
#include "Managers/ObjectPoolingManager.h"
#include "MapElements/Building.h"
#include "GameFramework/RTSGameInstance.h"
#include "UI/MainMenu/Lobby.h"
#include "UI/MainMenu/Menus.h"
#include "Networking/RTSGameSession.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Settings/DevelopmentSettings.h"
#include "GameFramework/RTSGameMode.h"
#include "UI/EndOfMatchWidget.h"
#include "Settings/RTSGameUserSettings.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "Managers/CPUControllerTickManager.h"
#include "Managers/UpgradeManager.h"
#include "Networking/RTSReplicationGraph.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Audio Component Container -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

int32 FAudioComponentContainer::Num() const
{
	return Array.Num();
}

bool FAudioComponentContainer::Contains(UFogObeyingAudioComponent * AudioComp) const
{
	return Map.Contains(AudioComp);
}

void FAudioComponentContainer::Add(UFogObeyingAudioComponent * AudioComp)
{
	assert(Array.Contains(AudioComp) == false);
	const int32 Index = Array.Emplace(AudioComp);
	Map.Emplace(AudioComp, Index);
}

void FAudioComponentContainer::RemoveChecked(UFogObeyingAudioComponent * AudioComp)
{
	assert(Map.Contains(AudioComp) == true);
	assert(Array.Contains(AudioComp) == true);

	const int32 Index = Map[AudioComp];
	Map.Remove(AudioComp);
	if (Array.Num() > 1)
	{
		UFogObeyingAudioComponent * LastInArray = Array.Last();
		if (Index != Array.Num() - 1)
		{
			Map[LastInArray] = Index;
		}
		Array.RemoveAtSwap(Index, 1, false);
	}
	else
	{
		assert(Array[0] == AudioComp);
		Array.RemoveAt(0, 1, false);
	}

	assert(Map.Contains(AudioComp) == false);
	assert(Array.Contains(AudioComp) == false);
}

const TArray<UFogObeyingAudioComponent*>& FAudioComponentContainer::GetArray() const
{
	return Array;
}


//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Game state class -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

ARTSGameState::ARTSGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	AllTeams_Overlap.SetAllChannels(ECR_Ignore);

	TimeWhenMatchStarted = -FLT_MAX;
	NextUniquePlayerID = 1;
	MatchLoadingStatus = ELoadingStatus::None;
}

void ARTSGameState::Tick(float DeltaTime)
{
	// Is this even needed?
	Super::Tick(DeltaTime);

#if MULTITHREADED_FOG_OF_WAR
	MultithreadedFogOfWarManager::Get().OnGameThreadTick(DeltaTime);
#endif

	if (HasAuthority())
	{
		/* Code that increments TickCounter. This code will only do one increment per frame but 
		perhaps we should allow more if DeltaTime is high enough? */
		AccumulatedTimeTowardsNextGameTick += DeltaTime;
		if (AccumulatedTimeTowardsNextGameTick > ProjectSettings::GAME_TICK_RATE)
		{
			AccumulatedTimeTowardsNextGameTick -= ProjectSettings::GAME_TICK_RATE;
			
			/* Increment TickCounter and skip past 255. We basically need to reserve 255 as a value 
			that means 'uninitialized'. Without doing this it would be possible for a selectable 
			to spawn while TickCounter is 255, then we would never receive the OnRep for that 
			and would never be able to setup the selectable */
			TickCounter++;
			if (TickCounter == UINT8_MAX)
			{
				TickCounter = 0;
			}

			Server_RegenSelectableResources();
		}
	}
}

void ARTSGameState::BeginPlay()
{
	Super::BeginPlay();

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());

	/* Default initialize resource spots TMap */
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);

		ResourceSpots.Emplace(ResourceType, FResourcesArray());
	}
}

void ARTSGameState::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	/* Consider creating new game state for match if these become too much on performance */
	// Lobby variables. Only consider for replication if not in match but in lobby
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyTeams, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyFactions, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyPlayerStarts, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyMapIndex, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, MatchLoadingStatus, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyPlayers, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyPlayerTypes, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyCPUDifficulties, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyStartingResources, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, LobbyDefeatCondition, !bIsInMatch);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ARTSGameState, bLobbyAreSlotsLocked, !bIsInMatch);
}

void ARTSGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSGameState, TickCounter);

	// Lobby variables 
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyTeams, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyFactions, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyPlayerStarts, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyMapIndex, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, MatchLoadingStatus, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyPlayers, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyPlayerTypes, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyCPUDifficulties, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyStartingResources, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, LobbyDefeatCondition, COND_Custom);
	DOREPLIFETIME_CONDITION(ARTSGameState, bLobbyAreSlotsLocked, COND_Custom);
}

void ARTSGameState::OnRep_TickCounter()
{
	/* Account for the fact that 255 is not a usable value. When incrementing on the server it 
	goes 253, 254, 0, ... */
	const uint8 NumTicksToProcess = (TickCounter < PreviousTickCounterValue)
		? TickCounter - PreviousTickCounterValue - 1 : TickCounter - PreviousTickCounterValue;
	
	PreviousTickCounterValue = TickCounter;

	Client_RegenSelectableResources(NumTicksToProcess);
}

void ARTSGameState::Server_RegenSelectableResources()
{
	// This function always does 1 ticks worth
	
	if (Server_SelectableResourceUsersThatRegen.Num() > 0)
	{
		// No casting overhead right?
		ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
		URTSHUD * HUDWidget = PlayCon->GetHUDWidget();
		
		for (const auto & Elem : Server_SelectableResourceUsersThatRegen)
		{
			Elem->RegenSelectableResourcesFromGameTicks(1, HUDWidget, PlayCon);
		}
	}
}

void ARTSGameState::Client_RegenSelectableResources(uint8 NumGameTicksWorth)
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	URTSHUD * HUDWidget = PlayCon->GetHUDWidget();
	
	for (const auto & Elem : Client_SelectableResourceUsersThatRegen)
	{
		/* Null check, but may need validity check too. This is here because currently 
		we call AActor::Destroy from the server to destroy a selectable and have it 
		replicate to clients. But I would like to change that if possible and have clients 
		know about death due to OnRep_Health. But remember if something isn't replicating 
		its health (possibly because it is in fog) then we do not get that rep update. 
		Could actually probably switch the array to hold TScriptInterface pointers 
		and can validity check on the UObject part of it */
		if (Elem != nullptr)
		{
			// No casting overhead right?
			CastChecked<ISelectable>(Elem)->RegenSelectableResourcesFromGameTicks(NumGameTicksWorth, HUDWidget, PlayCon);
		}
	}
}

void ARTSGameState::InitTeamTraceChannels()
{
	assert(TeamTraceChannels.Num() == 0);
	// Should not be calling this unless number of teams is known
	assert(NumTeams > 0);

	/* Check your DefaultEngine.ini file for the names of each team collision channel (can 
	easily view by looking in config folder here in visual studio's solution explorer).
	Here this for loop is set up to take the first 8 channels as team channels. If you end 
	up changing the channels around you can easily just hardcode which channel is for 
	what team */
	TeamTraceChannels.Reserve(NumTeams);
	for (uint32 i = 0; i < NumTeams; ++i)
	{
		const uint64 Channel = ECollisionChannel::ECC_GameTraceChannel1 + i;

		TeamTraceChannels.Emplace(Channel);

		/* Setup the "all teams" query params along the way */
		AllTeamsQueryParams.AddObjectTypesToQuery(Statics::IntToCollisionChannel(Channel));

		/* Setup this one aswell */
		AllTeams_Overlap.SetResponse(Statics::IntToCollisionChannel(Channel), ECR_Overlap);
	}

	/* Do the neutral team trace channel too. Change this for your project if needed. 
	Note: I do not think I have actually created a channel for this, will need to 
	eventually */
	NeutralTraceChannel = Statics::IntToCollisionChannel(ECollisionChannel::ECC_GameTraceChannel1 + NumTeams);
}

void ARTSGameState::SetPoolingManager(AObjectPoolingManager * NewValue)
{
	assert(NewValue != nullptr);
	PoolingManager = NewValue;
}

void ARTSGameState::SetupEffectsActors()
{
	/* Spawn the AInfo managers for each ability. 
	If the button uses the same effect as a previous button then we just point it to that 
	previous buttons effect actor instead of spawning another unecessary actor */

	TMap < TSubclassOf < AAbilityBase >, AAbilityBase * > NoDuplicates;

	for (int32 i = 0; i < GI->GetAllContextInfo().Num(); ++i)
	{
		const FContextButtonInfo & Info = GI->GetAllContextInfo()[i];

		const EContextButton AsContextButton = Statics::ArrayIndexToContextButton(i);

		TSubclassOf < AAbilityBase > AbilityBP = Info.GetEffectBP();
		if (NoDuplicates.Contains(AbilityBP))
		{
			ContextActionEffects.Emplace(AsContextButton, NoDuplicates[AbilityBP]);
		}
		else
		{
			/* Effect_BP is optional for some effects such as common actions and therefore is null
			checked first */
			if (AbilityBP != nullptr)
			{
				AAbilityBase * EffectActor = GetWorld()->SpawnActor<AAbilityBase>(AbilityBP, 
					Statics::INFO_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
				assert(EffectActor != nullptr);

				NoDuplicates.Emplace(AbilityBP, EffectActor);

				ContextActionEffects.Emplace(AsContextButton, EffectActor);
			}
		}
	}
}

void ARTSGameState::SetupCommanderAbilityEffects()
{
	TMap <TSubclassOf<UCommanderAbilityBase>, UCommanderAbilityBase *> NoDuplicates;

	const TArray <FCommanderAbilityInfo> & AbilityInfoArray = GI->GetAllCommanderAbilities();
	for (int32 i = 0; i < AbilityInfoArray.Num(); ++i)
	{
		const ECommanderAbility AbilityType = Statics::ArrayIndexToCommanderAbilityType(i);

		TSubclassOf<UCommanderAbilityBase> Effect_BP = AbilityInfoArray[i].GetEffectBP();

		if (NoDuplicates.Contains(Effect_BP))
		{
			/* Do not spawn another effect UObject, just point to the one already created by 
			some other ability */
			CommanderAbilityEffects.Emplace(AbilityType, NoDuplicates[Effect_BP]);
		}
		else
		{
			/* Currently there are no abilities that can be created without setting this, but if 
			there ever are then change this assert to an if statement */
			UE_CLOG(Effect_BP == nullptr, RTSLOG, Fatal, TEXT("Commander ability [%s] does not have "
				"an effect object set. Need to set one"), TO_STRING(ECommanderAbility, AbilityType));
			
			// This IS creating a UObject using my blueprint right and not the default UCommanderAbilityBase class?
			// Also outer may be able to be NULL because it will be placed in a UPROEPRTY container 
			// on this object
			UCommanderAbilityBase * AbilityObject = NewObject<UCommanderAbilityBase>(this, Effect_BP);
			
			CommanderAbilityEffects.Emplace(AbilityType, AbilityObject);

			NoDuplicates.Emplace(Effect_BP, AbilityObject);
		}
	}
}

void ARTSGameState::SetupTeamTags()
{
	/* Assuming this is already empty, but if it's not then just Empty() it first */
	assert(TeamTags.Num() == 0);

	/* Even if team isn't present in match need a tag for all teams possible for observers */
	TeamTags.Reserve(ProjectSettings::MAX_NUM_TEAMS + 2);
	for (uint8 i = 0; i < ProjectSettings::MAX_NUM_TEAMS; ++i)
	{
		TeamTags.Emplace(Statics::GenerateTeamTag(i));
	}

	/* Add on the neutral and observer tags */
	TeamTags.Emplace(Statics::NEUTRAL_TEAM_TAG);
	TeamTags.Emplace(Statics::OBSERVER_TEAM_TAG);
}

void ARTSGameState::AddController(ARTSPlayerController * NewController)
{
	assert(PlayerControllers.Contains(NewController) == false);

	PlayerControllers.Emplace(NewController);
}

void ARTSGameState::SetupForMatch(AObjectPoolingManager * InPoolingManager, uint8 InNumTeams)
{
	// Check if we have called this already
	if (NumTeams > 0)
	{
		return;
	}

	GI = CastChecked<URTSGameInstance>(GetGameInstance());

	NumTeams = InNumTeams;

	/* Initialize Teams array */
	Teams.Init(FPlayerStateArray(), InNumTeams);

	/* Initialize VisibilityInfo */
	VisibilityInfo.Init(FVisibilityInfo(), InNumTeams);

	/* Initialize TeamTraceChannels */
	InitTeamTraceChannels();

	SetPoolingManager(InPoolingManager);
	SetupEffectsActors();
	SetupCommanderAbilityEffects();
	SetupTeamTags();

	if (HasAuthority())
	{
		if (GetNetDriver() != nullptr)
		{
			URTSReplicationGraph * ReplicationGraph = GetNetDriver()->GetReplicationDriver<URTSReplicationGraph>();
			if (ReplicationGraph != nullptr)
			{
				ReplicationGraph->NotifyOfNumTeams(NumTeams, this);
			}
		}
	}
}

const TArray<AResourceSpot*>& ARTSGameState::GetAllResourceSpots() const
{
	return ResourceSpotsArray;
}

const TArray<AResourceSpot*>& ARTSGameState::GetResourceSpots(EResourceType ResourceType) const
{
	return ResourceSpots[ResourceType].GetArray();
}

void ARTSGameState::AddToResourceSpots(EResourceType ResourceType, AResourceSpot * Spot)
{
	assert(Spot != nullptr);

	ResourceSpots[ResourceType].AddResourceSpot(Spot);
	ResourceSpotsArray.Emplace(Spot);
}

void ARTSGameState::Server_RegisterNeutralSelectable(AActor * Selectable)
{
	assert(HasAuthority());
	assert(Selectable != nullptr);

	for (uint8 i = 0; i < NumTeams; ++i)
	{
		FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
		TeamVisibilityInfo.AddToMap(Selectable);
	}

	NeutralSelectables.Emplace(Selectable);
}

void ARTSGameState::RegisterFogProjectile(AProjectileBase * Projectile)
{
	assert(Projectile != nullptr);

	TemporaryFogProjectiles.Emplace(Projectile);
}

void ARTSGameState::UnregisterFogProjectile(AProjectileBase * Projectile)
{
	/* If using multithreaded fog manager this may need a lock */

	assert(Projectile != nullptr);

	const int32 Index = TemporaryFogProjectiles.Find(Projectile);
	TemporaryFogProjectiles.RemoveAtSwap(Index, 1, false);
}

void ARTSGameState::RegisterFogParticles(UParticleSystemComponent * Particles)
{
	assert(Particles != nullptr);

	TemporaryFogParticles.Emplace(Particles);
}

void ARTSGameState::Multicast_OnSelectableLevelUp_Implementation(uint8 SelectableID, uint8 SelectablesOwnerID, 
	uint8 NewRank)
{
	/* Already did stuff on server */
	if (GetWorld()->IsServer())
	{
		return;
	}

	/* What about the issue where their ID hasn't repped and therefore they have not setup and 
	we can't identify who they are. Maybe need similar logic to abilities in that we add to a 
	queue. In fact it would need to be the same queue */

	AActor * Selectable = GetPlayerFromID(SelectablesOwnerID)->GetSelectableFromID(SelectableID);
	CastChecked<ISelectable>(Selectable)->Client_OnLevelGained(NewRank);
}

void ARTSGameState::Multicast_OnUpgradeComplete_Implementation(uint8 PlayerID, EUpgradeType UpgradeType)
{
	AUpgradeManager * UpgradeManager = GetPlayerFromID(PlayerID)->GetUpgradeManager();
	UpgradeManager->OnUpgradeComplete(UpgradeType);
}

AAbilityBase * ARTSGameState::GetAbilityEffectActor(EContextButton AbilityType) const
{
	/* Many more types that probably will not have effect actors but just checking "None" 
	for now */
	assert(AbilityType != EContextButton::None);
	
	UE_CLOG(!ContextActionEffects.Contains(AbilityType), RTSLOG, Fatal, TEXT("Effect class not "
		"set for ability type \"%s\". Make sure in your game instance blueprint under ContextActions "
		"that the \"Effect\" variable is set for this ability"),
		TO_STRING(EContextButton, AbilityType));

	return ContextActionEffects[AbilityType];
}

void ARTSGameState::AddToTeam(ARTSPlayerState * NewMember, ETeam Team)
{
	assert(NewMember != nullptr);
	assert(Team != ETeam::Neutral);

	if (Team == ETeam::Observer)
	{
		Observers.Emplace(NewMember);
	}
	else
	{
		const int32 Index = Statics::TeamToArrayIndex(Team);

		const TArray <ARTSPlayerState *> & CurrentTeamMembers = Teams[Index].GetPlayerStates();

		/* Recently changed from if statement to assert */
		assert(CurrentTeamMembers.Contains(NewMember) == false);
		Teams[Index].AddToArray(NewMember);

		// Add to global list of ARTSPlayerStates
		assert(PlayerStates.Contains(NewMember) == false);
		PlayerStates.Emplace(NewMember);

		// Add entry in ID to player TMap
		assert(PlayerIDMap.Contains(NewMember->GetPlayerIDAsInt()) == false);
		PlayerIDMap.Emplace(NewMember->GetPlayerIDAsInt(), NewMember);

		/* We called this function during match setup right? */
		assert(UndefeatedPlayers.Contains(NewMember) == false);
		UndefeatedPlayers.Emplace(NewMember);
	}
}

void ARTSGameState::AddCPUController(ACPUPlayerAIController * CPUController)
{
	assert(CPUController != nullptr);
	assert(!CPUPlayers.Contains(CPUController));

	CPUPlayers.Emplace(CPUController);
}

void ARTSGameState::SetupCollisionChannels()
{
	EnemyTraceChannels.Init(FUint64Array(), Teams.Num());
	EnemyQueryParams.Init(FCollisionObjectQueryParams(), Teams.Num());

	for (uint8 i = 0; i < Teams.Num(); ++i)
	{
		for (uint8 j = 0; j < Teams.Num(); ++j)
		{
			/* Avoid adding our own team */
			if (i != j)
			{
				const ETeam EnemyTeam = Statics::ArrayIndexToTeam(j);
				const ECollisionChannel EnemyChannel = GetTeamCollisionChannel(EnemyTeam);

				EnemyTraceChannels[i].AddToArray(EnemyChannel);
				EnemyQueryParams[i].AddObjectTypesToQuery(EnemyChannel);
			}
		}
	}
}

uint8 ARTSGameState::GetGameTickCounter() const
{
	return TickCounter;
}

AObjectPoolingManager * ARTSGameState::GetObjectPoolingManager() const
{
	assert(PoolingManager != nullptr);
	return PoolingManager;
}

void ARTSGameState::OnBuildingPlaced(ABuilding * Building, ETeam Team, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	
	if (bIsServer)
	{
		/* Update visibility info. Add Selectable to every map that is not for Team */
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			if (i != Statics::TeamToArrayIndex(Team))
			{
				FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
				TeamVisibilityInfo.AddToMap(Building);
			}
		}
	}
	else
	{
		// Nothing that needs to be done
	}
}

void ARTSGameState::OnBuildingConstructionCompleted(ABuilding * Building, ETeam Team, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	
	if (bIsServer)
	{
		/* Register as a selectable resource regener if it is one */
		if (Building->GetAttributes()->HasASelectableResourceThatRegens())
		{
			Server_SelectableResourceUsersThatRegen.Emplace(Building);
		}
	}
	else
	{
		/* Register as a selectable resource regener if it is one */
		if (Building->GetAttributes()->HasASelectableResourceThatRegens())
		{
			/* Apply a modifier to that they will be in sync the next time TickCounter reps. 
			So Difference == -1 means this selectable is behind the game's current tick counter 
			by 1, so when the next OnRep_TickCounter comes in we will regen 1 more tick's worth 
			of the resource */
			int32 Difference = (int32)Building->GetAppliedGameTickCount() - (int32)TickCounter;
			/* Basically here we are adjusting for the fact that TickCounter overflows a lot 
			and that 255 is not a usable number. Doing this cuts in half how long we can wait 
			for tick counter reps to happen before we mess up mana regen and the client's values 
			are forever out of sync */
			if (Difference > INT8_MAX) // 127 is what we want, not 255
			{
				Difference -= UINT8_MAX;
			}
			else if (Difference < INT8_MIN)
			{
				Difference += UINT8_MAX;
			}
			Building->GetAttributesModifiable()->SetNumCustomGameTicksAhead(Difference);

			Client_SelectableResourceUsersThatRegen.Emplace(Building);
		}
	}
}

void ARTSGameState::OnInfantryBuilt(AInfantry * Infantry, ETeam Team, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	
	if (bIsServer)
	{
		/* Update visibility info. Add Selectable to every map that is not for Team */
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			if (i != Statics::TeamToArrayIndex(Team))
			{
				FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
				TeamVisibilityInfo.AddToMap(Infantry);
			}
		}

		/* Register as a selectable resource regener if it is one */
		if (Infantry->GetAttributes()->HasASelectableResourceThatRegens())
		{
			Server_SelectableResourceUsersThatRegen.Emplace(Infantry);
		}
	}
	else
	{
		/* Register as a selectable resource regener if it is one */
		if (Infantry->GetAttributes()->HasASelectableResourceThatRegens())
		{
			int32 Difference = (int32)Infantry->GetAppliedGameTickCount() - (int32)TickCounter;
			if (Difference > INT8_MAX)
			{
				Difference -= UINT8_MAX;
			}
			else if (Difference < INT8_MIN)
			{
				Difference += UINT8_MAX;
			}
			Infantry->GetAttributesModifiable()->SetNumCustomGameTicksAhead(Difference);

			Client_SelectableResourceUsersThatRegen.Emplace(Infantry);
		}
	}
}

void ARTSGameState::OnBuildingZeroHealth(ABuilding * Building, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	
	if (bIsServer)
	{
		if (Building->GetAttributes()->HasASelectableResourceThatRegens())
		{
			/* Unregister it as a selectable resource regener if it is one */
			Server_SelectableResourceUsersThatRegen.RemoveSingleSwap(Building, false);
		}
	}
	else
	{
		if (Building->GetAttributes()->HasASelectableResourceThatRegens())
		{
			/* Unregister it as a selectable resource regener if it is one */
			int32 NumRemoved = Client_SelectableResourceUsersThatRegen.RemoveSingleSwap(Building, false);

			// Assert that we found it - it should be there
			assert(NumRemoved == 1);
		}
	}
}

void ARTSGameState::OnInfantryZeroHealth(AInfantry * Infantry, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	
	if (bIsServer)
	{
		if (Infantry->GetAttributes()->HasASelectableResourceThatRegens())
		{
			/* Unregister it as a selectable resource regener if it is one */
			Server_SelectableResourceUsersThatRegen.RemoveSingleSwap(Infantry, false);
		}

		const FVector Loc = Infantry->GetActorLocation();

		// Drop inventory items that want to be dropped.
		const uint32 NumItemsDropped = Infantry->GetInventoryModifiable()->OnOwnerZeroHealth(GetWorld(), 
			GI, this, Infantry, Loc, Infantry->GetActorRotation().Yaw);

		if (NumItemsDropped > 0)
		{
			/* Multicast death event to all clients */
			Multicast_OnSelectableZeroHealth(Infantry, Loc.X, Loc.Y, Loc.Z, Infantry->GetActorRotation().Yaw);
		}
	}
	else
	{
		if (Infantry->GetAttributes()->HasASelectableResourceThatRegens())
		{
			/* Unregister it as a selectable resource regener if it is one */
			int32 NumRemoved = Client_SelectableResourceUsersThatRegen.RemoveSingleSwap(Infantry, false);

			// Assert that we found it - it should be there
			assert(NumRemoved == 1);
		}
	}
}

void ARTSGameState::Server_RegisterSelectableResourceRegener(ISelectable * Selectable)
{
	assert(Server_SelectableResourceUsersThatRegen.Contains(Selectable) == false);
	Server_SelectableResourceUsersThatRegen.Emplace(Selectable);
}

void ARTSGameState::Client_RegisterSelectableResourceRegener(AActor * Selectable)
{
	assert(Client_SelectableResourceUsersThatRegen.Contains(Selectable) == false);
	Client_SelectableResourceUsersThatRegen.Emplace(Selectable);
}

void ARTSGameState::Server_UnregisterSelectableResourceRegener(ISelectable * Selectable)
{
	const int32 NumRemoved = Server_SelectableResourceUsersThatRegen.RemoveSingleSwap(Selectable);
	assert(NumRemoved == 1);
}

void ARTSGameState::Client_UnregisterSelectableResourceRegener(AActor * Selectable)
{
	const int32 NumRemoved = Client_SelectableResourceUsersThatRegen.RemoveSingleSwap(Selectable);
	assert(NumRemoved == 1);
}

void ARTSGameState::OnSelectableDestroyed(AActor * Selectable, ETeam Team, bool bIsServer)
{
	assert((bIsServer && HasAuthority()) || (!bIsServer && !HasAuthority()));
	assert(Selectable != nullptr);
	
	if (bIsServer)
	{
		/* Remove Selectable from every map that is not for Team */
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			if (i != Statics::TeamToArrayIndex(Team))
			{
				FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
				TeamVisibilityInfo.RemoveFromMap(Selectable);
			}
		}
	}
	else
	{
		// Nothing that needs to be done
	}
}

TMap<ECommanderAbility, UCommanderAbilityBase*>& ARTSGameState::GetAllCommanderAbilityEffectObjects()
{
	return CommanderAbilityEffects;
}

UCommanderAbilityBase * ARTSGameState::GetCommanderAbilityEffectObject(ECommanderAbility AbilityType) const
{
	assert(AbilityType != ECommanderAbility::None);
	
	return CommanderAbilityEffects[AbilityType];
}

const TArray<AInventoryItem*>& ARTSGameState::GetInventoryItemsInWorld() const
{
	return InventoryItemsInWorldArray;
}

FInventoryItemID ARTSGameState::GenerateInventoryItemUniqueID(AInventoryItem * InventoryItemActor)
{
	assert(Statics::IsValid(InventoryItemActor));
	
	/* We will have problems using this method if a ID is reused and the previous item with
	that ID still exists in the world. A better way would be used a linked list but that takes
	up more memory */
	const FInventoryItemID ID = CurrentInventoryItemUniqueID++;
	
	InventoryItemsInWorldTMap.Emplace(ID, InventoryItemActor);

	return ID;
}

void ARTSGameState::PutInventoryItemActorInArray(AInventoryItem * InventoryItemActor)
{
	assert(InventoryItemsInWorldArray.Contains(InventoryItemActor) == false);
	
	InventoryItemsInWorldArray.Emplace(InventoryItemActor);

	// Add to fog calculations
	if (HasAuthority())
	{
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
			TeamVisibilityInfo.AddToMap(InventoryItemActor);
		}
	}
	else
	{
		// Client. May want to update TeamVisibilityInfo but only for our own team
	}
}

AInventoryItem * ARTSGameState::GetInventoryItemFromID(FInventoryItemID ItemID) const
{
	/* Clients (and server) don't need to do Contains checks because all item stuff happens through 
	GS reliable RPCs */
	return InventoryItemsInWorldTMap[ItemID];
}

EGameWarning ARTSGameState::Server_TryPutItemInInventory(ISelectable * SelectableGettingItem, 
	bool bIsInventoryOwnerSelected, bool bIsInventoryOwnerCurrentSelected, ARTSPlayerState * SelectablesOwner, 
	const FItemOnDisplayInShopSlot & ShopSlot, uint8 ShopSlotIndex, const FInventoryItemInfo & ItemsInfo, 
	URTSHUD * HUDWidget, ISelectable * ShopPurchasingFrom)
{
	SERVER_CHECK;

	FInventory * Inventory = SelectableGettingItem->GetInventoryModifiable();

	/* Try add item to inventory */
	const FInventory::TryAddItemResult Result = Inventory->TryPutItemInInventory(SelectableGettingItem,
		bIsInventoryOwnerSelected, bIsInventoryOwnerCurrentSelected, ShopSlot.GetItemType(), 
		ShopSlot.GetQuantityBoughtPerPurchase(), ItemsInfo, EItemAquireReason::PurchasedFromShop, 
		GI, HUDWidget);

	if (Result.Result == EGameWarning::None)
	{
		//========================================================
		//	Successfully added item to inventory

		const EInventoryItem LastItemAdded = Result.LastItemAdded;

		/* Play sound/particles for the last item that was created because of combinations.
		If we didn't create any combination items then this will just be the item we just added */

		const FInventoryItemInfo * LastItemsInfo = GI->GetInventoryItemInfo(LastItemAdded);

		// Possibly play a sound
		if (ShouldPlayInventoryItemAquireSound(SelectableGettingItem, LastItemAdded, *LastItemsInfo))
		{
			GI->PlayEffectSound(LastItemsInfo->GetAquireSound());
		}

		// Possibly show particles
		if (ShouldShowInventoryItemAquireParticles(SelectableGettingItem, LastItemAdded, *LastItemsInfo))
		{
			PlayItemAquireParticles(SelectableGettingItem, LastItemsInfo->GetAquireParticles());
		}

		/* This function will need to put the item in inventory just like the other 2
		multicast functions right above and below this and it will need to call
		ISelectable::OnItemPurchasedFromHere on the shop to update HUD. */
		Multicast_PutItemInInventoryFromShopPurchase(SelectableGettingItem, 
			FSelectableIdentifier(ShopPurchasingFrom), ShopSlotIndex);
	}
	else
	{
		//========================================================
		//	Could not add item to inventory. Warnings are handled outside this function usually...
		//	or are they?
	}

	return Result.Result;
}

EGameWarning ARTSGameState::Server_TryPutItemInInventory(ISelectable * SelectableGettingItem,
	bool bIsInventoryOwnerSelected, bool bIsInventoryOwnerCurrentSelected, 
	ARTSPlayerState * SelectablesOwner, EInventoryItem ItemType, uint8 Quantity, 
	const FInventoryItemInfo & ItemsInfo, EItemAquireReason ReasonForAquiringItem, 
	URTSHUD * HUDWidget, AInventoryItem * ItemOnGround)
{
	SERVER_CHECK;
	
	/* Call overloaded version instead if this throws */
	assert(ReasonForAquiringItem != EItemAquireReason::PurchasedFromShop);

	FInventory * Inventory = SelectableGettingItem->GetInventoryModifiable();

	/* Try add item to inventory */
	const FInventory::TryAddItemResult Result = Inventory->TryPutItemInInventory(SelectableGettingItem, 
		bIsInventoryOwnerSelected, bIsInventoryOwnerCurrentSelected, ItemType, Quantity, ItemsInfo, 
		ReasonForAquiringItem, GI, HUDWidget);
	
	if (Result.Result == EGameWarning::None)
	{
		//========================================================
		//	Successfully added item to inventory

		const EInventoryItem LastItemAdded = Result.LastItemAdded;

		/* Play sound/particles for the last item that was created because of combinations.
		If we didn't create any combination items then this will just be the item we just added */

		const FInventoryItemInfo * LastItemsInfo = GI->GetInventoryItemInfo(LastItemAdded);

		// Possibly play a sound
		if (ShouldPlayInventoryItemAquireSound(SelectableGettingItem, LastItemAdded, *LastItemsInfo))
		{
			GI->PlayEffectSound(LastItemsInfo->GetAquireSound());
		}

		// Possibly show particles
		if (ShouldShowInventoryItemAquireParticles(SelectableGettingItem, LastItemAdded, *LastItemsInfo))
		{
			PlayItemAquireParticles(SelectableGettingItem, LastItemsInfo->GetAquireParticles());
		}
		
		if (ReasonForAquiringItem == EItemAquireReason::PickedUpOffGround)
		{
			// Remove the item from the ground
			ItemOnGround->OnPickedUp(SelectableGettingItem, this, SelectableGettingItem->GetLocalPC());

			// Tell clients what happened
			Multicast_PutItemInInventoryFromGround(SelectableGettingItem, ItemOnGround->GetUniqueID());
		}
		else
		{
			// Tell clients what happened
			Multicast_PutItemInInventory(SelectableGettingItem, ItemType, Quantity, ReasonForAquiringItem);
		}
	}
	else
	{
		//========================================================
		//	Could not add item to inventory. Warnings are handled outside this function usually...
		//	or are they?
	}

	return Result.Result;
}

bool ARTSGameState::PutItemInInventoryFromShopPurchase_PassedParamChecks(AActor * ItemPurchaser, AActor * Shop)
{
	// Just returns true for now TODO
	return true;
}

void ARTSGameState::Multicast_PutItemInInventoryFromShopPurchase_Implementation(FSelectableIdentifier SelectableGettingItem,
	FSelectableIdentifier ShopThatSoldItem, uint8 ShopSlotIndex)
{
	// Already did stuff on server
	if (HasAuthority())
	{
		return;
	}

	AActor * SelectableReceivingItem = SelectableGettingItem.GetSelectable(this);
	AActor * Shop = ShopThatSoldItem.GetSelectable(this);

	if (PutItemInInventoryFromShopPurchase_PassedParamChecks(SelectableReceivingItem, Shop))
	{
		ISelectable * ReceiverAsSelectable = CastChecked<ISelectable>(SelectableReceivingItem);
		FInventory * Inventory = ReceiverAsSelectable->GetInventoryModifiable();
		ISelectable * ShopAsSelectable = CastChecked<ISelectable>(Shop);
		const FShopInfo * ShopInfo = ShopAsSelectable->GetShopAttributes();
		const FItemOnDisplayInShopSlot & SlotPurchasedFrom = ShopInfo->GetSlotInfo(ShopSlotIndex);

		const FInventory::TryAddItemResult Result = Inventory->TryPutItemInInventory(ReceiverAsSelectable,
			ReceiverAsSelectable->GetAttributesBase().IsSelected(), ReceiverAsSelectable->GetAttributesBase().IsPrimarySelected(),
			SlotPurchasedFrom.GetItemType(), SlotPurchasedFrom.GetQuantityBoughtPerPurchase(), 
			*GI->GetInventoryItemInfo(SlotPurchasedFrom.GetItemType()), EItemAquireReason::PurchasedFromShop, 
			GI, ReceiverAsSelectable->GetLocalPC()->GetHUDWidget());

		/* This should not fail - server already checked it was ok to do */
		assert(Result.Result == EGameWarning::None);

		const EInventoryItem LastItemAdded = Result.LastItemAdded;
		const FInventoryItemInfo * LastItemsInfo = GI->GetInventoryItemInfo(LastItemAdded);

		// Possibly play a sound
		if (ShouldPlayInventoryItemAquireSound(ReceiverAsSelectable, LastItemAdded, *LastItemsInfo))
		{
			// Likely want to obey fog when playing this sound
			GI->PlayEffectSound(LastItemsInfo->GetAquireSound());
		}

		// Possibly show some particles
		if (ShouldShowInventoryItemAquireParticles(ReceiverAsSelectable, LastItemAdded, *LastItemsInfo))
		{
			PlayItemAquireParticles(ReceiverAsSelectable, LastItemsInfo->GetAquireParticles());
		}

		ShopAsSelectable->OnItemPurchasedFromHere(ShopSlotIndex, ReceiverAsSelectable, true);
	}
}

void ARTSGameState::Multicast_PutItemInInventoryFromGround_Implementation(FSelectableIdentifier SelectableGettingItem, 
	FInventoryItemID ItemID)
{
	// Already did stuff on server
	if (HasAuthority())
	{
		return;
	}

	AActor * Selectable = SelectableGettingItem.GetSelectable(this);
	AInventoryItem * ItemOnGround = GetInventoryItemFromID(ItemID);

	/* Here there is no need to check if ItemOnGround is null and valid because all item stuff 
	is done through reliable GS RPCs */
	if (Selectable != nullptr)
	{
		if (Selectable->IsPendingKill() == false)
		{
			ISelectable * AsSelectable = CastChecked<ISelectable>(Selectable);
			
			FInventory * Inventory = AsSelectable->GetInventoryModifiable();

			const FInventory::TryAddItemResult Result = Inventory->TryPutItemInInventory(AsSelectable,
				AsSelectable->GetAttributesBase().IsSelected(), AsSelectable->GetAttributesBase().IsPrimarySelected(),
				ItemOnGround->GetType(), ItemOnGround->GetItemQuantity(), *ItemOnGround->GetItemInfo(),
				EItemAquireReason::PickedUpOffGround, GI, AsSelectable->GetLocalPC()->GetHUDWidget());

			/* This should not fail - server already checked it was ok to do */
			assert(Result.Result == EGameWarning::None);

			const EInventoryItem LastItemAdded = Result.LastItemAdded;
			const FInventoryItemInfo * LastItemsInfo = GI->GetInventoryItemInfo(LastItemAdded);

			// Possibly play a sound
			if (ShouldPlayInventoryItemAquireSound(AsSelectable, LastItemAdded, *LastItemsInfo))
			{
				// Likely want to obey fog when playing this sound
				GI->PlayEffectSound(LastItemsInfo->GetAquireSound());
			}

			// Possibly show some particles
			if (ShouldShowInventoryItemAquireParticles(AsSelectable, LastItemAdded, *LastItemsInfo))
			{
				PlayItemAquireParticles(AsSelectable, LastItemsInfo->GetAquireParticles());
			}

			// Notify the item that it was picked up
			ItemOnGround->OnPickedUp(AsSelectable, this, AsSelectable->GetLocalPC());
		}
		else
		{
			/* Selectable got item after it was destoyed. */
		}
	}
	else
	{
		NoteDownUnexecutedRPC_PutItemInInventoryFromGround(SelectableGettingItem, ItemID);
	}
}

void ARTSGameState::Multicast_PutItemInInventory_Implementation(FSelectableIdentifier SelectableGettingItem, 
	EInventoryItem ItemType, uint8 Quantity, EItemAquireReason ReasonForAquiringItem)
{
	// Already did stuff on server
	if (HasAuthority())
	{
		return;
	}

	// Call Multicast_PutItemInInventoryFromGround instead if this is the case
	assert(ReasonForAquiringItem != EItemAquireReason::PickedUpOffGround);
	// Call Multicast_PutItemInInventoryFromShopPurchase if this is the case
	assert(ReasonForAquiringItem != EItemAquireReason::PurchasedFromShop);

	AActor * Selectable = SelectableGettingItem.GetSelectable(this);

	// Validity check because networking
	if (Selectable != nullptr)
	{
		if (Selectable->IsPendingKill() == false)
		{
			ISelectable * AsSelectable = CastChecked<ISelectable>(Selectable);

			FInventory * Inventory = AsSelectable->GetInventoryModifiable();

			const FInventory::TryAddItemResult Result = Inventory->TryPutItemInInventory(AsSelectable, 
				AsSelectable->GetAttributes()->IsSelected(), AsSelectable->GetLocalPC()->GetCurrentSelected() == Selectable,
				ItemType, Quantity, *GI->GetInventoryItemInfo(ItemType), ReasonForAquiringItem, 
				GI, AsSelectable->GetLocalPC()->GetHUDWidget());

			/* This should not fail - server already checked it was ok to do */
			assert(Result.Result == EGameWarning::None);

			const EInventoryItem LastItemAdded = Result.LastItemAdded;
			const FInventoryItemInfo * LastItemsInfo = GI->GetInventoryItemInfo(LastItemAdded);

			// Possibly play a sound
			if (ShouldPlayInventoryItemAquireSound(AsSelectable, LastItemAdded, *LastItemsInfo))
			{
				// Likely want to obey fog when playing this sound
				GI->PlayEffectSound(LastItemsInfo->GetAquireSound());
			}

			// Possibly show some particles
			if (ShouldShowInventoryItemAquireParticles(AsSelectable, LastItemAdded, *LastItemsInfo))
			{
				PlayItemAquireParticles(AsSelectable, LastItemsInfo->GetAquireParticles());
			}
		}
		else
		{
			/* Selectable got item after it was destoyed. */
		}
	}
	else
	{
		/* Oh crap. This could be a case where the selectable has not called ISelectable::Setup()
		yet. This is hopefully very rare. Need to put this action into a queue or something */
		NoteDownUnexecutedRPC_PutItemInInventory(SelectableGettingItem, ItemType, Quantity, 
			ReasonForAquiringItem);
	}
}

void ARTSGameState::Multicast_OnInventoryItemSold_Implementation(FSelectableIdentifier Seller, 
	uint8 ServerInvSlotIndex)
{
	// Already did stuff on server
	if (HasAuthority())
	{
		return;
	}

	AActor * Tar = Seller.GetSelectable(this);

	if (Tar != nullptr)
	{
		if (Tar->IsPendingKill() == false)
		{
			ISelectable * Selectable = CastChecked<ISelectable>(Tar);
			FInventory * Inventory = Selectable->GetInventoryModifiable();
			const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(Inventory->GetSlotGivenServerIndex(ServerInvSlotIndex).GetItemType());

			Inventory->OnItemSold(Selectable, ServerInvSlotIndex, 1, *ItemsInfo, 
				Selectable->GetLocalPC()->GetHUDWidget());
		}
		else
		{
			// Possibly nothing needed to be done
		}
	}
	else
	{
		NoteDownUnexecutedRPC_OnInventoryItemSold(Seller, ServerInvSlotIndex);
	}
}

void ARTSGameState::PutInventoryItemInObjectPool(AInventoryItem_SM * InventoryItem)
{
	/* Remove from array */
	InventoryItemsInWorldArray.RemoveSingleSwap(InventoryItem, false);
	
	/* Do something with TMap? I think we can just leave it alone */

	PoolingManager->PutInventoryItemInPool(InventoryItem);

	// Remove from fog calculations
	if (HasAuthority())
	{
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
			TeamVisibilityInfo.RemoveFromMap(InventoryItem);
		}
	}
	else
	{
		// Client. May want to update TeamVisibilityInfo but only for our own team
	}
}

void ARTSGameState::PutInventoryItemInObjectPool(AInventoryItem_SK * InventoryItem)
{
	/* Remove from array */
	InventoryItemsInWorldArray.RemoveSingleSwap(InventoryItem, false);

	/* Do something with TMap? I think we can just leave it alone */

	PoolingManager->PutInventoryItemInPool(InventoryItem);

	// Remove from fog calculations
	if (HasAuthority())
	{
		for (uint8 i = 0; i < NumTeams; ++i)
		{
			FVisibilityInfo & TeamVisibilityInfo = VisibilityInfo[i];
			TeamVisibilityInfo.RemoveFromMap(InventoryItem);
		}
	}
	else
	{
		// Client. May want to update TeamVisibilityInfo but only for our own team
	}
}

bool ARTSGameState::ShouldPlayInventoryItemAquireSound(ISelectable * SelectableThatGotItem,
	EInventoryItem ItemAquired, const FInventoryItemInfo & ItemsInfo) const
{
	/* Always play sound if it is not null and selectable is owned */
	return ItemsInfo.GetAquireSound() != nullptr 
		&& SelectableThatGotItem->GetAttributes()->GetAffiliation() == EAffiliation::Owned;
}

bool ARTSGameState::ShouldShowInventoryItemAquireParticles(ISelectable * SelectableThatGotItem, 
	EInventoryItem ItemAquired, const FInventoryItemInfo & ItemsInfo) const
{
	// Always show as long as template is not null
	return ItemsInfo.GetAquireParticles().GetParticles() != nullptr;
}

void ARTSGameState::PlayItemAquireParticles(ISelectable * SelectableThatGotItem, const FParticleInfo_Attach & Particles)
{
	const FAttachInfo * AttachInfo = SelectableThatGotItem->GetBodyLocationInfo(Particles.GetAttachPoint());
	
	/* Just attaching them to root and no specific bone. Could be good idea perhaps to allow a 
	body location in FParticleInfo to allow attaching to a specific bone */
	Statics::SpawnFogParticlesAttached(this, Particles.GetParticles(), AttachInfo->GetComponent(),
		AttachInfo->GetSocketName(), 0.f, AttachInfo->GetAttachTransform().GetLocation(), 
		AttachInfo->GetAttachTransform().GetRotation().Rotator(), 
		AttachInfo->GetAttachTransform().GetScale3D());
}

void ARTSGameState::PutInventoryItemInWorld(EInventoryItem ItemType, int32 ItemStackQuantity,
	int16 ItemNumCharges, const FVector & Location, const FRotator & Rotation, EItemEntersWorldReason SpawnReason)
{
	const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(ItemType);
	AInventoryItem * Item = PoolingManager->PutItemInWorld(ItemType, ItemStackQuantity, 
		ItemNumCharges, *ItemsInfo, Location, Rotation, SpawnReason, FogManager);
}

void ARTSGameState::Multicast_OnSelectableZeroHealth_Implementation(FSelectableIdentifier Selectable, 
	float Location_X, float Location_Y, float Location_Z, float Yaw)
{
	if (GetWorld()->IsServer())
	{
		return; // Already did stuff on server
	}

	AActor * Actor = Selectable.GetSelectable(this);
	if (Actor != nullptr)
	{
		if (Actor->IsPendingKill() == false)
		{
			ISelectable * AsSelectable = CastChecked<ISelectable>(Actor);

			/* No null check whether they have an inventory because the only reason we call this
			func in the first place is because they have an inventory. If I add other reasons in the
			future then will need to null check it */
			AsSelectable->GetInventoryModifiable()->OnOwnerZeroHealth(GetWorld(), GI, this, AsSelectable,
				FVector(Location_X, Location_Y, Location_Z), Yaw);
		}
	}
	else
	{
		/* This kind of doesn't need to be in order with the other RPCs, at least as long as all 
		it does is drop items into the world. It really wants to be a kind of 
		"RPCThatIsGuaranteedToBeSentEventually" which is somewhere in between reliable and unreliable. 
		I guess it's reliable really, but we just don't need the ordering guarantee, and we can 
		live with it arriving latish, so it kind of behaves like a replicated variable then */
		NoteDownUnexecutedRPC_OnSelectableZeroHealth(Selectable, FVector(Location_X, Location_Y, Location_Z), Yaw);
	}
}

void ARTSGameState::NoteDownUnexecutedRPC_Ability()
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);
	
	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_CommanderAbility()
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_BuildingTargetingAbility()
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_PutItemInInventory(FSelectableIdentifier SelectableGettingItem,
	EInventoryItem ItemType, uint8 Quantity, EItemAquireReason ReasonForAquiringItem)
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_PutItemInInventoryFromGround(FSelectableIdentifier SelectableGettingItem, FInventoryItemID ItemID)
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_OnSelectableZeroHealth(FSelectableIdentifier Selectable, const FVector & Location, float Yaw)
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::NoteDownUnexecutedRPC_OnInventoryItemSold(FSelectableIdentifier Selectable, uint8 ServerInvSlotIndex)
{
	CLIENT_CHECK;

	// Assert because it would be good to get an idea how often this happens
	assert(0);

	// TODO implement. Also have not added any variables for this yet
}

void ARTSGameState::SetGI(URTSGameInstance * InGameInstance)
{
	GI = InGameInstance;
}

float ARTSGameState::Server_GetMatchTime() const
{
	assert(GetWorld()->IsServer());
	
	return GetWorld()->GetTimeSeconds() - TimeWhenMatchStarted;
}

void ARTSGameState::SetLocalPlayersTeam(ETeam InLocalPlayersTeam)
{
	LocalPlayersTeam = InLocalPlayersTeam;
}

ETeam ARTSGameState::GetLocalPlayersTeam() const
{
	return LocalPlayersTeam;
}

const TArray<ARTSPlayerController*>& ARTSGameState::GetPlayers() const
{
	return PlayerControllers;
}

const TArray<ARTSPlayerState*>& ARTSGameState::GetPlayerStates() const
{
	return PlayerStates;
}

TArray<ARTSPlayerState*> ARTSGameState::GetPlayerStatesByValue() const
{
	return PlayerStates;
}

const TArray<ACPUPlayerAIController*>& ARTSGameState::GetCPUControllers() const
{
	return CPUPlayers;
}

const TArray<FPlayerStateArray> & ARTSGameState::GetTeams() const
{
	return Teams;
}

const TArray <ARTSPlayerState *> & ARTSGameState::GetTeamPlayerStates(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);

	return Teams[Index].GetPlayerStates();
}

TArray<ARTSPlayerState*>& ARTSGameState::GetUndefeatedPlayers()
{
	return UndefeatedPlayers;
}

ARTSPlayerState * ARTSGameState::GetPlayerFromID(uint8 InRTSPlayerID) const
{
	return PlayerIDMap[InRTSPlayerID];
}

FVisibilityInfo & ARTSGameState::GetTeamVisibilityInfo(ETeam Team)
{
	const int32 Index = Statics::TeamToArrayIndex(Team);

	return VisibilityInfo[Index];
}

const FVisibilityInfo & ARTSGameState::GetTeamVisibilityInfo(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);

	return VisibilityInfo[Index];
}

TArray<FVisibilityInfo>& ARTSGameState::GetAllTeamsVisibilityInfo()
{
	return VisibilityInfo;
}

uint32 ARTSGameState::GetNumTeams() const
{
	return NumTeams;
}

const TArray<AActor*>& ARTSGameState::GetNeutrals() const
{
	return NeutralSelectables;
}

const FName & ARTSGameState::GetTeamTag(ETeam InTeam) const
{
	const int32 Index = Statics::TeamToArrayIndex(InTeam, true);

	return TeamTags[Index];
}

TArray<AProjectileBase*>& ARTSGameState::GetTemporaryFogProjectiles()
{
	return TemporaryFogProjectiles;
}

TArray<UParticleSystemComponent*>& ARTSGameState::GetTemporaryFogParticles()
{
	return TemporaryFogParticles;
}

const TArray<uint64>& ARTSGameState::GetAllTeamCollisionChannels() const
{
	return TeamTraceChannels;
}

const FCollisionObjectQueryParams & ARTSGameState::GetAllTeamsQueryParams() const
{
	return AllTeamsQueryParams;
}

const FCollisionResponseContainer & ARTSGameState::GetAllTeamsCollisionResponseContainer_Overlap() const
{
	return AllTeams_Overlap;
}

ECollisionChannel ARTSGameState::GetTeamCollisionChannel(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);

	return static_cast<ECollisionChannel>(TeamTraceChannels[Index]);
}

const TArray <uint64> & ARTSGameState::GetEnemyChannels(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);

	UE_CLOG(EnemyTraceChannels.IsValidIndex(Index) == false, RTSLOG, Fatal, TEXT("Team not in "
		"array. Team was [%s] which generated array index [%d]"), TO_STRING(ETeam, Team), Index);

	return EnemyTraceChannels[Index].GetArray();
}

const FCollisionObjectQueryParams & ARTSGameState::GetAllEnemiesQueryParams(ETeam Team) const
{
	const int32 Index = Statics::TeamToArrayIndex(Team);
	return EnemyQueryParams[Index];
}

ECollisionChannel ARTSGameState::GetNeutralTeamCollisionChannel() const
{
	return NeutralTraceChannel;
}

#if !MULTITHREADED_FOG_OF_WAR
void ARTSGameState::SetFogManager(AFogOfWarManager * InFogManager)
{
	FogManager = InFogManager;
}

AFogOfWarManager * ARTSGameState::GetFogManager() const
{
	return FogManager;
}
#endif


//==============================================================================================
//	Lobby 
//==============================================================================================

void ARTSGameState::SetupSingleplayerLobby()
{
	/* This clears the arrays here in game state */
	ClearLobby();

	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);

	/* Set default values */
	Lobby->SetLobbyType(EMatchType::Offline);
	LobbyName = LobbyOptions::DEFAULT_SINGLEPLAYER_LOBBY_NAME;
	LobbyStartingResources = LobbyOptions::DEFAULT_STARTING_RESOURCES;
	LobbyDefeatCondition = LobbyOptions::DEFAULT_DEFEAT_CONDITION;
	LobbyMapIndex = LobbyOptions::DEFAULT_MAP_ID;
	Lobby->ClearChat();

	/* Set first slot to open */
	LobbyPlayerTypes[0] = ELobbySlotStatus::Open;

	/* Put default CPU players in slots */
	for (uint32 i = 1; i < LobbyOptions::DEFAULT_NUM_CPU_OPPONENTS + 1; ++i)
	{
		/* Kind of inefficient - calls UpdateServerLobby */
		PopulateLobbySlot(LobbyOptions::DEFAULT_CPU_DIFFICULTY, i);
	}

	/* Set all other slots to closed by default */
	for (uint32 i = LobbyOptions::DEFAULT_NUM_CPU_OPPONENTS + 1; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
	{
		LobbyPlayerTypes[i] = ELobbySlotStatus::Closed;
	}

	/* Put us as the first player */
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	const EFaction StartingFaction = Settings->GetDefaultFaction();
	GetWorld()->GetFirstPlayerController()->PlayerState->SetPlayerName(Settings->GetPlayerAlias());
	PopulateLobbySlot(GetWorld()->GetFirstPlayerController(), StartingFaction);// NB calls UpdateServerLobby
}

void ARTSGameState::SetupNetworkedLobby(const TSharedPtr<FOnlineSessionSettings> SessionSettings)
{
	/* This clears the arrays here in game state */
	ClearLobby();

	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	const EMatchType LobbyType = SessionOptions::IsLANSession(SessionSettings) ? EMatchType::LAN : EMatchType::SteamOnline;

	/* Set network type, name, rules and map we set in lobby creation screen */
	Lobby->SetLobbyType(LobbyType);
	LobbyName = SessionOptions::GetLobbyName(SessionSettings);
	LobbyStartingResources = SessionOptions::GetStartingResources(SessionSettings);
	LobbyDefeatCondition = SessionOptions::GetDefeatCondition(SessionSettings);
	LobbyMapIndex = SessionOptions::GetMapID(SessionSettings);
	Lobby->ClearChat();

	/* Set right amount of slots to open status */
	const int32 NumSlots = SessionOptions::GetNumPublicConnections(SessionSettings);
	for (int32 i = 0; i < NumSlots; ++i)
	{
		LobbyPlayerTypes[i] = ELobbySlotStatus::Open;
	}
	/* Set the remainder to closed */
	for (int32 i = NumSlots; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
	{
		LobbyPlayerTypes[i] = ELobbySlotStatus::Closed;
	}

	/* Put us as the first player */
	URTSGameUserSettings * Settings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());
	const EFaction StartingFaction = Settings->GetDefaultFaction();
	// Setting name now so it appears in lobby but should really be done when PS is created
	GetWorld()->GetFirstPlayerController()->PlayerState->SetPlayerName(Settings->GetPlayerAlias());
	PopulateLobbySlot(GetWorld()->GetFirstPlayerController(), StartingFaction);	// NB calls UpdateServerLobby
}

int32 ARTSGameState::OnClientJoinsLobby(APlayerController * NewPlayerController, EFaction DefaultFaction)
{
	assert(GetWorld()->IsServer());

	return PopulateLobbySlot(NewPlayerController, DefaultFaction);

	// TODO make other slots uninteractable
}

int32 ARTSGameState::GetNextOpenLobbySlot() const
{
	/* Find next free slot in lobby */
	int32 FreeIndex = -1;
	for (int32 i = 0; i < LobbyPlayers.Num(); ++i)
	{
		if (LobbyPlayerTypes[i] == ELobbySlotStatus::Open)
		{
			assert(!Statics::IsValid(LobbyPlayers[i]));

			FreeIndex = i;
			break;
		}
	}

	/* Assumed open slot was there. If lobby is full then players should not have been allowed
	to join in the first place */
	assert(FreeIndex != -1);

	return FreeIndex;
}

void ARTSGameState::ClearLobby()
{
	LobbyPlayers.Init(nullptr, ProjectSettings::MAX_NUM_PLAYERS);
	LobbyPlayerTypes.Init(ELobbySlotStatus::Closed, ProjectSettings::MAX_NUM_PLAYERS);
	LobbyCPUDifficulties.Init(LobbyOptions::DEFAULT_CPU_DIFFICULTY, ProjectSettings::MAX_NUM_PLAYERS);
	LobbyTeams.Init(LobbyOptions::DEFAULT_TEAM, ProjectSettings::MAX_NUM_PLAYERS);
	LobbyFactions.Init(EFaction::Humans, ProjectSettings::MAX_NUM_PLAYERS);
	LobbyPlayerStarts.Init(-1, ProjectSettings::MAX_NUM_PLAYERS);
}

int32 ARTSGameState::PopulateLobbySlot(APlayerController * Player, EFaction StartingFaction)
{
	/* TODO need to make sure that player name is set on the player state. This should be
	parsed from the join string that contained the password. Also
	APlayerState::OnRep_PlayerName() may need to be used to update lobby visuals */

	assert(GetWorld()->IsServer());
	assert(StartingFaction != EFaction::None);

	const int32 SlotIndex = GetNextOpenLobbySlot();
	if (SlotIndex == -1)
	{
		return SlotIndex;
	}

	LobbyPlayers[SlotIndex] = CastChecked<ARTSPlayerState>(Player->PlayerState);
	LobbyPlayerTypes[SlotIndex] = ELobbySlotStatus::Human;
	LobbyCPUDifficulties[SlotIndex] = LobbyOptions::DEFAULT_CPU_DIFFICULTY;
	LobbyTeams[SlotIndex] = LobbyOptions::DEFAULT_TEAM;
	LobbyFactions[SlotIndex] = StartingFaction;

	UpdateServerLobby();

	return SlotIndex;
}

void ARTSGameState::PopulateLobbySlot(ECPUDifficulty CPUDifficulty, int32 SlotIndex)
{
	assert(GetWorld()->IsServer());

	LobbyPlayers[SlotIndex] = nullptr;
	LobbyPlayerTypes[SlotIndex] = ELobbySlotStatus::CPU;
	LobbyCPUDifficulties[SlotIndex] = CPUDifficulty;
	LobbyTeams[SlotIndex] = LobbyOptions::DEFAULT_TEAM;
	LobbyFactions[SlotIndex] = GI->GetRandomFaction();

	UpdateServerLobby();
}

void ARTSGameState::RemoveFromLobby(int32 SlotIndex, ELobbySlotStatus NewSlotStatus)
{
	assert(GetWorld()->IsServer());
	assert(NewSlotStatus == ELobbySlotStatus::Open || NewSlotStatus == ELobbySlotStatus::Closed);

	LobbyPlayers[SlotIndex] = nullptr;
	LobbyPlayerTypes[SlotIndex] = NewSlotStatus;

	UpdateServerLobby();
}

void ARTSGameState::UpdateServerLobby()
{
	OnRep_LobbyName();
	OnRep_LobbyPlayers();
	OnRep_LobbyPlayerTypes();
	OnRep_LobbyCPUDifficulties();
	OnRep_LobbyTeams();
	OnRep_LobbyFactions();
	OnRep_LobbyPlayerStarts();
	OnRep_LobbyStartingResources();
	OnRep_LobbyDefeatCondition();
	OnRep_LobbyMap();
	OnRep_LobbyAreSlotsLocked();
}

void ARTSGameState::OnRep_LobbyName()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	Lobby->SetLobbyName(FText::FromString(LobbyName));

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyPlayers()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	for (uint8 i = 0; i < LobbyPlayers.Num(); ++i)
	{
		ULobbySlot * const LobbySlot = Lobby->GetSlot(i);
		LobbySlot->SetPlayerState(LobbyPlayers[i], true);
	}

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyPlayerTypes()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	for (uint8 i = 0; i < LobbyPlayerTypes.Num(); ++i)
	{
		ULobbySlot * const LobbySlot = Lobby->GetSlot(i);
		LobbySlot->SetStatus(LobbyPlayerTypes[i], true);
	}

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyCPUDifficulties()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	for (uint8 i = 0; i < LobbyCPUDifficulties.Num(); ++i)
	{
		ULobbySlot * const LobbySlot = Lobby->GetSlot(i);
		LobbySlot->SetCPUDifficulty(LobbyCPUDifficulties[i]);
	}

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyTeams()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	for (uint8 i = 0; i < LobbyTeams.Num(); ++i)
	{
		ULobbySlot * const LobbySlot = Lobby->GetSlot(i);
		LobbySlot->SetTeam(LobbyTeams[i]);
	}

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyFactions()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	for (uint8 i = 0; i < LobbyFactions.Num(); ++i)
	{
		ULobbySlot * LobbySlot = Lobby->GetSlot(i);
		LobbySlot->SetFaction(LobbyFactions[i]);
	}

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyPlayerStarts()
{
	ULobbyWidget * Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
	Lobby->UpdatePlayerStartAssignments(LobbyPlayerStarts);
}

void ARTSGameState::OnRep_LobbyStartingResources()
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));

	// Avoid doing costly ULobbyWidget::UpdateVisibilities here since we know what to update
	Lobby->SetStartingResources(LobbyStartingResources);
}

void ARTSGameState::OnRep_LobbyDefeatCondition()
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));

	// Avoid doing costly ULobbyWidget::UpdateVisibilities here since we know what to update
	Lobby->SetDefeatCondition(LobbyDefeatCondition);
}

void ARTSGameState::OnRep_LobbyMap()
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
	Lobby->SetMap(GI->GetMapInfo(LobbyMapIndex));

	Lobby->UpdateVisibilities();
}

void ARTSGameState::OnRep_LobbyAreSlotsLocked()
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));

	// Avoid doing costly ULobbyWidget::UpdateVisibilities here since we know what to update
	Lobby->SetAreSlotsLocked(bLobbyAreSlotsLocked);
}

bool ARTSGameState::IsInMatch() const
{
	return bIsInMatch;
}

bool ARTSGameState::AreLobbySlotsLocked() const
{
	return bLobbyAreSlotsLocked;
}

void ARTSGameState::ChangeTeamInLobby(ARTSPlayerController * Player, ETeam NewTeam)
{
	assert(GetWorld()->IsServer());

	if (Statics::IsValid(Player))
	{
		ULobbyWidget * const Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
		
		/* Figure out which lobby slot player is in. Can ditch this for loop in favor for using the 
		lobby slot index assigned during GM::PostLogin instead */
		for (uint8 i = 0; i < Lobby->GetSlots().Num(); ++i)
		{
			ULobbySlot * const Slot = Lobby->GetSlot(i);
			if (Slot->GetPlayerState() == Player->PlayerState)
			{
				// Found slot of player 

				if (AreLobbySlotsLocked())
				{
					/* A situation where OnRep did not get there in time to make combo box
					uninteractable (or modified client). Client will still have their new value
					selected in their combo box so need to tell them to change it back. Also
					another team OnRep update happening will change it back but there's no
					guarantee another change will happen by the time the match starts so need 
					to call this RPC */

					Player->Client_OnChangeTeamInLobbyFailed(LobbyTeams[i]);
				}
				else
				{
					/* Need this RPC basically because without it it would be possible for a
					client to switch their team but have it rejected because slots are locked,
					then try and change their team again, be successful and have value rep to
					them but then RPC for lock slot change attempt comes in and changes team
					back to what it started at before this whole sequence of events. */
					Player->Client_OnChangeTeamInLobbySuccess(NewTeam); // Recently changed param from LobbyTeams[i]

					LobbyTeams[i] = NewTeam;

					OnRep_LobbyTeams();
				}

				return;
			}
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Player %s requested to change team in lobby but weren't "
			"present in lobby"), *Player->GetName());
	}
}

void ARTSGameState::ChangeTeamInLobby(uint8 SlotIndex, ETeam NewTeam)
{
	assert(GetWorld()->IsServer());

	LobbyTeams[SlotIndex] = NewTeam;

	OnRep_LobbyTeams();
}

void ARTSGameState::ChangeFactionInLobby(ARTSPlayerController * Player, EFaction NewFaction)
{
	assert(GetWorld()->IsServer());

	if (Statics::IsValid(Player))
	{
		ULobbyWidget * const Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
		for (uint8 i = 0; i < Lobby->GetSlots().Num(); ++i)
		{
			ULobbySlot * const Slot = Lobby->GetSlot(i);
			if (Slot->GetPlayerState() == Player->PlayerState)
			{
				// Found slot of player 

				if (AreLobbySlotsLocked())
				{
					Player->Client_OnChangeFactionInLobbyFailed(LobbyFactions[i]);
				}
				else
				{
					Player->Client_OnChangeFactionInLobbySuccess(NewFaction);

					LobbyFactions[i] = NewFaction;

					OnRep_LobbyFactions();
				}

				return;
			}
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Player state not present in lobby, player controller name: %s"), 
			*Player->GetName());
	}
}

void ARTSGameState::ChangeFactionInLobby(uint8 SlotIndex, EFaction NewFaction)
{
	assert(GetWorld()->IsServer());

	LobbyFactions[SlotIndex] = NewFaction;

	OnRep_LobbyFactions();
}

void ARTSGameState::ChangeStartingSpotInLobby(ARTSPlayerController * Player, int16 StartingSpotID)
{
	assert(GetWorld()->IsServer());

	if (Statics::IsValid(Player))
	{
		ULobbyWidget * const Lobby = GI->GetWidget<ULobbyWidget>(EWidgetType::Lobby);
		for (uint8 i = 0; i < Lobby->GetSlots().Num(); ++i)
		{
			ULobbySlot * const Slot = Lobby->GetSlot(i);

			if (Slot->GetPlayerState() == Player->PlayerState)
			{
				// Found slot of player 

				if (AreLobbySlotsLocked())
				{
					// Do not allow change if lobby slots are locked
					Player->Client_OnChangeStartingSpotInLobbyFailed(LobbyPlayerStarts[i]);
				}
				else
				{
					Player->Client_OnChangeStartingSpotInLobbySuccess(StartingSpotID);

					LobbyPlayerStarts[i] = StartingSpotID;

					OnRep_LobbyPlayerStarts();
				}
			}

			return;
		}

		UE_LOG(RTSLOG, Fatal, TEXT("Player %s requested to change their starting spot in lobby "
			"but apparently they are not in lobby"), *Player->GetName());
	}
}

void ARTSGameState::ChangeCPUDifficultyInLobby(uint8 SlotIndex, ECPUDifficulty NewDifficulty)
{
	assert(GetWorld()->IsServer());

	LobbyCPUDifficulties[SlotIndex] = NewDifficulty;

	OnRep_LobbyCPUDifficulties();
}

void ARTSGameState::ChangeStartingResourcesInLobby(EStartingResourceAmount NewAmount)
{
	assert(GetWorld()->IsServer());

	LobbyStartingResources = NewAmount;

	OnRep_LobbyStartingResources();
}

void ARTSGameState::ChangeDefeatConditionInLobby(EDefeatCondition NewCondition)
{
	assert(GetWorld()->IsServer());

	LobbyDefeatCondition = NewCondition;

	OnRep_LobbyDefeatCondition();
}

void ARTSGameState::ChangeMapInLobby(const FMapInfo & NewMap)
{
	assert(GetWorld()->IsServer());

	LobbyMapIndex = NewMap.GetUniqueID();

	OnRep_LobbyMap();
}

void ARTSGameState::KickPlayerInLobby(uint8 SlotIndex, ARTSPlayerState * Player)
{
	// Can only be initiated from server... so game mode is a better place for it in that regard
	assert(GetWorld()->IsServer());

	if (Player == nullptr)
	{
		// Assuming it is a CPU player

		ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
		ULobbySlot * Slot = Lobby->GetSlot(SlotIndex);
		assert(Slot->HasCPUPlayer());

		LobbyPlayerTypes[SlotIndex] = ELobbySlotStatus::Open;

		UpdateServerLobby();
	}
	else
	{
		// TODO Human player
	}
}

void ARTSGameState::ChangeLockedSlotsStatusInLobby(bool bNewValue)
{
	assert(GetWorld()->IsServer());

	bLobbyAreSlotsLocked = bNewValue;

	OnRep_LobbyAreSlotsLocked();
}

void ARTSGameState::SetTeamFromServer(ARTSPlayerController * PlayerToCorrectFor, ETeam Team)
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
	for (uint8 i = 0; i < Lobby->GetSlots().Num(); ++i)
	{
		ULobbySlot * const Slot = Lobby->GetSlot(i);
		if (Slot->GetPlayerState() == PlayerToCorrectFor->PlayerState)
		{
			LobbyTeams[i] = Team;

			OnRep_LobbyTeams();
		}
	}
}

void ARTSGameState::SetFactionFromServer(ARTSPlayerController * PlayerToCorrectFor, EFaction Faction)
{
	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
	for (uint8 i = 0; i < Lobby->GetSlots().Num(); ++i)
	{
		ULobbySlot * const Slot = Lobby->GetSlot(i);
		if (Slot->GetPlayerState() == PlayerToCorrectFor->PlayerState)
		{
			LobbyFactions[i] = Faction;

			OnRep_LobbyFactions();
		}
	}
}

void ARTSGameState::SetStartingSpotFromServer(ARTSPlayerController * PlayerToCorrectFor, int16 StartingSpotID)
{
	const uint8 PlayersLobbyIndex = PlayerToCorrectFor->GetLobbySlotIndex();
	LobbyPlayerStarts[PlayersLobbyIndex] = StartingSpotID;

	OnRep_LobbyPlayerStarts();
}

bool ARTSGameState::TryOpenLobbySlot()
{
	assert(GetWorld()->IsServer());

	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));

	for (int32 i = 0; i < Lobby->GetSlots().Num(); ++i)
	{
		ULobbySlot * Slot = Lobby->GetSlot(i);
		if (Slot->GetStatus() == ELobbySlotStatus::Closed)
		{
			/* Make sure slot was empty. As long as player input cannot happen in the middle of a
			function call this should never throw */
			assert(!Slot->HasHumanPlayer() && !Slot->HasCPUPlayer());

			LobbyPlayers[i] = nullptr;
			LobbyPlayerTypes[i] = ELobbySlotStatus::Open;
			LobbyCPUDifficulties[i] = LobbyOptions::DEFAULT_CPU_DIFFICULTY;
			LobbyTeams[i] = LobbyOptions::DEFAULT_TEAM;
			LobbyFactions[i] = GI->GetRandomFaction();

			UpdateServerLobby();

			return true;
		}
	}

	return false;
}

void ARTSGameState::CloseLobbySlot(uint8 SlotIndex)
{
	assert(GetWorld()->IsServer());

	ULobbyWidget * Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
	ULobbySlot * Slot = Lobby->GetSlot(SlotIndex);

	/* Make sure slot was empty */
	assert(!Slot->HasHumanPlayer() && !Slot->HasCPUPlayer());

	LobbyPlayers[SlotIndex] = nullptr;
	LobbyPlayerTypes[SlotIndex] = ELobbySlotStatus::Closed;
	//LobbyCPUDifficulties[SlotIndex] = ECPUDifficulty::None;

	UpdateServerLobby();
}

int16 ARTSGameState::GetLobbyPlayerStart(uint8 LobbyIndexOfPlayer) const
{
	return LobbyPlayerStarts[LobbyIndexOfPlayer];
}

void ARTSGameState::Multicast_SendLobbyChatMessage_Implementation(ARTSPlayerState * Sender, const FString & Message)
{
	FString SendersName;

	if (Statics::IsValid(Sender))
	{
		SendersName = Sender->GetPlayerName();
	}
	else
	{
		/* IMO message should still be shown, but use "Unknown" name */
		SendersName = "Unknown";
	}

	ULobbyWidget * const Lobby = CastChecked<ULobbyWidget>(GI->GetWidget(EWidgetType::Lobby));
	Lobby->OnChatMessageReceived(SendersName, Message);
}

FString ARTSGameState::IsMatchInfoCorrect(const FMatchInfo & MatchInfo, const TMap <ETeam, ETeam> & NewTeamMap, 
	bool bCheckPlayerStarts) const
{
#if !UE_BUILD_SHIPPING

	/* Check each players info */
	for (int32 i = 0; i < MatchInfo.GetPlayers().Num(); ++i)
	{
		const FPlayerInfo & PlayerInfo = MatchInfo.GetPlayers()[i];
		if (PlayerInfo.PlayerState != LobbyPlayers[i])
		{
			return TEXT("Player states");
		}
		if (PlayerInfo.PlayerType != LobbyPlayerTypes[i])
		{
			return TEXT("Slot status");
		}
		if (PlayerInfo.CPUDifficulty != LobbyCPUDifficulties[i]
			&& PlayerInfo.PlayerType == ELobbySlotStatus::CPU)
		{
			return TEXT("CPU difficulties");
		}
		if (PlayerInfo.Team != NewTeamMap[LobbyTeams[i]])
		{
			return TEXT("Teams");
		}
		if (PlayerInfo.Faction != LobbyFactions[i])
		{
			return TEXT("Factions");
		}
		if (bCheckPlayerStarts && PlayerInfo.StartingSpotID != LobbyPlayerStarts[i])
		{
			return TEXT("Starting spots");
		}
	}

	if (MatchInfo.GetStartingResources() != LobbyStartingResources)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Mismatch. Match info starting resources: [%s], Game state starting resources: [%s]"),
			TO_STRING(EStartingResourceAmount, MatchInfo.GetStartingResources()),
			TO_STRING(EStartingResourceAmount, LobbyStartingResources));

		return TEXT("Starting resources");
	}

	/* Check defeat condition */
	if (MatchInfo.GetDefeatCondition() != LobbyDefeatCondition)
	{
		return TEXT("Defeat condition");
	}

	/* Check map */
	if (GI->GetMapInfo(MatchInfo.GetMapFName()).GetUniqueID() != LobbyMapIndex)
	{
		return TEXT("Map");
	}

#endif

	return FString();
}

bool ARTSGameState::AreMatchInfoPlayerStartsCorrect(const FMatchInfo & MatchInfo) const
{
	for (int32 i = 0; i < MatchInfo.GetPlayers().Num(); ++i)
	{
		const FPlayerInfo & PlayerInfo = MatchInfo.GetPlayers()[i];
		if (PlayerInfo.StartingSpotID != LobbyPlayerStarts[i])
		{
			return false;
		}
	}

	return true;
}


//----------------------------------------------------
//	Inital Map Setup for Skip Main Menu for Testing
//----------------------------------------------------

int32 ARTSGameState::GetNumPCSetupAcksForPIE() const
{
	return PIE_NumPCSetupCompleteAcks;
}

int32 ARTSGameState::GetNumPSSetupAcksForPIE() const
{
	return PIE_NumPSSetInitialValuesAcks;
}

int32 ARTSGameState::GetNumFinalSetupAcks() const
{
	return NumFinalSetupAcks;
}


//----------------------------------------------------
//	Inital Match Setup
//----------------------------------------------------

uint8 ARTSGameState::GenerateUniquePlayerID()
{
	assert(GetWorld()->IsServer());

	return NextUniquePlayerID++;
}

void ARTSGameState::Multicast_LoadMatchMap_Implementation(const FName & MapName)
{
	/* Remember to enable world composition in the persistent match level and add all maps you
	want to play on as sub-levels to it (unloaded) */

	if (GetWorld()->IsServer())
	{
		SetMatchLoadingStatus(ELoadingStatus::WaitingForAllPlayersToStreamInLevel);
	}

	StreamInMatchLevel(MapName);
}

void ARTSGameState::SetNumPlayersForMatch(int32 InNumPlayers, int32 InNumHumanPlayers)
{
	assert(GetWorld()->IsServer());

	ExpectedNumPlayers = InNumPlayers;
	ExpectedNumHumanPlayers = InNumHumanPlayers;
}

void ARTSGameState::OnRep_MatchLoadingStatus()
{
	ULoadingScreen * LoadingScreen = GI->GetWidget<ULoadingScreen>(EWidgetType::LoadingScreen);
	
	LoadingScreen->SetStatusText(GI->GetMatchLoadingStatusText(MatchLoadingStatus));
}

void ARTSGameState::StreamInMatchLevel(const FName & MapPath)
{
	/* Unstream lobby level */
	const FLatentActionInfo LobbyLatentInfo = FLatentActionInfo(0, 62, TEXT("OnLevelStreamedOut"),
		this);
	UGameplayStatics::UnloadStreamLevel(GetWorld(), MapOptions::LOBBY_MAP_NAME, LobbyLatentInfo,
		/*bShouldBlockOnUnload*/false);

	/* Unstream all levels from map pool in case they are streamed in */
	static int32 ID = 1337;	// Not sure if needed or can use same number every time
	for (const auto & Elem : GI->GetMapPool())
	{
		const FLatentActionInfo Info = FLatentActionInfo(0, ID++, TEXT("OnLevelStreamedOut"), this);
		UGameplayStatics::UnloadStreamLevel(GetWorld(), Elem.Key, Info, /*bShouldBlockOnUnload*/false);
	}

	/* Stream in map to play match on */
	const FLatentActionInfo LatentInfo = FLatentActionInfo(0, ID++,
		*FString("OnMatchLevelStreamedIn"), this);
	UGameplayStatics::LoadStreamLevel(GetWorld(), MapPath, true, false, LatentInfo);

	/* Regardless of whether the map name is a valid map name or not the latent action info
	will call the function */
}

void ARTSGameState::OnLevelStreamedOut()
{
	NumLevelsStreamedOut++;

	CheckIfStreamingComplete();
}

void ARTSGameState::OnMatchLevelStreamedIn()
{
	bHasStreamedInMatchLevel = true;

	/* Destroy all selectables on map (exculding things like resource spots). The reason for 
	doing this is to remove any selectables the developer may have left on their map for 
	testing purposes.  
	Alternative methods that don't incur this runtime cost:
	- Remove them from maps when the game is packaged (have not looked into if this is even 
	possible or not). However I assume the map would then need to be saved. Now after packaging 
	maps will have had all starting selectables removed but if continuing to develop with editor 
	now user has to re-add them. 
	- Create an extra map that doesn't contain the selectables. Duplicating maps may increase 
	executable size though. 
	All in all the alternatives are too much effort so this is what I'll do */
	DestroyAllSelectablesOnMap();

	CheckIfStreamingComplete();
}

void ARTSGameState::DestroyAllSelectablesOnMap()
{	
	// For all actors...
	for (FActorIterator Iter(GetWorld()); Iter; ++Iter)
	{
		AActor * Actor = *Iter;
		
		/* Destroy all selectables excluding the ones we've explictly said not to. Resource spots 
		are an example of selectables not to destroy */
		if (Actor->GetClass()->ImplementsInterface(USelectable::StaticClass()) 
			&& GI->ShouldStayOnMap(Actor) == false)
		{
			Actor->Destroy();
		}
	}
}

void ARTSGameState::CheckIfStreamingComplete()
{
	/* + 1 is for lobby map */
	if (NumLevelsStreamedOut == GI->GetMapPool().Num() + 1
		&& bHasStreamedInMatchLevel)
	{
		/* Reset these for next match even though irrelenant since GS is destroyed with UWorld */
		NumLevelsStreamedOut = 0;
		bHasStreamedInMatchLevel = false;

		AckLevelStreamingComplete();

		if (GetWorld()->IsServer())
		{
			/* Server needed to wait for it's map to load before starting match, so do that now */
			GI->OnMatchLevelLoaded();
		}
	}
}

void ARTSGameState::CheckIfLevelsLoadedAndPostLoginsComplete()
{
	assert(!bHasAckedPostLoginsAndMaps);
	assert(ExpectedNumHumanPlayers > 0);

	bool bAllSetUp = false;

	if (ExpectedNumPlayers == PlayerArray.Num()) // Don't think this is ever false
	{
		if (ExpectedNumHumanPlayers == NumPCSetupAcks)
		{
			if (ExpectedNumHumanPlayers == NumLoadedLevelAcks)
			{
				bAllSetUp = true;
				bHasAckedPostLoginsAndMaps = true;
			}
			else
			{
				SetMatchLoadingStatus(ELoadingStatus::WaitingForAllPlayersToStreamInLevel);
			}
		}
		else
		{
			SetMatchLoadingStatus(ELoadingStatus::WaitingForPlayerControllerClientSetupForMatchAcknowledgementFromAllPlayers);
		}
	}
	else
	{
		SetMatchLoadingStatus(ELoadingStatus::WaitingForAllPlayersToConnect);
	}

	if (bAllSetUp)
	{
		// Move on to next part
		GI->OnLevelLoadedAndPCSetupsDone();
	}
}

ELoadingStatus ARTSGameState::GetMatchLoadingStatus() const
{
	return MatchLoadingStatus;
}

void ARTSGameState::SetMatchLoadingStatus(ELoadingStatus NewStatus)
{
	assert(GetWorld()->IsServer());

	MatchLoadingStatus = NewStatus;

	// Call explicitly because on server
	OnRep_MatchLoadingStatus();
}

void ARTSGameState::Server_AckPCSetupComplete(ARTSPlayerController * PlayerController)
{
#if WITH_EDITOR
	if (GI->EditorPlaySession_ShouldSkipMainMenu())
	{
		PIE_NumPCSetupCompleteAcks++;
	}
	else
#endif
	{
		NumPCSetupAcks++;
		CheckIfLevelsLoadedAndPostLoginsComplete();
	}
}

void ARTSGameState::AckLevelStreamingComplete()
{
	if (GetWorld()->IsServer())
	{
		NumLoadedLevelAcks++;
		CheckIfLevelsLoadedAndPostLoginsComplete();
	}
	else
	{
		ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
		PlayCon->Server_AckLevelStreamedIn();
	}
}

void ARTSGameState::Server_AckInitialValuesReceived()
{
#if WITH_EDITOR
	/* Trying to distinguish here between PIE and going straight to map, or anything else */
	if (GI->EditorPlaySession_ShouldSkipMainMenu())
	{
		// ARTSGameMode::GoToMapFromStartupPart4 is polling this
		PIE_NumPSSetInitialValuesAcks++;
	}
	else
#endif
	{
		NumInitialValueAcks++;

		/* Everyone including CPU players should ack they have received player state initial values
		including their own */
		const int32 NumAcksRequired = ExpectedNumHumanPlayers * ExpectedNumPlayers;

		if (NumInitialValueAcks == NumAcksRequired)
		{
			GI->OnInitialValuesAcked();
		}

		// TODO add RPC ack to PS multicast_SetInitialValues to call this. When enough acks have 
		// been received (need more acks than players because need one for each player state on 
		// each client)
	}
}

void ARTSGameState::Server_AckFinalSetupComplete()
{
	NumFinalSetupAcks++;

#if WITH_EDITOR
	if (GI->EditorPlaySession_ShouldSkipMainMenu() == false)
	{
		GI->OnMatchFinalSetupComplete();
	}
	else
	{
		// When testing in PIE it will poll NumFinalSetupAcks so do nothing
	}
#else
	GI->OnMatchFinalSetupComplete();
#endif
}

void ARTSGameState::StartMatch(const TMap < ARTSPlayerState *, FStartingSelectables > & StartingSelectables)
{
	SERVER_CHECK;
	
	/* Make this negative to account for the fact we have to sit through some camera fades */
	AccumulatedTimeTowardsNextGameTick = -3.f;
	/* If this throws then perhaps this game state is left over from another match although 
	that's not how this project is setup so that should dnever happen */
	assert(TickCounter == 0);
	SetActorTickEnabled(true);

	/* Start the match clock */
	StartMatchTimer();

	/* Start CPU players behavior */
	StartCPUPlayerBehavior(StartingSelectables);

	/* Enable player input, hide loading screen etc */
	Multicast_OnMatchStarted();

	/* Start defeat condition checking */
	ARTSGameMode * GameMode = CastChecked<ARTSGameMode>(GetWorld()->GetAuthGameMode());
	GameMode->StartDefeatConditionChecking(GI->GetMatchInfo().GetDefeatCondition());

	/* Not sure if I call ISelectable::Setup() on anything, or if it's even required. 
	I do it in GM when setting up for PIE */
}

void ARTSGameState::Multicast_OnMatchStarted_Implementation()
{
	if (GetWorld()->IsServer())
	{
		SetMatchLoadingStatus(ELoadingStatus::ShowingBlackScreenRightBeforeMatchStart);
	}

	/* For performance reasons destroying any main menu related widgets would be a good option now
	but I think they will not tick if not drawn therefore do not need to */

	/* Show 1 second of black screen. This should give time for starting selectables to fully
	replicate, but isn't required. */
	const float BlackScreenDuration = 1.f;
	GetWorld()->GetFirstPlayerController()->PlayerCameraManager->StartCameraFade(1.f, 1.f,
		BlackScreenDuration, FLinearColor::Black, true, true);

	FTimerHandle TimerHandle_Dummy;
	Delay(TimerHandle_Dummy, &ARTSGameState::OnMatchStarted_Part2, BlackScreenDuration);
}

void ARTSGameState::OnMatchStarted_Part2()
{
	ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
	PlayCon->OnMatchStarted();

	// This set probably not necessary
	//SetMatchLoadingStatus(ELoadingStatus::None);
	bIsInMatch = true;
}

void ARTSGameState::StartMatchTimer()
{
	TimeWhenMatchStarted = GetWorld()->GetTimeSeconds();
}

void ARTSGameState::StartCPUPlayerBehavior(const TMap <ARTSPlayerState *,FStartingSelectables>& StartingSelectables)
{
	/* May want to add check that this only gets called once before each match */

	/* Spawn the manager in charge of ticking the CPU player AI controllers */
	ACPUControllerTickManager * TickManager = GetWorld()->SpawnActor<ACPUControllerTickManager>(
		Statics::INFO_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

	for (const auto & Elem : GetCPUControllers())
	{
		/* Notify them of what their starting selectables are */
		const FStartingSelectables & StartingStuff = StartingSelectables[Elem->GetPS()];
		Elem->NotifyOfStartingSelectables(StartingStuff.GetBuildings(), StartingStuff.GetUnits());
		Elem->PerformFinalSetup();

		if (Elem->GetDifficulty() != ECPUDifficulty::DoesNothing)
		{
			/* Register with tick manager */
			TickManager->RegisterNewCPUPlayerAIController(Elem);
		}
	}
}

#if WITH_EDITOR
void ARTSGameState::StartPIEMatch(const TMap <ARTSPlayerState *,FStartingSelectables>& CPUStartingSelectables)
{
	SERVER_CHECK;
	
	MatchLoadingStatus = ELoadingStatus::None;
	bIsInMatch = true;

	AccumulatedTimeTowardsNextGameTick = 0.f;
	assert(TickCounter == 0);
	SetActorTickEnabled(true);

	/* Start the match clock */
	StartMatchTimer();

	StartCPUPlayerBehavior(CPUStartingSelectables);

	/* Enable input and show mouse cursor for all players */
	for (const auto & Elem : PlayerControllers)
	{
		Elem->Client_OnPIEStarted();
	}

	/** Start defeat condition checking. Use condition specified in development settings */
	const EDefeatCondition Condition = GI->EditorPlaySession_GetDefeatCondition();
	ARTSGameMode * GameMode = CastChecked<ARTSGameMode>(GetWorld()->GetAuthGameMode());
	GameMode->StartDefeatConditionChecking(Condition);
}
#endif


//----------------------------------------------------
//	Players Leaving 
//----------------------------------------------------

void ARTSGameState::HandleClientLoggingOutInMatch(ARTSPlayerController * Exiting)
{
	// TODO definently not done

	/* A lot of stuff needs to be done.
	Will need to:
	- Remove player controller and player state from lists

	Will probably need to multicast to update some stuff on clients too */

	/* Probably a lot of this code will fail if it is an edge case like match is half way
	through loading...
	not really going to factor that in here, will just have to crash if player leaves then */

	/* Bear in mind fog manager uses at least one of these arrays we're removing from...
	will need a lock */

	ARTSPlayerState * ExitingPlayerState = Exiting->GetPS();

	PlayerControllers.RemoveSwap(Exiting);

	if (Exiting->IsObserver())
	{
		Observers.RemoveSingleSwap(ExitingPlayerState);
	}
	else
	{
		PlayerStates.RemoveSingleSwap(ExitingPlayerState);

		const int32 Index = UndefeatedPlayers.Find(ExitingPlayerState);
		if (Index != INDEX_NONE)
		{
			// Player was previously not defeated

			UndefeatedPlayers.RemoveAtSwap(Index);

			// Check if match is over now that they are gone
			TArray <ARTSPlayerState *> Leaver;
			Leaver.Emplace(ExitingPlayerState);
			TSet <ETeam> RemainingTeams;
			if (IsMatchNowOver(Leaver, RemainingTeams))
			{
				// Stop defeat condition checking on game mode
				ARTSGameMode * GameMode = CastChecked<ARTSGameMode>(AuthorityGameMode);
				GameMode->StopDefeatConditionChecking();

				OnMatchWinnerFound(RemainingTeams);
			}
			else
			{
				/* Getting this called when exiting PIE with CPU players in it */

				Multicast_OnPlayersDefeated(Leaver);
			}
		}
		else
		{
			// Player was already defeated
		}
	}
}


//----------------------------------------------------
//	Match Ending
//----------------------------------------------------

void ARTSGameState::DisconnectAllClients()
{
	assert(GetWorld()->IsServer());

	/* Because it's assumed the match will end when this happens and therefore will change
	to a new map and all actors will be destroyed we will not bother updating any of the
	state variables here in GS since it will no longer exist soon */

	for (FConstPlayerControllerIterator Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		TWeakObjectPtr <APlayerController> PlayCon = *Iter;

		// Guard against doing anything to host
		if (PlayCon != GetWorld()->GetFirstPlayerController())
		{
			PlayCon->ClientReturnToMainMenuWithTextReason(FText::FromString("Host has left"));
		}
	}
}

bool ARTSGameState::IsMatchNowOver(const TArray<ARTSPlayerState*>& DefeatedThisCheck, TSet < ETeam > & OutWinningTeams)
{
	/* Do not call this unless a player has recently been defeated */
	assert(DefeatedThisCheck.Num() > 0);

	// Do not call this with items already in set
	assert(OutWinningTeams.Num() == 0);

	/* Check if match should end */

	for (const auto & Elem : UndefeatedPlayers)
	{
		OutWinningTeams.Emplace(Elem->GetTeam());

		if (OutWinningTeams.Num() == 2)
		{
			// Still at least 2 teams standing - match has not ended
			return false;
		}
	}

	if (OutWinningTeams.Num() == 1)
	{
		/* Case where a team has won
		Side note: if testing with PIE and skip main menu: if all players are the same team
		then when they get defeated they will actually be the victors. Not really a real-world
		scenario since all matches are expected to have 2+ teams to start with */

		return true;
	}
	else if (OutWinningTeams.Num() == 0)
	{
		/* Case where it is a draw. Add to out param the teams that were defeated this check */

		for (const auto & Elem : DefeatedThisCheck)
		{
			OutWinningTeams.Emplace(Elem->GetTeam());
		}

		return true;
	}

	return false;
}

void ARTSGameState::Multicast_OnPlayersDefeated_Implementation(const TArray<ARTSPlayerState*>& NewlyDefeatedPlayers)
{
	// Disable issuing any commands from these players. Destroy all selectables they own
	for (const auto & Player : NewlyDefeatedPlayers)
	{
		Player->OnDefeated();
	}
}

void ARTSGameState::OnMatchWinnerFound(const TSet<ETeam>& LastStandingTeams)
{
	assert(GetWorld()->IsServer());
	assert(LastStandingTeams.Num() > 0);

	// TODO disable all commands from being issued

	/* Transfer teams from set to array... RPC does not like TSet */
	TArray < ETeam > AsArray;
	AsArray.Reserve(LastStandingTeams.Num());
	for (const auto & Elem : LastStandingTeams)
	{
		AsArray.Emplace(Elem);
	}

	Multicast_OnMatchWinnerFound(AsArray);
}

void ARTSGameState::Multicast_OnMatchWinnerFound_Implementation(const TArray<ETeam>& LastStandingTeams)
{
	ARTSPlayerController * LocalPC = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());

	/* Disable game input. Make HUD uninteractable */
	LocalPC->DisableInput(LocalPC);
	LocalPC->DisableHUD();

	// Check if we won, lost or drew
	EMatchResult MatchResult;
	if (LastStandingTeams.Num() == 1)
	{
		/* A single team has won. Check if it is us */
		MatchResult = LastStandingTeams.Contains(LocalPlayersTeam)
			? EMatchResult::Victory : EMatchResult::Defeat;
	}
	else
	{
#if WITH_EDITOR
		/* When player plays with PIE and all players are the same team then when one gets
		defeated it will actually be a draw and .Num() == 1. Otherwise though there should be 2+
		teams in the TSet */
		if (GI->EditorPlaySession_ShouldSkipMainMenu() == false)
		{
			assert(LastStandingTeams.Num() >= 2);
		}
#endif

		/* 2+ teams have drawn. Check if it was us */
		MatchResult = LastStandingTeams.Contains(LocalPlayersTeam)
			? EMatchResult::Draw : EMatchResult::Defeat;
	}

	/* Show end of match widget */
	UInGameWidgetBase * Widget = LocalPC->ShowWidget(EMatchWidgetType::EndOfMatch, true);
	UEndOfMatchWidget * EndOfMatchWidget = CastChecked<UEndOfMatchWidget>(Widget);
	EndOfMatchWidget->SetupForResult(MatchResult);
}


//----------------------------------------------------
//	In-Match Messaging
//----------------------------------------------------

void ARTSGameState::SendInMatchChatMessageToEveryone(ARTSPlayerState * Sender, const FString & Message)
{
	assert(GetWorld()->IsServer());

	/* Could use a multicast but is done like this for the main reason that it ensures chat logs
	are the exact same order across all clients because of guaranteed RPC ordering on the same
	object */
	for (const auto & PlayCon : PlayerControllers)
	{
		PlayCon->Client_OnAllChatInMatchChatMessageReceived(Sender, Message);
	}
}

void ARTSGameState::SendInMatchChatMessageToTeam(ARTSPlayerState * Sender, const FString & Message)
{
	assert(GetWorld()->IsServer());

	const int32 Index = Statics::TeamToArrayIndex(Sender->GetTeam());
	const TArray < ARTSPlayerState * > & Friendlies = Teams[Index].GetPlayerStates();

	for (const auto & PlayerState : Friendlies)
	{
		// Don't try send messages to CPU players
		if (PlayerState->bIsABot == false)
		{
			PlayerState->GetPC()->Client_OnTeamChatInMatchChatMessageReceived(Sender, Message);
		}
	}
}

void ARTSGameState::Delay(FTimerHandle & TimerHandle, void(ARTSGameState::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}


