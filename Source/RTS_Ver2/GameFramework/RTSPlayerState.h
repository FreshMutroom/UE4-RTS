// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "Statics/Structs_1.h"
#include "Statics/Structs_3.h"
#include "RTSPlayerState.generated.h"

class URTSGameInstance;
class ARTSGameState;
class ARTSPlayerController;
class AAreaEffect;
class AUpgradeManager;
class AObjectPoolingManager;
class AFactionInfo;
class URTSHUD;
class UAudioComponent;
class ABuilding;
class ACPUPlayerAIController;
class AInfantry;
class UCommanderSkillTreeWidget;
class UCommanderSkillTreeNodeWidget;


//==============================================================================================
//	Structs
//==============================================================================================

/** 
 *	Array of EUpgradeType. 
 *	
 *	Where I use this we could replace with a TSet. The only reason it is an array is that the 
 *	ArePrerequisitesMet func that returns the type of missing prereq would require getting an 
 *	element from the TSet which I assume reqires iteration of it. 
 *	In fact I could even use an int32 instead of this struct if the ArePrerequisitesMet that 
 *	returns the missing upgrade type didn't exist.
 */
struct FUpgradeTypeArray
{
	FUpgradeTypeArray(int32 ReserveAmount)
	{
		Array.Reserve(ReserveAmount);
	}

	TArray<EUpgradeType> Array;
};


/* A simple struct that holds a TSet as a workaround for non-2D TArrays */
USTRUCT()
struct FBuildingSet
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	TSet < ABuilding * > Buildings;

public:

	const TSet <ABuilding *> & GetSet() const { return Buildings; }

	void AddBuilding(ABuilding * InBuilding) { Buildings.Emplace(InBuilding); }
	void RemoveBuilding(ABuilding * InBuilding) { Buildings.Remove(InBuilding); }
};


/* Array of buildings as aworkaround for non-2D TArrays */
USTRUCT()
struct FBuildingArray
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	TArray < ABuilding * > Buildings;

public:

	const TArray < ABuilding * > & GetArray() const { return Buildings; }

	void AddBuilding(ABuilding * InBuilding) { Buildings.Emplace(InBuilding); }
	void RemoveBuilding(ABuilding * InBuilding) { Buildings.RemoveSwap(InBuilding); }
};


//==============================================================================================
//	Player state implementation
//==============================================================================================

/**
 *	RTS Player state. Used by both human players and CPU players
 */
UCLASS()
class RTS_VER2_API ARTSPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ARTSPlayerState();

protected:

	virtual void BeginPlay() override;

private:

	//-----------------------------------------
	//	Actor Components
	//-----------------------------------------

	/* Sound component to play 'building just built' sound when building has been fully
	constructed */
	UPROPERTY()
	UAudioComponent * BuildingBuiltAudioComp;

	/* Sound component to play 'Unit just built' sound e.g. "Kirov reporting", "My life for Aiur" */
	UPROPERTY()
	UAudioComponent * UnitBuiltAudioComp;


	//-----------------------------------------
	//	Other Stuff
	//-----------------------------------------

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	/* Variables for different resource types. For now they are hardcoded - one for each resource
	type but they don't have to be - could use an array instead. I would just prefer a solution
	where checks to see if resources have changed are quick and bandwidth stays low so this is
	how I am doing it for now */

	/* Array that holds current resource values. Updated when an OnRep for a resource happens */
	UPROPERTY()
	TArray < int32 > ResourceArray;

	/* Array that holds previous resource values before a new value was replicated. Updated when
	an OnRep for a resource happens */
	UPROPERTY()
	TArray < int32 > OldResourceArray;

	/* Resource array for housing resources. Not replicated */
	UPROPERTY()
	TArray < FHousingResourceState > HousingResourceArray;

	//~ Start hardcoded resource variables

	/* Amount of cash resource */
	UPROPERTY(ReplicatedUsing = OnRep_ResourcesCash)
	int32 Resource_Cash;

	/* Amount of sand resource */
	UPROPERTY(ReplicatedUsing = OnRep_ResourcesSand)
	int32 Resource_Sand;

	//~ End hardcoded resource variables

	/* All the information about all building garrison networks for buildings owned by this player */
	FPlayersBuildingNetworksState BuildingGarrisonNetworkInfos;

	//-----------------------------------------------------------
	//	------- Experience, rank and commander skills -------
	//-----------------------------------------------------------

	/** 
	 *	How much total experience the player has obtained. Does not decrease when the player 
	 *	levels up 
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Experience)
	float Experience;

	int32 NumUnspentSkillPoints;

	/* The player's rank or level. This starts at 0 */
	uint8 Rank;

	/* Ordering is important. It should be the same across server/clients.
	This container belongs on the faction info really */
	TArray <const FCommanderAbilityTreeNodeInfo *> AllCommanderSkillTreeNodes;

	/* Maps ability type to which skill tree node it belongs to. 
	This container belongs on the faction info really */
	TMap <const FCommanderAbilityInfo *, ECommanderSkillTreeNodeType> CommanderAbilityToNodeType;

	/** 
	 *	Abilities from the commander skill tree the player has not aquired yet. 
	 */
	UPROPERTY()
	TMap<ECommanderSkillTreeNodeType, FUnaquiredCommanderAbilityState> UnaquiredCommanderAbilities;

	/* Abilities the player has aquired */
	UPROPERTY()
	TArray<FAquiredCommanderAbilityState> AquiredCommanderAbilities;

	TMap <ECommanderSkillTreeNodeType, FAquiredCommanderAbilityState *> AquiredCommanderAbilitiesTMap;

	// --------------------- End experience, rank and commander skills ---------------------

	/* The UI image for this player. Maybe this could go on the ULocalPlayer class instead. 
	I don't think I actually let the player set this in any way */
	UPROPERTY()
	FSlateBrush PlayerNormalImage;
	UPROPERTY()
	FSlateBrush PlayerHoveredImage;
	UPROPERTY()
	FSlateBrush PlayerPressedImage;

	/* Whether this player state belongs to the local player */
	bool bBelongsToLocalPlayer;

	/* If true then this player has been defeated */
	bool bHasBeenDefeated;

	/* Reference to the players HUD widget */
	UPROPERTY()
	URTSHUD * HUDWidget;

	/* ID as a single byte */
	uint8 IDAsInt;

	/* ID of player who owns this state. Used to determine
	what can and can't be selected. Unique to each player */
	UPROPERTY()
	FName ID;

	ETeam Team;

	/* Team as FName */
	UPROPERTY()
	FName TeamTag;

	// Affiliation towards local player. Not safe to use until Client_FinalSetup() has completed
	EAffiliation Affiliation;

	EFaction Faction;

	/* Unique ID of RTS player start player started match at. Can change and is only final 
	after GI::AssignOptimalStartingSpots has been called. -2 implies was never assigned a 
	spot and was most likely spawned at origin */
	int16 StartingSpot;

	/* The actual world location on the map where the player starts */
	FVector StartLocation;

	/* Reference to the faction info of the faction of this player */
	UPROPERTY()
	AFactionInfo * FI;

	/** 
	 *	Number of selectables that are in production queues, so does not include upgrades. 
	 *	
	 *	This variable was added because of the selectable cap 
	 */
	int32 NumQueuedSelectables;

	/* Holds all the buildings this player owns. Includes both placed and fully constructed 
	buildings (comment used to say just fully constructed but from inspecting code that looks 
	to be wrong) */
	UPROPERTY()
	TArray <ABuilding *> Buildings;

	/**
	 *	Maps building type to how many of it are in all production queues for this player. 
	 *	This includes building's production queues 
	 *
	 *	This was added for implementing unique buildings
	 */
	UPROPERTY()
	TMap < EBuildingType, int32 > InQueueBuildingTypeQuantities;

	/**
	 *	Maps building type to how many of that type this unit has placed but not fully
	 *	constructed.
	 *
	 *	This was added for implementing unique buildings
	 */
	UPROPERTY()
	TMap <EBuildingType, int32> PlacedBuildingTypeQuantities;

	/**
	 *	Maps building type to how many of that type this player has that are fully constructed.
	 *	No entry means they have 0.
	 *
	 *	Useful to know what prerequisites are fulfilled
	 *
	 *	My notes: this was called PrerequisitesMet or similar for a long time
	 */
	UPROPERTY()
	TMap < EBuildingType, int32 > CompletedBuildingTypeQuantities;

	/** 
	 *	Maps unit type to the quantity that is in a production queue. Considers all production 
	 *	queues this player owns 
	 *	
	 *	This was added to implement unique units 
	 */
	UPROPERTY()
	TMap < EUnitType, int32 > InQueueUnitTypeQuantities;

	/* Holds all the infantry this player owns. */
	UPROPERTY()
	TArray < AInfantry * > Units;

	/** 
	 *	Maps unit type to the quantity of that unit the player has. No key/value pair means 
	 *	the player has 0 of that type. 
	 *
	 *	This does not take into account units that are in production queues
	 *
	 *	This was added to implement unique units
	 */
	UPROPERTY()
	TMap < EUnitType, int32 > UnitTypeQuantities;

	/** 
	 *	Reference to player controller that owns this. Will be null for clients that do not own 
	 *	this. Will be null if player state belongs to CPU player 
	 */
	UPROPERTY()
	ARTSPlayerController * PC;

	/** The AI controller who owns this. Will be null if player state belongs to a human player */
	UPROPERTY()
	ACPUPlayerAIController * AICon;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to upgrade manager */
	UPROPERTY()
	AUpgradeManager * UpgradeManager;

public:

	/** 
	 *	These containers map building/unit/upgrade type to an array of all the upgrades that 
	 *	are prerequisites for it and that have not been researched yet. 
	 *	
	 *	If a building/unit/upgrade does not have any upgrade prerequisites then it might not get 
	 *	any entry in the map. 
	 */
	TMap<EBuildingType, FUpgradeTypeArray> BuildingTypeToMissingUpgradePrereqs;
	TMap<EUnitType, FUpgradeTypeArray> UnitTypeToMissingUpgradePrereqs;
	TMap<EUpgradeType, FUpgradeTypeArray> UpgradeTypeToMissingUpgradePrereqs;

private:

	/* Set of resource depots owned by this player. Used to calculate
	the closest depot when collectors return from gathering resources */
	UPROPERTY()
	TMap <EResourceType, FBuildingSet> ResourceDepots;

	/* Update each resource spots closest depot for this player.
	Should be called when a new resource depot is built
	@param ResourceType - resource type building is a depot for
	@param Building - The building that just finshed construction
	@pararm bWasBuilt - true if building was just built, false if it was just destroyed */
	void UpdateClosestDepots(EResourceType ResourceType, ABuilding * Depot, bool bWasBuilt);

	/* Buildings that have at least one persistent build slot, like construction yard type
	buildings */
	UPROPERTY()
	TSet <ABuilding *> PersistentQueueSupportingBuildings;

	/* Maps a button type to all the buildings that support producing what that button requests. */
	UPROPERTY()
	TMap <FContextButton, FBuildingArray> ProductionCapableBuildings;

	/* Get building to build from when clicking a HUD persistent tab button like in C&C.
	@param Button - button that was clicked on
	@param bShowHUDWarning - whether to try and show a warning message on the HUD if no queue
	could be found
	@param OutQueueType - the type of production queue that was chosen, or None if no queue could
	be found */
	virtual ABuilding * GetProductionBuilding(const FContextButton & Button, bool bShowHUDWarning, 
		EProductionQueueType & OutQueueType);

	/* Array of buildings that have completed construction of a building using the BuildsInTab
	build method and therefore it should be ready to place when player clicks HUD persistent panel
	button corrisponding to it. This array contains the constructors, not the constructed */
	UPROPERTY()
	TArray <ABuilding *> BuildsInTabCompleteBuildings;

	/* Since I'm using a single hardcoded variable for each resource type, get a reference to
	that variable */
	int32 & GetReplicatedResourceVariable(EResourceType ResourceType);

	typedef void (ARTSPlayerState:: *FunctionPtrType)(void);
	// Return pointer to OnRep function for a replicated resource variable
	FunctionPtrType GetReplicatedResourceOnRepFunction_DEPRECIATED(EResourceType ResourceType) const;
	FunctionPtrType GetReplicatedResourceOnRepFunction(EResourceType ResourceType) const;

	//~ Functions to convert between int32 and EResourceType
	int32 ResourceTypeToArrayIndex(EResourceType ResourceType) const;
	EResourceType ArrayIndexToResourceType(int32 ResourceArrayIndex) const;

	UFUNCTION()
	void OnRep_ResourcesCash();

	UFUNCTION()
	void OnRep_ResourcesSand();

	/* These are similar to the OnRep_Resources... functions */
	void Server_PostResourceChange_Cash();
	void Server_PostResourceChange_Sand();

	UFUNCTION()
	void OnRep_Experience();

	/* Called when the player gains a rank 
	
	@param LevelGainedsInfo - info struct for the level that was gained */
	void OnLevelUp_NotLastForEvent(uint8 OldRank, const FCommanderLevelUpInfo & LevelGainedsInfo);

	void OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, const FCommanderLevelUpInfo & LevelGainedsInfo);

	/** 
	 *	[Server] A queue that contains IDs that haven't been assigned to any selectables. 
	 *	This queue is only ever modified by this class on the server only so Spsc is 
	 *	very fitting
	 *	
	 *	It can range from having 0 elements (at selectable cap) to 
	 *	ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER (0 selectables) 
	 *
	 *	Side note: after adding this variable to this class a editor restart is required otherise 
	 *	you get massive memory usage which eventually uses all system memory. I also vaguely 
	 *	remember this happening when I tried using this a while back.
	 */
	TQueue < uint8, EQueueMode::Spsc > IDQueue;

	/** 
	 *	Number of items in IDQueue. Useful when we want to know whether we are at the 
	 *	selectable cap or not.
	 *
	 *	We actually also update on the client to serve as a way for them to know whether they 
	 *	are at the selectable cap or not  
	 */
	uint8 IDQueueNum;

	/** 
	 *	Maps ID to selectable for issuing commands. Key = selectable's ID. 
	 *	
	 *	Pretty sure we maintain this for all players but really that could be unecessary. Only 
	 *	the server player and the player this player state belongs to need to maintain it.
	 */
	UPROPERTY()
	TArray <AActor *> IDMap;

	/* [Server] Initialize the selectable ID queue */
	void InitIDQueue();

	/* Send RPC to show a game message on the HUD */
	UFUNCTION(Client, Reliable)
	void Client_ShowHUDNotificationMessage(EGameNotification NotificationType);

	/** 
	 *	Called in OnGameWarningHappened. Return whether a game warning should be shown on the HUD
	 *	or not. This is here to throttle the number of warnings that appear on HUD and to keep 
	 *	server-to-client bandwidth low 
	 */
	virtual bool CanShowGameWarning(EGameWarning MessageType);

	/* Version for custom ability requirements */
	virtual bool CanShowGameWarning(EAbilityRequirement ReasonForMessage);

	/* Send RPC to client to show game message on HUD */
	UFUNCTION(Client, Unreliable)
	void Client_OnGameWarningHappened(EGameWarning MessageType) const;
	UFUNCTION(Client, Unreliable)
	void Client_OnGameWarningHappenedAbility(EAbilityRequirement ReasonForMessage) const;
	/* Send RPC to client to show a "not enough of a resource" type message */
	UFUNCTION(Client, Unreliable)
	void Client_OnGameWarningHappenedMissingResource(EResourceType ResourceType) const;
	UFUNCTION(Client, Unreliable)
	void Client_OnGameWarningHappenedMissingHousingResource(EHousingResourceType HousingResourceType) const;

	/** 
	 *	Call function that returns void after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ARTSPlayerState:: *Function)(), float Delay);

	void DoNothing();

public:

	void StartAbilitysCooldownTimer(FTimerHandle & AbilitysCooldownTimerHandle, FAquiredCommanderAbilityState * Ability, float DelayAmount);

	/** 
	 *	Called when an item is added to any production queue this player owns. Whether it was 
	 *	the 1st or 100th item to be added to the queue this is called. 
	 *	Called by both server and owning client only
	 *	
	 *	@param Item - Item that represents the item that was added to the queue 
	 */
	void OnItemAddedToAProductionQueue(const FTrainingInfo & Item);

	/** 
	 *	Called when an item is removed from any production queue this player owns 
	 *	Called by both server and owning client only
	 *
	 *	@param Item - the item that was removed 
	 */
	void OnItemRemovedFromAProductionQueue(const FTrainingInfo & Item);

	/* Call from building when placed */
	void OnBuildingPlaced(ABuilding * Building);

	/** 
	 *	Call from building when has finished construction.
	 *	
	 *	@param Building - building that was just built
	 *	@param CreationMethod - how the building was created
	 */
	void OnBuildingBuilt(ABuilding * Building, EBuildingType BuildingType, ESelectableCreationMethod CreationMethod);

	/* Call from building when it reaches zero health */
	void OnBuildingZeroHealth(ABuilding * Building);

	/* Call when a building is considered destroyed */
	void OnBuildingDestroyed(ABuilding * Building);

	void RemoveBuildingFromBuildingsContainer(ABuilding * Building);

	/** 
	 *	Call when a unit is built
	 *	
	 *	@param Unit - unit that was just built
	 *	@param UnitType - type of unit just built for convenience
	 *	@param CreationMethod - way the infantry was spawned 
	*/
	void OnUnitBuilt(AInfantry * Unit, EUnitType UnitType, ESelectableCreationMethod CreationMethod);

	/** 
	 *	Call when a unit is destroyed. Removes unit from Units container and frees up their 
	 *	selectable ID 
	 */
	void OnUnitDestroyed(AInfantry * Unit);

	/* Only called during faction info creation */
	void Setup_OnUnitDestroyed(AInfantry * Unit);

	/* Sets some values */
	void Server_SetInitialValues(uint8 InID, FName InPlayerIDAsFName, ETeam InTeam, EFaction InFaction,
		int16 InStartingSpot, const TArray <int32> & StartingResources);

	/* Basically the same as Server_SetInitialValues but sends changes to every client. The
	variables set with the server version are replicated but we need a 100% sure confirmation
	that they have been received so RPC is used */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetInitialValues(uint8 InID, FName InPlayerIDAsFName, ETeam InTeam, EFaction InFaction);

	/* Set the world location where the player starts */
	void SetStartLocation(const FVector & InLocation);

	/* Before match setup anything that requires all info about all players to be known. Great
	place to put anything you want to do before match starts */
	UFUNCTION(Client, Reliable)
	void Client_FinalSetup();

	/* Returns true if this player state belongs to the local player */
	bool BelongsToLocalPlayer() const;

private:

	bool HasBeenDefeated() const;

	/* Setup upgrade manager */
	void SetupUpgradeManager();

	/* Constantly check if setup has fully completed */
	void BusyWaitForSetupToComplete();

	/* Return whether this player state has fully setup, everything has repped and is ready to
	be used in a match */
	bool HasFullySetup() const;

	/* [Private] Set the amount of resource to have at the start of the match */
	void Server_SetInitialResourceAmounts(const TArray < int32 > & InitialAmounts);

	/* Send ack that Client_FinalSetup has completed */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AckFinalSetupComplete();

public:

	/* Setup resource arrays and HUD on client-side */
	void Client_SetInitialResourceAmounts(const TArray < int32 > & InitialAmounts, URTSHUD * InHUD);

	/** 
	 *	Get resource cost array for a building
	 *	
	 *	@return - cost of the building 
	 */
	const TArray < int32 > & GetBuildingResourceCost(EBuildingType Type) const;

	/* Get resource cost array for a unit */
	const TArray < int32 > & GetUnitResourceCost(EUnitType Type) const;

#if ENABLE_VISUAL_LOG

	/** 
	 *	Get how much of a resource player has. Added here for CPU player AI controllers
	 *
	 *	@param ArrayIndex - Statics::ResourceTypeToArrayIndex(ResourceType) 
	 */
	int32 GetNumResources(int32 ArrayIndex) const;

#endif 

	/** 
	 *	Check if player has enough resources to buy something given its cost array. If not then will
	 *	try to display a message on the HUD letting the player know they do not have enough
	 *	
	 *	@param - CostArray - the cost array of something
	 *	@param bShowHUDMessage - if not enough resources whether we want to try and show a message on
	 *	the HUD
	 *	@return - true if player can afford 
	 */
	bool HasEnoughResources(const TArray < int32 > & CostArray, bool bShowHUDMessage);

	/**
	 *	Check if player has enough resources. If this returns "None" then player has enough
	 *	resources. Otherwise it returns the first resource that the player does not have enough of
	 *
	 *	return - the first missing resource, or "None" if the player has enough resources
	 */
	EResourceType HasEnoughResourcesSpecific(const TArray <int32> & CostArray);

	/** 
	 *	Check if player has enough resources to build building
	 *	
	 *	@param Type - the type of building
	 *	@return - true if player can afford building 
	 */
	bool HasEnoughResources(EBuildingType Type, bool bShowHUDMessage);

	/** 
	 *	Check if player has enough resources to train unit
	 *	
	 *	@param Type - the type of unit
	 *	@return - true if player can afford unit 
	 */
	bool HasEnoughResources(EUnitType Type, bool bShowHUDMessage);

	/* Check if player has enough resources to research upgrade */
	bool HasEnoughResources(EUpgradeType UpgradeType, bool bShowHUDWarning);

	/* Check if player has enough resources to produce what button requests */
	bool HasEnoughResources(const FContextButton & Button, bool bShowHUDWarning);

	/* Check if player has enough resources to produce what the training info requests */
	bool HasEnoughResources(const FTrainingInfo & InTrainingInfo, bool bShowHUDWarning);

	/** 
	 *	Check if the player has enough of a housing type resource to produce the item. 
	 *	
	 *	Housing type resources are resources like the stuff you get when you build a 
	 *	pylon/overlord/depot in starcraft 
	 */
	bool HasEnoughHousingResources(const TArray < int16 > & HousingCostArray, bool bShowHUDWarning);
	bool HasEnoughHousingResources(EUnitType UnitType, bool bShowHUDWarning);

	void GainExperience(float RawExperienceBounty, bool bApplyRandomness);

	/* [Server] [Owning client] Get how much experience in total the player has aquired */
	float GetTotalExperience() const;

	/* [Server] [Owning client] */
	uint8 GetRank() const;

	/* Return how many commander skill points this player has */
	int32 GetNumUnspentSkillPoints() const;
	
	/* Setupy type function */
	void RegisterCommanderSkillTreeNode(const FCommanderAbilityTreeNodeInfo * NodeInfo, uint8 ArrayIndex);

	/* Get the ability state that belongs to an ability. This is kind of slow - have to do 
	2 TMap lookups */
	FAquiredCommanderAbilityState * GetCommanderAbilityState(const FCommanderAbilityInfo & AbilityInfo) const;

	/* Given some commander ability info get which node on the skill tree that ability is on. 
	Will crash if it is not on any node. This function is more suited to be on FI */
	ECommanderSkillTreeNodeType GetCommanderSkillTreeNodeType(const FCommanderAbilityInfo * AbilityInfo) const;

	/** 
	 *	Get the rank the player has obtained for a commander ability. -1 means they have not 
	 *	obtained it. The return value is 0 indexed meaning a value of 0 means the first rank has been 
	 *	obtained. 
	 */
	int32 GetCommanderAbilityObtainedRank(const FCommanderAbilityTreeNodeInfo & NodeInfo) const;

	/* Return true if the player can afford to aquire the next rank of a commander ability. 
	If the player already has the max rank of the ability... decide on what to return there */
	bool CanAffordCommanderAbilityAquireCost(const FCommanderAbilityTreeNodeInfo & NodeInfo) const;

	bool ArePrerequisitesForNextCommanderAbilityRankMet(const FCommanderAbilityTreeNodeInfo & NodeInfo) const;

	TMap<ECommanderSkillTreeNodeType, FUnaquiredCommanderAbilityState> & GetUnaquiredCommanderAbilities();

	/* @return - None if the ability can be aquired, something different otherwise */
	EGameWarning CanAquireCommanderAbility(const FCommanderAbilityTreeNodeInfo & NodeInfo, bool bHandleHUDWarnings);

	/* This version takes an array index which is the index in the AllCommanderSkillTreeNodes arary */
	EGameWarning CanAquireCommanderAbility(uint8 AllNodesArrayIndex, bool bHandleHUDWarnings);

	/* This version takes an array index value */
	void AquireNextRankForCommanderAbility(uint8 AllNodesArrayIndex, bool bInstigatorIsServerPlayer);

	/** 
	 *	Aquire the next rank of a commander ability. Next rank can mean the first rank. This 
	 *	will also update the HUD, skill tree widget, skill tree node widget and tooltips.
	 */
	void AquireNextRankForCommanderAbility(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
		bool bInstigatorIsServerPlayer, uint8 AllNodesArrayIndex);

	/* This function will actually increment rank and stuff */
	void AquireNextRankForCommanderAbilityInternal(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
		bool bInstigatorIsServerPlayer, uint8 AllNodesArrayIndex);

protected:

	/* Intended not to be called for nodes that execute on aquired */
	UFUNCTION(Client, Reliable)
	void Client_AquireNextRankForCommanderAbility(uint8 AllNodesArrayIndex);

public:

	/* Called when an ability is used by this player */
	void OnCommanderAbilityUse(const FCommanderAbilityInfo & AbilityInfo, uint8 GameTickCountAtTimeOfAbilityUse);

	/** 
	 *	Called when a commander ability comes off cooldown, either the initial cooldown if it 
	 *	has one or the regular cooldown. 
	 */
	UFUNCTION()
	void OnCommanderAbilityCooledDown(FAquiredCommanderAbilityState & CooledDownAbility);

	/** 
	 *	Get the actor to build something from when clicking a HUD persistent tab button
	 *	
	 *	@param Button - button type on HUD that was clicked
	 *	@param bShowHUDWarning - if true and the func returns null, then a warning message will
	 *	try to be shown on the HUD
	 *	@param OutActor - Out param which will point to non-null if an actor was found. This is 
	 *	always a building right now but may change in future if different actors are allowed to 
	 *	produce stuff too
	 *	@param QueueType - Out param. The type of queue the selected actor will use 
	 */
	virtual void GetActorToBuildFrom(const FContextButton & Button, bool bShowHUDWarning, 
		ABuilding *& OutActor, EProductionQueueType & QueueType);

	/* Get array of buildings that have a queue */
	TArray < ABuilding * > & GetBuildsInTabCompleteBuildings();

	/* When a building completes cosntruction of a BuildsInTab building it should call this.
	@param ProducerBuilding - the building that has produced the new building */
	void OnBuildsInTabProductionComplete(ABuilding * ProducerBuilding);

	/* Show a game notification on HUD */
	void OnGameEventHappened(EGameNotification NotificationType);

	/* Try show a message on the HUD if allowed */
	void OnGameWarningHappened(EGameWarning MessageType);

	/* Try to show a message on the HUD if allowed. Version that takes a custom ability 
	requirement reason. */
	void OnGameWarningHappened(EAbilityRequirement ReasonForMessage);

protected:

	/* Version for when we do not have enough resources. Will try show a message on the HUD */
	void TryShowGameWarning_NotEnoughResources(EResourceType ResourceType);

	/* Version for when we do not have enough housing resources. Will try show a message on the HUD */
	void OnGameWarningHappened(EHousingResourceType HousingResourceType);

public:

	/* Version for when player does not have enough of a resource */
	void OnGameWarningHappened(EResourceType MissingResource);

	/* Timer handle for throttling game messages that appear on HUD */
	FTimerHandle TimerHandle_GameWarning;

	/** 
	 *	[Server] Subtract a single resource and make sure HUD gets updated. 
	 *
	 *	@param ResourceType - resource type to adjust
	 *	@param AmountToSpend - amount to subtract
	 *	@param bClampToWithinLimits - whether to clamp the resource to within limits if the spend 
	 *	amount would otherwise cause it to go outside
	 *	@return - amount of resource after the change
	 */
	int32 SpendResource(EResourceType ResourceType, int32 AmountToSpend, bool bClampToWithinLimits);
	
	/** 
	 *	[Server] Add a single resource and make sure HUD gets updated. This version of the function 
	 *	will clamp the resource to within its limits if the gain amount would cause it to 
	 *	go outside otherwise. 
	 *
	 *	@param ResourceType - resource type to adjust 
	 *	@param AmountToGain - amount to gain
	 *	@return - amount of resource after the change
	 */
	int32 GainResource(EResourceType ResourceType, int32 AmountToGain, bool bClampToWithinLimits);

	/** 
	 *	[Server] Spend some of potentially all resource types and make sure HUD gets updated
	 *	
	 *	@param CostArray - a cost array to subtract 
	 */
	void SpendResources(const TArray < int32 > & CostArray, bool bClampToWithinLimits = false);

	/** 
	 *	[Server] Gain some of potentially all resource types and make sure HUD gets updated
	 *	
	 *	@param GainArray - a cost array to gain 
	 */
	void GainResources(const TArray <int32> & GainArray, bool bClampToWithinLimits = false);
	void GainResources(int32 * GainArray, bool bClampToWithinLimits = false);

	/**
	 *	[Server] Gain some of potentially all resource types and make sure HUD gets updated. Multiply 
	 *	each amount by a multiplier.
	 *
	 *	@param GainArray - a cost array to gain
	 *	@param GainMultiplier - how much to multiply each entry in GainArray by
	 */
	void GainResources(const TArray <int32> & GainArray, float GainMultiplier, bool bClampToWithinLimits = false);

	/** 
	 *	[Server] Adjust resource amounts. Also stores the resource amounts before the adjustment and 
	 *	after the adjustment. This  
	 *	
	 *	@param AdjustArray - amount to add to resources 
	 *	@param PreAdjustAmounts - out array param. Will contain how much resources player had before the 
	 *	adjustment. 
	 *	@param PostAdjustAmounts - out array param. Will contain how much resources the player had after 
	 *	the adjustment.
	 */
	void AdjustResources(const TArray <int32> & AdjustArray, int32 * PreAdjustAmounts, 
		int32 * PostAdjustAmounts, bool bClampToWithinLimits = false);

protected:

	/** 
	 *	[Server] Clamps a resource variable to within its limits which usually just means no 
	 *	less than 0. 
	 */
	static void ClampResource(int32 & ResourceVariable);

public:

	/* Increase how much of housing resources we are using */
	void AddHousingResourceConsumption(const TArray <int16> & HousingCostArray);
	void AddHousingResourceConsumption(EUnitType UnitType);

	/* Decrease how much of housing resources we are using. */
	void RemoveHousingResourceConsumption(const TArray <int16> & HousingCostArray);
	void RemoveHousingResourceConsumption(EUnitType UnitType);

protected:

	void AddHousingResourcesProvided(const TArray < int16 > & HousingProvidedArray);
	void RemoveHousingResourcesProvided(const TArray < int16 > & HousingProvidedArray);

public:

	/* Return the number of persistent queues this player has built. This returns the number of
	construction yard type buildings */
	int32 GetNumPersistentQueues() const;

	/* For a given button get the number of buildings that can produce what it requests */
	int32 GetNumSupportedProducers(const FContextButton & InButton) const;

	/* Get all the buildings that can produce what the button requests */
	const TArray < ABuilding * > & GetProductionCapableBuildings(const FContextButton & Button) const;

	/* Get reference to the set of built buildings that are construction yards */
	const TSet < ABuilding * > & GetPersistentQueueSupportingBuildings() const;

	/** 
	 *	Check if the prerequisite buildings are built in order to build a certain 
	 *	building/unit/upgrade. Overloaded for buildings, units and upgrades
	 *	
	 *	@param TheFirstParam - what to check prereqs for. Accepts many different types 
	 *	@param bShowHUDMessage - if true and prereqs are not met then a message will try to be
	 *	displayed on the HUD saying so
	 *	@return - true if the prerequisites are met 
	 */
	bool ArePrerequisitesMet(const FContextButton & InButton, bool bShowHUDMessage);
	bool ArePrerequisitesMet(EBuildingType BuildingType, bool bShowHUDMessage);
	bool ArePrerequisitesMet(EUnitType UnitType, bool bShowHUDMessage);
	bool ArePrerequisitesMet(EUpgradeType UpgradeType, bool bShowHUDMessage);
	bool ArePrerequisitesMet(const FUpgradeInfo & UpgradeInfo, EUpgradeType UpgradeType, bool bShowHUDMessage);
	bool ArePrerequisitesMet(const FTrainingInfo & InTrainingInfo, bool bShowHUDMessage);

	/** 
	 *	Versions that will also output the first prerequisite encountered that was not met 
	 *	
	 *	@param OutFirstMissingPrereq_Building - if returned false the first prereq encountered that was not 
	 *	met. If it's an upgrade that is mssing then this will have the value EBuildingType::NotBuilding 
	 *	@param OutFirstMissingPrereq_Upgrade - if returned false and OutFirstMissingPrereq_Building == EBuildingType::NotBuilding 
	 *	then it was an upgrade that was missing and this will contain the missing upgrade's type 
	 *	@return - true if all prerequisites are met 
	 */
	bool ArePrerequisitesMet(EBuildingType BuildingType, bool bShowHUDMessage,
		EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade);
	bool ArePrerequisitesMet(EUnitType UnitType, bool bShowHUDMessage,
		EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade);
	bool ArePrerequisitesMet(EUpgradeType UpgradeType, bool bShowHUDMessage,
		EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade);

	/** 
	 *	Given a button return whether there exists a building that can produce what is requests. 
	 *	Does not take into account whether the building's queues are full or not.
	 *	
	 *	More specific: 
	 *	For build building buttons: returns true if at least one persistent queue exists 
	 *	For other buttons: true if at least one building is built that has this button included as a
	 *	button in its context menu 
	 */
	bool HasQueueSupport(const FContextButton & Button, bool bTryShowHUDWarnings);

	/** 
	 *	Return whether this player is at the selectable cap i.e. they should not allowed to 
	 *	produce anymore selectables. This cap is a technical limitation and has nothing to do 
	 *	with population caps like the 200 population limit in SCII 
	 *
	 *	@param bIncludeQueuedSelectables - whether to include selectables queued in production 
	 *	queues. This will consider all queues, including the one on this player state plus all 
	 *	building's queues
	 *	@param bTryShowHUDWarning - if this func will return true, whether we should try and 
	 *	show a message on the HUD
	 *	@return - true if at selectable cap implying we cannot spawn anymore selectables
	 */
	bool IsAtSelectableCap(bool bIncludeQueuedSelectables, bool bTryShowHUDMessage);

	/** 
	 *	[Server] Return an ID that hasn't been assigned to a selectable.
	 *	Assumes that there is at least one spare ID available
	 *	
	 *	@param Selectable - selectable to generate ID for 
	 */
	uint8 GenerateSelectableID(AActor * Selectable);

protected:

	/**
	 *	[Server] Allow a selectable's selectable ID to be used again.
	 *	
	 *	@param Selectable - the selectable whose ID we are making available again 
	 */
	void FreeUpSelectableID(ISelectable * Selectable);

public:

	/* [Client] Put a selectables ID into the ID map */
	void Client_RegisterSelectableID(uint8 SelectableID, AActor * Selectable);

	/** 
	 *	Given a building type return whether we have built enough of them that we are not 
	 *	allowed to build anymore 
	 *	
	 *	@param bIncludeQueuedSelectables - whether to count the selectables that are in production 
	 *	queues. 
	 */
	bool IsAtUniqueBuildingCap(EBuildingType BuildingType, bool bIncludeQueuedSelectables, 
		bool bTryShowHUDMessage);
	bool IsAtUniqueUnitCap(EUnitType UnitType, bool bIncludeQueuedSelectables, 
		bool bTryShowHUDMessage);
	
	/* Version that takes a training info struct and will then call either IsAtUniqueBuildingCap 
	or IsAtUniqueUnitCap */
	bool IsAtUniqueSelectableCap(const FTrainingInfo & TrainingInfo, bool bIncludeQueuedSelectables,
		bool bTryShowHUDMessage);

	/** 
	 *	Checks if another player state is allied with this one
	 *	
	 *	@return - true if they are allied with this player 
	 */
	bool IsAllied(const ARTSPlayerState * Other) const;

	/**
	 *	Returns whether this player can be targeted by an ability 
	 *	
	 *	@param AbilityInfo - the ability info struct of the ability we are trying to target this 
	 *	player with 
	 *	@param AbilityInstigator - player trying to use ability on this
	 *	@return - None if yes, otherwise a reason why not
	 */
	EGameWarning CanBeTargetedByAbility(const FCommanderAbilityInfo & AbilityInfo,
		ARTSPlayerState * AbilityInstigator) const;

	/* [Server] Command location is a location that could be used if figuring out the direction 
	an aircraft should fly for a commander ability e.g. fuel air bomb in C&C generals */
	FVector GetCommandLocation() const;

	void SetupProductionCapableBuildingsMap();

	/** 
	 *	Get info about all the units garrisoned inside a garrison network for this player
	 *	
	 *	@param GarrisonNetworkType - garrison network to get info for 
	 */
	FBuildingNetworkState * GetBuildingGarrisonNetworkInfo(EBuildingNetworkType GarrisonNetworkType);


	//---------------------------------------------------------------
	//	Audio
	//---------------------------------------------------------------

protected:

	/**
	 *	Returns true if a sound should be played for a building that has just been fully
	 *	constructed 
	 *	
	 *	@param BuildingInfo - building info struct of the building that was just built 
	 *	@param CreationMethod - whether this is a starting selectable (a selectable the player 
	 *	started the match with) 
	 *	@return - true if a sound should be played
	 */
	bool ShouldPlayBuildingBuiltSound_ConstructionComplete(const FBuildingInfo & BuildingInfo, ESelectableCreationMethod CreationMethod);

	// Plays the 'building just built' sound through BuildingBuiltSoundComp 
	void PlayBuildingBuiltSound(USoundBase * SoundToPlay);

	/**
	 *	Returns true if this player state should play the 'unit just built' sound for a unit 
	 *	
	 *	@param UnitJustBuild - the type of unit that was just built 
	 *	@param UnitInfo - unit info struct of UnitJustBuilt for convenience 
	 *	@param bIsStartingSelectable - whether this is a starting selectable (a selectable the player 
	 *	started the match with)
	 *	@return - true if a sound should be played
	 */
	virtual bool ShouldPlayUnitBuiltSound(EUnitType UnitJustBuilt, const FBuildInfo & UnitInfo, 
		ESelectableCreationMethod CreationMethod);

	/* Plays the 'unit just built' sound through the UnitBuiltSoundComp */
	void PlayUnitBuiltSound(USoundBase * SoundToPlay);


	//----------------------------------------------------------
	//	Player Defeated Functions
	//----------------------------------------------------------

public:

	/* Called when a notification comes through from server that this player has been defeated */
	void OnDefeated();

	//------------------------------------------------
	//	Getters and setters
	//------------------------------------------------

	AFactionInfo * GetFI() const;
	void SetFactionInfo(AFactionInfo * NewInfo);

	ARTSPlayerController * GetPC() const;
	void SetPC(ARTSPlayerController * PlayerController);

	/* Returns the CPU player AI controller, or null if this player state belongs to a human player */
	ACPUPlayerAIController * GetAIController() const;
	void SetAIController(ACPUPlayerAIController * InAIController);

	ARTSGameState * GetGS() const;
	void SetGS(ARTSGameState * GameState);
	void SetGI(URTSGameInstance * GameInstance);

	uint8 GetBuildingIndex(EBuildingType Type) const;

	uint8 GetPlayerIDAsInt() const;
	FName GetPlayerID() const;

	ETeam GetTeam() const;
	void SetTeam(ETeam NewValue);

	// Affiliation towards local player
	EAffiliation GetAffiliation() const;
	void SetAffiliation(EAffiliation InAffiliation);

	const FName & GetTeamTag() const;
	void SetTeamTag(const FName & InTag);

	EFaction GetFaction() const;

	// Values < 0 imply no spot assigned
	int16 GetStartingSpot() const;

	// CPU player states only
	void AICon_SetFinalStartingSpot(int16 InStartingSpot);

	const TArray < ABuilding * > & GetBuildings() const;
	const TArray <AInfantry *> & GetUnits() const;
	TMap <EBuildingType, int32> & GetPrereqInfo();

	AUpgradeManager * GetUpgradeManager() const;

	/* Get ID for selectable */
	AActor * GetSelectableFromID(uint8 SelectableID) const;

	/* Get the image that represents this player
	TODO create a variable for this and return it here instead of null, and maybe this 
	could belong somewhere different such as on the UPlayer class instead */
	const FSlateBrush * GetPlayerNormalImage();
	const FSlateBrush * GetPlayerHoveredImage();
	const FSlateBrush * GetPlayerPressedImage();

	URTSHUD * GetHUDWidget() const;

protected:

#if WITH_EDITOR
	/* Assigns a engine image to the player's normal, hovered and pressed brushes */
	void AssignPlayerImages();
#endif
};
