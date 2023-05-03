// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Statics/CommonEnums.h"
//#include "Statics/OtherEnums.h"
#include "CPUPlayerAIController.generated.h"

class ARTSPlayerState;
class ARTSGameState;
class URTSGameInstance;
class AFactionInfo;
class ABuilding;
class AInfantry;
class AResourceSpot;
struct FBuildingInfo;
class ACPUPlayerAIController;
struct FProductionQueue;
struct FTrainingInfo;
struct FCPUPlayerTrainingInfo;
struct FUnitInfo;

#define MINUTE 60.f

/* 
	Diagram of how pending command incrementing/decrementing works

	Producing building with a building or unit:
	  
	                                 Functions can call 
									 this directly too. 
									 Don't have to have 
									 saved up or waited 
									 on full queue
	                                      |
										  |
	  +1               -1                 |
	SaveUp ------------------                  +1
	                        |        If BuildsInTab type building:        From here on out 
							|------>	Start producing            -----> no more changes
          +1           -1   |        else:
	Wait for queue   --------			Try find spot
	to become unfull



	Producing unit/upgrade with building:                             
	
									Functions can call
									this directly too.
									Don't have to have
									saved up or waited
									on full queue
											|
											|
											|
	  +1               -1					|
	SaveUp ------------------				|
	                        |                 +1                   From here on out 
							|------> Actually issue command -----> no more changes
          +1           -1   |       
	Wait for queue   --------		
	to become unfull						


	Diagrams not complete: at some point need to do final decrement
*/


/* My notes: a few TWeakObjectPtr in this file that are also UPROPERTY. Do weak ptrs even need 
the UPROPERTY macro above them to function properly? */


/*
 Look over this file and the tick manager file to see if eveyrthing seems ok
 
 List of things todo for this file:
 - Move the tick manager to its own file
 - Make sure to populate its AIControllers array with nulls 
 - Make sure to change its fixed allocator to ProjectSettings::MAX_NUM_PLAYERS
 - For tick func replace 1337 with MAXNUMPLAYERS

 AICOn:
 AquiredResourceSpotS:
 Make sure to update this when resource depots are destroyed/created
 Make sure when a unit/building is destroyed then it removes itself from the ????? ( assumed 
 AquiredResourceSpotS)
 - In DecideIfShouldBuildCollector possibly add logic that checks if resource spot is under 
 attack to prevent a human player farming the same building over and over
 - Make sure pending command fields are incremented/decremented when needed
 - Will need to check for the QueueWaitInfo also during tick, probably right before or after 
 the savings info is checked
 - PS::ProductionCapableBuildings could be very useful for choosing buildings to build things from
 - PS::GetNumSupportedProducers may be useful but can also just iterate producitonCapableBuildings 
 right away
 - Make sure the TSet UnitsWithPendingCommands gets entries taken/removed from it. Would have to 
 remove in the AInfantryController when it switches to idle or something
 - Added another tag to the actor tag sysme. Will need to update all selectables tag populating 
 code to add it too. Could actually wrap an PS->bIsABot check around adding them since only 
 CPU players need them
 - AInfo deriving classes do not tick by default. May have to change that in ctor of 
 tick manager
 - FSavingUpForInfo::HasBuilderForIt may want to check if queue is unfull. Even though currently 
 we don't build anything while trying to save it could be added in future
 - Need to make sure starting selectables count towards things like num depots and army strength
 - NumCollectors needs to be incremented/decremented whenever a collector is assigned to 
 spot or leaves/destroyed
 - Does not take into account the persistent queue on PS
 - Make sure when a building completes production that it lets the AI controller know
 - When deciding whether a building or unit should build a build need to consider its 
 build method because units wont be able to build some and vice versa
 - Make sure whenever a construction yard type building is destroyed that it removes itself 
 from the BuildsInTabBuildingInfo struct
 - THe first two 'decide' funcs for DoBehavior might want to check if already saving or trying 
 to queue a collector/depot and if so then just do nothing, or add that logic first in thte 
 DoBehavior func before those 2 decide funcs
 - Need to think about LayFoundATLoc and prtoss build methods cause they will not actually 
 place building until later on when unit is at loc 
 - make sure the COmmandReasons for making depots has the resource type as the auxillery info
 - need to take into account selectable cap. At least what should be done about this: 
 --- Add an PS::IsAtSelectableCap assert anytime we check all the stuff like resources, prereqs etc 
 before we build something. 
 --- The funcs like DecideIfShouldBuildArmyUnit can quick return if player is at cap. Can possibly 
 have that check done only once right before all the selectable building funcs like 
 DecideIfShouldBuildX run
 - Note in case I implement them in future: need to take the unique building/unit limits + 
 population cap into account aswell
 - Resource consumption? Especially housing resource consumption. Have not added I think any code 
 for housing resources

 ----------------------------------
 - Starting selectables need to be taken into account
 - Need to add way to stop infinite recursiion for the recursveTryBuild funcs. Could use a TSet 
 that saves history of what has been requested, then Contains check it, and reset it on return
 -----------------------------------

 ##########################################################
 - QueueWereWaitingToFinish needs to incrment PendingCOmmands actually all 5 funcs will. 
 ###################################################### 

 - in PC need to check if BuildsInTab and BuildsItself buildings are destroyed when their 
 construction yard is destroyed if they have not fully built

 - Statics::IsBuildableLocation is thinking buildings with extents != 32 are inside fog

 - Review: ISelectable::GetCOntextMenu maybe can be moved to be a ABuilding func instead
 - Constructing FContextButton for each HasButton call could be costly especially since 
 FContextButton::operator== checks 4 vars. Consider developing quicker comparison method. 
 PS::ProductionCapableBuildings also relies on FContextButton as key
 - Need to create a master func to check if a queue can be used for something, or a func that 
 finds the queue with the smallest wait time
 
 */

//=============================================================================================
//	Enums
//=============================================================================================

/* The state the CPU players infastructure and economy is in */
UENUM()
enum class ECPUPlayerState : uint8
{
	None,

	/* Can build buildings and gather resources */
	Normal,

	/* Cannot construct buildings anymore e.g. construction yard gone. A long victory may still
	be possible but perhaps CPU player will try to win in the short to medium term */
	InfastructureUnrecoverable,

	/* Cannot gather resources anymore. CPU player will play to win in the short-term */
	EconomyUnrecoverable,

	/* Cannot construct buildings and build any army units. CPU player will play to win in the
	very short-term */
	InfastructureUnrecoverableAndArmyProductionImpossible
};


USTRUCT()
struct FUint8Set
{
	GENERATED_BODY()

protected:
	
	TSet <uint8> Container;

public:

	void Emplace(uint8 InValue) { Container.Emplace(InValue); }
	int32 Num() const { return Container.Num(); }
};


/** 
 *	Basic struct that holds the reason a macro command is being issued. 
 *
 *	Contains two parts: info about the command and info about the original command that triggered 
 *	this command.
 *	e.g. if we want to build a battlecruiser to strengthen our army but we don't have a starport 
 *	and we don't have and SCV then we will need to train an SCV. The reason that goes along with 
 *	training that SCV will be: 
 *	PrimaryReason = EMacroCommandSecondaryType::TrainUnit
 *	AuxilleryInfo = SCV
 *	OriginalCommand_PrimaryReason = EMacroCommandType::TrainArmyUnit
 *	OriginalCommand_AuxilleryInfo = EUnitType::Battlecruiser
 */
USTRUCT()
struct FMacroCommandReason
{
	GENERATED_BODY()

protected:

	/* What the command will actually do */
	EMacroCommandSecondaryType ActualCommand;

	/* Additional info about the reason for the command. Always just use a straight static_cast 
	to assign this value */
	uint8 AuxilleryInfo;

	/* Original command: primary reason */
	EMacroCommandType OriginalCommand_PrimaryReason;

	/* Original command: auxillery info */
	uint8 OriginalCommand_AuxilleryInfo;

public:

	// Never call this
	FMacroCommandReason();

	// Ctor for command that is not a chained command
	explicit FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, EBuildingType InBuildingType,
		EMacroCommandType InReason, EResourceType InResourceType);
	explicit FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, EUnitType InUnitType,
		EMacroCommandType InReason, EResourceType InResourceType); 
	explicit FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, EUnitType InUnitType,
		EMacroCommandType InReason);
	explicit FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, EUpgradeType InUpgrade, 
		EMacroCommandType InReason);

	/* Ctors for calling RecursiveTryBuild inside a decide function */
	explicit FMacroCommandReason(EMacroCommandSecondaryType InCommand, EBuildingType InBuildingType,
		EMacroCommandType InCommandReason, EBuildingType InBuildingForCommandReason);
	explicit FMacroCommandReason(EMacroCommandSecondaryType InCommand, EUnitType InUnitType, 
		EMacroCommandType InCommandReason, EBuildingType InBuildingForCommandReason);

	// Ctors for auxillery info too
	explicit FMacroCommandReason(EMacroCommandSecondaryType InReason, EResourceType InResourceType,
		const FMacroCommandReason & OriginalCommand);
	explicit FMacroCommandReason(EMacroCommandSecondaryType InReason, EBuildingType InBuildingType,
		const FMacroCommandReason & OriginalCommand);
	explicit FMacroCommandReason(EMacroCommandSecondaryType InReason, EUnitType InUnitType,
		const FMacroCommandReason & OriginalCommand);
	// Added this ctor after implementing upgrades as prerequisites. Hope it's right
	explicit FMacroCommandReason(EMacroCommandSecondaryType InReason, EUpgradeType InUpgradeType, 
		const FMacroCommandReason & OriginalCommand);

	explicit FMacroCommandReason(const FCPUPlayerTrainingInfo & InInfo);

	/* Get the immediate command, not the big picture command */
	EMacroCommandSecondaryType GetActualCommand() const;
	
	/* Get the auxillery reason as a uint8 */
	uint8 GetRawAuxilleryReason() const;

	EBuildingType GetBuildingType() const;
	EUnitType GetUnitType() const;
	EUpgradeType GetUpgradeType() const;

	/* Should only call this if reason is to either train collector or build resource depot */
	EResourceType OriginalCommand_GetResourceType() const;

	EMacroCommandType GetOriginalCommandMainReason() const;
	uint8 GetOriginalCommandAuxilleryData() const;
};


//=============================================================================================
//	On Class Structs
//=============================================================================================
//	These structs only appear on the AI controller. Perhaps they should be defined as nested 
//	structs instead of outside AI controller class body. But anyway they are in the form of 
//	structs simply to easily identify which variables are relevant to what.

/** 
 *	Info about whether the player is saving up resources for something and if they are what they 
 *	are saving for and who will produce it 
 */
USTRUCT()
struct FSavingUpForInfo
{
	GENERATED_BODY()

protected:

	/* If false then we're not saving up for anything */
	uint32 bSavingUp : 1;

	/** 
	 *	True if the building is being built to serve as a resource depot. Only up to date if 
	 *	saving up for a building 
	 */
	uint32 bSavingForResourceDepot : 1;

	/* If saving for a resource depot the resource spot to build depot near */
	UPROPERTY()
	TWeakObjectPtr < AResourceSpot > TargetResourceSpot;

	EBuildingType Building;
	EUnitType Unit;
	EUpgradeType Upgrade;

	/* The thing that will produce what we're saving up for. Can be left null at which point 
	we may have to try and find one when the time comes when player has enough resources and 
	wants to build it */
	UPROPERTY()
	TWeakObjectPtr < ABuilding > ProducerBuilding;
	UPROPERTY()
	TWeakObjectPtr < AInfantry > ProducerWorker;

	/* The reason we want to build what we are saving up for */
	FMacroCommandReason CommandReason;

public:

	FSavingUpForInfo();

	/** 
	 *	Set what the player should save up for 
	 *	@param BuildingType - building player should save up for 
	 *	@param InProducerBuilding - the construction yard we have chosen to build the building 
	 *	@param ResourceSpot - resource spot to build resource depot near 
	 */
	void SetSavingsTarget(EBuildingType BuildingType, ABuilding * InProducerBuilding, 
		AResourceSpot * ResourceSpot);
	void SetSavingsTarget(EBuildingType BuildingType, AInfantry * InWorker,
		AResourceSpot * ResourceSpot);
	
	/* Versions for building non-depot buildings */
	void SetSavingsTarget(EBuildingType BuildingType, ABuilding * InProducerBuilding,
		FMacroCommandReason InCommandReason);
	void SetSavingsTarget(EBuildingType BuildingType, AInfantry * InWorker,
		FMacroCommandReason InCommandReason);
	
	/* Version for building units */
	void SetSavingsTarget(EUnitType UnitType, ABuilding * InBarracks, FMacroCommandReason InCommandReason);
	
	/* Version for building upgrades */
	void SetSavingsTarget(EUpgradeType UpgradeType, ABuilding * InProducerBuilding, 
		FMacroCommandReason InCommandReason);

	/** 
	 *	Return whether the player is saving up for something or not 
	 */
	bool IsSavingUp() const;

	/** Return whether the thing we're saving up for is a building or not (can be depot) */
	bool IsForBuilding() const;
	
	/** Return whether what we're saving up for is a unit or not */
	bool IsForUnit() const;
	
	/** Return whether what we're saving up for is an upgrade or not */
	bool IsForUpgrade() const;

	/** Check if player has enough resources for what they're saving up for */
	bool HasEnoughResourcesForIt(ACPUPlayerAIController * AICon) const;
	
	/** 
	 *	Return whether prereqs are met for what we're saving for 
	 *
	 *	@param AICon - CPU player AI controller that owns this struct
	 *	@param OutMissingPrereq - if return false then the first prereq we're missing 
	 *	@return - true if prereqs are met
	 */
	bool ArePrerequisitesMetForIt(ACPUPlayerAIController * AICon, 
		EBuildingType & OutMissingPrereq_Building, EUpgradeType & OutMissingPrereq_Upgrade) const;

	/** 
	 *	Return whether the thing we're saving up has a worker/building that can produce it 
	 *	on the field 
	 *
	 *	@param AICon - AI controller that owns this struct
	 *	@param bTryFindIfNone - if the cached producer is no longer valid/alive whether we should 
	 *	search the player's selectables looking for another usuable producer
	 *	@return - true if a producer can produce what we're saving up for
	 */
	bool HasBuilderForIt(ACPUPlayerAIController * AICon, bool bTryFindIfNone);

	/* Build what the player is saving up for. Assumed that if calling this then resources and 
	prereqs are good */
	void BuildIt(ACPUPlayerAIController * AICon);
};


/**
 *	Info that says what production queue the player is waiting to become un-full so they 
 *	can add an item to it. This is kind of similar to FSavingUpForInfo.
 */
USTRUCT()
struct FQueueWaitingInfo
{
	GENERATED_BODY()

protected:

	bool bWaiting;

	/* Building that owns the queue */
	UPROPERTY()
	TWeakObjectPtr < ABuilding > QueueOwner;

	/* Building's queue we're waiting on */
	const FProductionQueue * Queue;

	/* What we will put in the queue. Will be one of these 3 */
	EBuildingType Building;
	EUnitType Unit;
	EUpgradeType Upgrade;

	/* Reason why we want to produce what we want to produce */
	FMacroCommandReason CommandReason;

	bool IsForBuilding() const;
	bool IsForUnit() const;
	bool IsForUpgrade() const;

public:

	FQueueWaitingInfo();

	/** Set the queue we are waiting on. Set the item we will build when queue becomes unfull */
	void WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
		EBuildingType WhatWeWantToBuild, FMacroCommandReason InCommandReason);
	void WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue,
		EUnitType WhatWeWantToBuild, FMacroCommandReason InCommandReason);
	void WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue,
		EUpgradeType WhatWeWantToBuild, FMacroCommandReason InCommandReason);

	/* Return whether the player is waiting on a queue */
	bool IsWaiting() const;

	/* Return whether the queue owner is still valid and alive */
	bool IsQueueOwnerValidAndAlive() const;

	/* Return whether the queue now has a slot free */
	bool IsQueueNowUnFull() const;

	/* Return prereqs are met for what player is waiting for. Assumes is waiting for something */
	bool ArePrerequisitesMetForIt(ACPUPlayerAIController * AICon, EBuildingType & OutMissingPrereq_Building, 
		EUpgradeType & OutMissingPrereq_Upgrade) const;
	
	/* Return whether player has enough resources for what they are waiting for. Assumes is 
	waiting for something */
	bool HasEnoughResourcesForIt(ACPUPlayerAIController * AICon) const;

	void BuyIt();

	/* Stop waiting on queue */
	void CancelWaiting();
};


/** 
 *	Info about status of trying to place a building (depots and non-depots). We limit the number 
 *	of sweeps the player can do each tick, meaning sometimes it can take multiple ticks to find 
 *	a suitable location to place a building
 */
USTRUCT()
struct FTryingToPlaceBuildingInfo
{
	GENERATED_BODY()

public:

	//-------------------------------------------------
	//	Constructor
	//-------------------------------------------------

	FTryingToPlaceBuildingInfo();

protected:

	/* The type of building we're trying to build. This will be flagged as 
	EBuildingType::NotBuilding if not trying to place a building */
	EBuildingType PendingBuilding;
	
	/* True if we want to place PendingBuilding as a resource depot */
	bool bIsForDepot;

	/* If placing a resource depot, the resource spot to place depot near */
	UPROPERTY()
	TWeakObjectPtr < AResourceSpot > ResourceSpot;

	/* Whether to use ProducerBuilding or ProducerInfantry. True means use building */
	bool bUsingConstructionYard;

	/* The construction yard that will build this building */
	UPROPERTY()
	TWeakObjectPtr < ABuilding > ProducerBuilding;

	/* The worker unit that will build this building */
	UPROPERTY()
	TWeakObjectPtr < AInfantry > ProducerInfantry;

	/* The reason why we want to build the building */
	FMacroCommandReason CommandReason;

	int32 NumBuildLocationsTriedThisTick;

	//=======================================================================================
	//	Resource Depot specific variables
	//=======================================================================================
	
	int32 NumResourceDepotSpotsTried;
	int32 ResourceDepotBuildLocsIndex;
	
	//=======================================================================================
	//	Non-Resource depot specific variables
	//=======================================================================================

	int32 NumGeneralAreasExhausted;
	int32 BuildLocMultipliersIndex;
	int32 NumLocationsTriedForCurrentGeneralArea;
	FVector GeneralArea;

public:

	/* Version of BeingForBuilding that is intended to be used to try and place a building 
	intended to be used as a resource depot */
	void BeginForResourceDepot(EBuildingType InBuildingType, ABuilding * InBuildingsProducer, 
		AResourceSpot * InResourceSpot, const FMacroCommandReason & InCommandReason);
	void BeginForResourceDepot(EBuildingType InBuildingType, AInfantry * InBuildingsProducer, 
		AResourceSpot * InResourceSpot, const FMacroCommandReason & InCommandReason);

	/* Set what the building is that player should try and place. Resets all general area 
	info and starts at blank slate. Building will try to be placed on next behavior tick */
	void BeginForBuilding(ACPUPlayerAIController * AICon, EBuildingType InBuildingType, 
		ABuilding * InBuildingsProducer, const FMacroCommandReason & InCommandReason);
	void BeginForBuilding(ACPUPlayerAIController * AICon, EBuildingType InBuildingType,
		AInfantry * InBuildingsProducer, const FMacroCommandReason & InCommandReason);

	/* Return true if trying to place a building, either a depot or regular building */
	bool IsPending() const;

	// Assumes actually trying to place a building
	EBuildingType GetBuilding() const;

	int32 GetResourceDepotBuildLocsIndex() const;
	int32 GetBuildLocMultipliersIndex() const;
	int32 GetNumGeneralAreasExhausted() const;
	FVector GetCurrentGeneralArea() const;

	/* Get the rotation to try when trying to place building */
	FRotator GetRotation(ACPUPlayerAIController * AICon) const;

	/* 
	 *	Continue trying to place building during tick 
	 *
	 *	@param AICon - CPU player AI controller that this struct belongs to 
	 *	@return - true if a command is given for a building to be placed 
	 */
	bool TryPlaceForTick(ACPUPlayerAIController * AICon);

	/* Return whether either ProducerBuilding or ProducerInfantry are able to build what we 
	are trying to place */
	bool IsProducerUsable() const;

	/** 
	 *	Give up trying to place building. If it's a BuildsInTab build method building then we 
	 *	will need to cancel its production 
	 *	
	 *	@return - building we gave up trying to place 
	 */
	EBuildingType GiveUp(ACPUPlayerAIController * AICon);
};


/** 
 *	@See FBuildsInTabBuildingInfo
 *
 *	A single entry. There's one for each queue producing a building with the BuildsInTab
 *	method 
 */
USTRUCT()
struct FSingleBuildsInTabInfo
{
	GENERATED_BODY()

protected:

	// Building we're building. NotBuilding if not building anything
	EBuildingType Building;

	// Whether Building is being built to serve as a resource depot or not
	bool bForResourceDepot;

	/* The building that builds */
	UPROPERTY()
	TWeakObjectPtr < ABuilding > ConstructionYard;

	/* The resource spot to build depot near if building a depot */
	UPROPERTY()
	TWeakObjectPtr < AResourceSpot > ResourceSpotForDepot;

#if !UE_BUILD_SHIPPING
	/* Queue that is producing building */
	const FProductionQueue * Queue;
#endif

	/* If true then the production has completed and building will be placed on next tick */
	bool bProductionComplete;

	/* Reason why we want to build this building */
	FMacroCommandReason CommandReason;

public:

	// Never call this
	FSingleBuildsInTabInfo();

	/* Resource depot ctor */
	FSingleBuildsInTabInfo(EBuildingType BuildingToBuild, ABuilding * ProducerBuilding,
		AResourceSpot * ResourceSpot, const FProductionQueue * InQueue);
	/* Regular building ctor */
	FSingleBuildsInTabInfo(EBuildingType BuildingToBuild, ABuilding * ProducerBuilding, 
		FMacroCommandReason InCommandReason, const FProductionQueue * InQueue);

	/* Get the producer building that this entry is for */
	TWeakObjectPtr < ABuilding > GetConstructionYard() const;

#if !UE_BUILD_SHIPPING
	const FProductionQueue * GetQueue() const;
#endif

	/* Called when production for the queue has complete. The param InQueue is there just to
	verify the queue is the correct one */
	void FlagProductionAsComplete();

	/* If the construction yard is no longer around or we explicity wanted to cancel production
	then this should be called */
	void FlagProductionAsCancelled();

	/* Return whether production has completed and therefore we can try place building */
	bool IsProductionComplete() const;

	/**
	 *	When production has complete now signal that we should try find a buildable location
	 *	to place building
	 *
	 *	@param AICon - AI controller this struct belongs to
	 *	@param TryPlaceBuildingInfo - AICon's FTryingToPlaceBuildingInfo struct for convenience
	 */
	void NowTryPlaceEachTick(ACPUPlayerAIController * AICon, FTryingToPlaceBuildingInfo & TryPlaceBuildingInfo);
};


/** 
 *	Info about buildings we are building that uses the BuildsInTab build method. 
 */
USTRUCT()
struct FBuildsInTabBuildingInfo
{
	GENERATED_BODY()

protected:

	/* Array of all BuildsInTab production by this player. Both in progress and ready to place 
	info will be in here */
	UPROPERTY()
	TArray < FSingleBuildsInTabInfo > Array;

public:

	/* Note down that we are building this building in tab. Resource depot version */
	void NoteAsProducing(EBuildingType BuildingToBuild, ABuilding * ProducerBuilding, 
		AResourceSpot * ResourceSpot, const FProductionQueue * InQueue);
	/* Note down that we are building this building in tab. Regular building version */
	void NoteAsProducing(EBuildingType BuildingToBuild, ABuilding * ProducerBuilding, 
		FMacroCommandReason CommandReason, const FProductionQueue * InQueue);

	/* Return whether player is currently building something with BuildsInTab build method. 
	Either a still in progress or a complete building will return true */
	bool IsBuildingSomething() const;

	/* @param InQueue - the queue that completed production */
	void FlagProductionAsComplete(ABuilding * ConstructionYard, const FProductionQueue * InQueue);

	/** 
	 *	Check if any buildings have completed production and are now ready to try and be placed. 
	 *	Max one building each call. It is assumed that if calling this then the player isn't already 
	 *	trying to place a building each tick
	 *
	 *	@return - true if a building is now trying to be placed each tick 
	 */
	bool NowTryPlaceEachTick(ACPUPlayerAIController * AICon, FTryingToPlaceBuildingInfo & TryingToPlaceBuildingInfo);
};


//=============================================================================================
//	CPU Player AI Controller Implementation
//=============================================================================================

/**
 *	Terminology
 *
 *	Unit types:
 *	A unit can be more than one type
 *	Worker: a unit that can produce at least one building because it has a BuildBuilding button 
 *	in its context menu
 *	Collector: a unit that can gather at least one resource 
 *	Attacking/Army: a unit that is not a worker or a collector
 */


/**
 *	AI controller for a CPU player. 
 *
 *	This class's performance will be more affected by the size of faction info and the number of 
 *	selectables this player controls than other code
 *
 *	Would like to make these more event driven instead of querying state each tick but it is 
 *	much more work than I thought 
 *
 *	Perhaps make this a base class and have two different AI controllers: 
 *	- A general AI controller that isn't specific to any type of faction. It is the most general 
 *	and will be the hardest to program
 *	- An AI controller that has a fixed build order defined for each faction
 */
UCLASS()
class RTS_VER2_API ACPUPlayerAIController : public AAIController
{
	GENERATED_BODY()

	friend struct FSavingUpForInfo;
	friend struct FQueueWaitingInfo;
	friend struct FSingleBuildsInTabInfo;
	friend struct FTryingToPlaceBuildingInfo;

public:

	ACPUPlayerAIController();

	/**
	 *	Set values before behavior is started 
	 *
	 *	@param InIDAsInt - unique ID of player
	 *	@param InPlayerID - unique ID of player as FName
	 *	@param InTeam - the team this player is on 
	 *	@param InFaction - the faction this player will be playing as 
	 *	@param InStartingSpot - the starting spot before GI::AssignOptimalStartinSpots is called. 
	 *	Therefore this can change and is not final
	 *	@param StartingResources - resources player starts the match with
	 *	@param InDifficulty - the difficulty of this player
	 */
	void Setup(uint8 InIDAsInt, FName InPlayerID, ETeam InTeam, EFaction InFaction, int16 InStartingSpot, 
		const TArray <int32> & StartingResources, ECPUDifficulty InDifficulty);

	/** 
	 *	Set what the default rotation is for this player given a starting spot 
	 *	
	 *	@param InStartingSpotID - the startin spot ID of where this player starts the match at 
	 */
	void SetInitialLocAndRot(int16 InStartingSpotID);

	/** 
	 *	Called right before behavior starts. Notifies the AI controller what the selectables 
	 *	it has started the game with are 
	 *	
	 *	@param StartingBuildings - the buildings it has started the game with. One entry for each 
	 *	building 
	 *	@param StartingUnits - the units it has started the game with. One entry for each unit 
	 */
	void NotifyOfStartingSelectables(const TArray < EBuildingType > & StartingBuildings,
		const TArray < EUnitType > & StartingUnits);

	/* Called right after NotifyOfStartingSelectables. */
	void PerformFinalSetup();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry * Snapshot) const override;
#endif // ENABLE_VISUAL_LOG

	/** 
	 *	This is this class's tick. It is called from a tick manager. This is where all the 
	 *	behavior decisions are made 
	 */
	void TickFromManager();

protected:

	//==========================================================================================
	//	Framework Variables
	//==========================================================================================

	/* Difficulty of this player */
	ECPUDifficulty Difficulty;

	/* Reference to player state */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to player states faction info */
	UPROPERTY()
	AFactionInfo * FI;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* The world coords of where we started for match. If this information ever needs to be 
	moved to PS then make sure to remove this variable from here */
	FVector StartingSpotLocation;

	/* The yaw rotation the player should start the match at. It will use this when trying to 
	place buildings */
	float StartingViewYaw;


	//=========================================================================================
	//	State Variables
	//=========================================================================================

	/* Info about a building we're building that uses the BuildsInTab build method */
	FBuildsInTabBuildingInfo BuildsInTabBuildingInfo;

	/* Info about the building we're trying to place. Only needed because of self-imposed
	limit on number of sweeps allowed per tick */
	FTryingToPlaceBuildingInfo TryingToPlaceBuildingInfo;

	/* Info about what we're saving up for */
	FSavingUpForInfo ThingSavingUpFor;

	/* Holds something we want to build + references a queue we want to build it from. 
	This is for when player wants to build something but the queue is full */
	FQueueWaitingInfo QueueWereWaitingToFinish;

	UPROPERTY()
	TArray < TWeakObjectPtr < AInfantry > > IdleCollectors;

	UPROPERTY()
	TArray < TWeakObjectPtr < AInfantry > > IdleWorkers;

	/* Set of resource spots that are counted as 'aquired'. Even if the depot is not fully 
	constructed it will add an entry to this */
	UPROPERTY()
	TSet < TWeakObjectPtr < AResourceSpot > > AquiredResourceSpots;

	/* Maps resource type to the number of resource spots that have been aquired for that 
	resource type */
	UPROPERTY()
	TMap < EResourceType, int16 > NumAquiredResourceSpots;

	/* Maps each resource spot to the selectable IDs of the collectors that are collecing 
	from it */
	UPROPERTY()
	TMap < AResourceSpot *, FUint8Set > NumCollectors;

	/* Sets of what has been tried by the current RecursiveTryBuild functions. These are here 
	to avoid an infinite recursion situation */
	TSet < EBuildingType > TriedRecursiveBuildBuildings;
	TSet < EUnitType > TriedRecursiveBuildUnits;

	/* A worker is a unit that can build at least one building */
	int32 NumWorkers;

	/* Barracks is a building that can train an attacking type unit. 
	Don't know what this counts exactly: either number of barracks or number of buildings built 
	for the purpose of training attacking type units */
	int32 NumBarracks;

	/* Number of base defense buildings the player owns */
	int32 NumBaseDefenses;

	/* Strength of army. Units only, not base defenses */
	int32 ArmyStrength;

	//------------------------------------------------------------
	//	State: Pending Commands
	//------------------------------------------------------------
	// These containers contain commands that have been issued

	/* I am using this to quickly identify which units are idle during the pretick phase */
	UPROPERTY()
	TSet < AActor * > UnitsWithPendingCommands;

	// Maps building type to how many of them are being produced 
	UPROPERTY()
	TMap < EBuildingType, int16 > PendingCommands_BuildBuilding;

	// Maps unit type to how many of them are being produced
	UPROPERTY()
	TMap < EUnitType, int16 > PendingCommands_TrainUnit;

	// If upgrade is in set then it is being researched
	UPROPERTY()
	TSet < EUpgradeType > PendingCommands_ResearchUpgrade;

	/* How much army strength is expected from the pending commands */
	int32 PendingCommands_ArmyStrength;

	//-------------------------------------------------------------------
	//	State: Command Reasons
	//-------------------------------------------------------------------
	//	Basically if you want to build a battlecruiser but you have no starport but do have an 
	//	SCV then you will issue a command on that SCV to build a starport. Next tick you would 
	//	likely decide again that you want to build a battlecruiser and would give out another 
	//	command to build a starport, but that's what these variables are here for
	//	Every command will have an entry in pending commands and in command reasons

	UPROPERTY()
	TMap < EResourceType, int32 > CommandReasons_BuildResourceDepot;

	UPROPERTY()
	TMap < EResourceType, int32 > CommandReasons_TrainCollector;

	/* Commands that are not build resource depot or train collector */
	UPROPERTY()
	TMap < EMacroCommandType, int32 > CommandReasons_PendingCommands;

	//=========================================================================================
	//	Behavior Functions
	//=========================================================================================

	/* Before deciding commands for tick, iterate over all buildings and units and gather 
	necessary data about them. */
	void PreBehaviorPass();

	/* What is called during each AI tick that handles doing a lot of behavior. It handles 
	deciding if player should take an action and will carry out that action */
	void DoBehavior();

	//=========================================================================================
	//	Decision Making Queries
	//=========================================================================================

	/* Decide if should build a resource collector and if yes then take action towards making 
	it happen */
	bool DecideIfShouldBuildCollector();

	bool DecideIfShouldBuildResourceDepot();

	/* Decide if should build either a worker or a construction yard type building. */
	bool DecideIfShouldBuildInfastructureProducingThing();

	/* Decide if should build a building that can make army units */
	bool DecideIfShouldBuildBarracks();
	
	bool DecideIfShouldResearchUpgrade();

	/* Decide if should build a base defense building */
	bool DecideIfShouldBuildBaseDefense();

	/* Decide if should train a unit to use as an army unit */
	bool DecideIfShouldBuildArmyUnit();


	//=========================================================================================
	//	Command Issuing Funcs
	//=========================================================================================

	/** 
	 *	Tell all idle units capable of collecting resources to go collect resources 
	 */
	void AssignIdleCollectors();

	/**
	 *	Try build a resource depot near a certain resource spot. If not successful initially 
	 *	then try again on next tick
	 *
	 *	@param BuildingType - building to try and place
	 *	@param ConstructionYard - the construction yard that we have checked has its queue empty 
	 *	and will build the building
	 *	@param ResourceSpot - the resource spot we want to build depot near 
	 */
	void TryIssueBuildResourceDepotCommand(EBuildingType BuildingType, ABuilding * ConstructionYard,
		AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason);
	void TryIssueBuildResourceDepotCommand(EBuildingType BuildingType, AInfantry * Worker,
		AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason);

	/** 
	 *	Try and build a building. Assumed all params are legit. The only reason this can fail 
	 *	is if a suitable build location is never found, so make sure to check everything that 
	 *	needs to be checked before calling this like resources, prereqs, queue capacity, etc
	 *
	 *	@param BuildingType - building to try build
	 *	@param ConstructionYard - building that will build the building of type BuildingType
	 *	@param CommandType - the reason the building is being built 
	 */
	void TryIssueBuildBuildingCommand(EBuildingType BuildingType, ABuilding * ConstructionYard, 
		const FMacroCommandReason & CommandReason);
	void TryIssueBuildBuildingCommand(EBuildingType BuildingType, AInfantry * Worker, 
		const FMacroCommandReason & CommandReason);

#if !UE_BUILD_SHIPPING
	/* Do all the checks required before doing a build building command */
	bool PreBuildBuildingCommandChecks(EBuildingType BuildingType, ABuilding * ConstructionYard) const;
	bool PreBuildBuildingCommandChecks(EBuildingType BuildingType, AInfantry * Worker) const;
	
	/* Check everything before issuing a train unit command */
	bool PreTrainUnitCommandChecks(EUnitType UnitType, ABuilding * Barracks) const;

	/* Check everything before issuing a research upgrade command */
	bool PreResearchUpgradeCommandChecks(EUpgradeType UpgradeType, ABuilding * Researcher) const;
#endif

	/** 
	 *	Tell a construction yard to construct a building for the purpose of being used as a 
	 *	resource depot 
	 *	
	 *	@param BuildingType - type of building to construct 
	 *	@param ConstructionYard - construction yard that will construct the depot 
	 *	@param Location - location to build depot 
	 *	@param Rotation - rotation of depot 
	 *	@param CommandReason - the reason for building depot. This 'main reason' for this struct 
	 *	should always be the same however the auxillery info should contain the type of resource 
	 *	we are building the depot for
	 */
	void ActuallyIssueBuildResourceDepotCommand(EBuildingType BuildingType, ABuilding * ConstructionYard,
		const FVector & Location, const FRotator & Rotation, const FMacroCommandReason & CommandReason);
	/** Version that orders a worker to construct a depot */
	void ActuallyIssueBuildResourceDepotCommand(EBuildingType BuildingType, AInfantry * Worker,
		const FVector & Location, const FRotator & Rotation, const FMacroCommandReason & CommandReason);

	/**
	 *	Tell a construction yard to construct a building 
	 *	
	 *	@param BuildingType - type of building to construct 
	 *	@param ConstructionYard - construction yard that will construct the building 
	 *	@param Location - world location where we want to place building 
	 *	@param Rotation - world rotation for building 
	 *	@param CommandReason - the reason we are issuing this command
	 */
	void ActuallyIssueBuildBuildingCommand(EBuildingType BuildingType, ABuilding * ConstructionYard,
		const FVector & Location, const FRotator & Rotation, const FMacroCommandReason & CommandReason);
	/** Version that tells a worker to construct a building */
	void ActuallyIssueBuildBuildingCommand(EBuildingType BuildingType, AInfantry * Worker,
		const FVector & Location, const FRotator & Rotation, const FMacroCommandReason & CommandReason);

	/** 
	 *	Give out a command to train a unit. Assumes that resources, prereqs and barracks are 
	 *	all good 
	 *	
	 *	@param UnitType - the unit we want to train 
	 *	@param Barracks - the building we will train the unit from 
	 *	@param CommandReason - the reason we are training this unit, for behavior purposes
	 */
	void ActuallyIssueTrainUnitCommand(EUnitType UnitType, ABuilding * Barracks, 
		const FMacroCommandReason & CommandReason);

	/**
	 *	Give out command to research an upgrade. Assumes that resources, prereqs and the 
	 *	researching building are all good 
	 *
	 *	@param UpgradeType - the type of upgrade we want to research 
	 *	@param ResearchingBuilding - the building that will research the upgrade 
	 *	@param CommandType - the reason we are researching this upgrade 
	 */
	void ActuallyIssueResearchUpgradeCommand(EUpgradeType UpgradeType, ABuilding * ResearchingBuilding, 
		const FMacroCommandReason & CommandReason);

	/** Tell a unit to collect resources */
	void ActuallyIssueCollectResourcesCommand(AInfantry * Collector, AResourceSpot * ResourceSpot);

	void ActuallyIssueCancelProductionCommand(ABuilding * ConstructionYard);

	//=========================================================================================
	//	Pseudo-Command Issuing Functions
	//=========================================================================================

	/** 
	 *	Set what we should save up our resources for 
	 *
	 *	@param SomethingType - what to save up for
	 *	@param ConstructionYard - building that will build this. Can pass null which means a 
	 *	suitable construction yard will tried to be found when it comes time that player has 
	 *	enough resources.
	 *	@param ResourceSpot - if building is going to be placed as a resource depot the spot to 
	 *	place it next to 
	 */
	void SetSavingsTarget(EBuildingType BuildingType, ABuilding * ConstructionYard, 
		AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason);
	void SetSavingsTarget(EBuildingType BuildingType, AInfantry * Worker, 
		AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason);
	/* Use these non resource spot param versions for placing regular buildings */
	void SetSavingsTarget(EBuildingType BuildingType, ABuilding * ConstructionYard, FMacroCommandReason CommandReason);
	void SetSavingsTarget(EBuildingType BuildingType, AInfantry * Worker, FMacroCommandReason CommandReason);
	void SetSavingsTarget(EUnitType UnitType, ABuilding * Barracks, FMacroCommandReason CommandReason);
	void SetSavingsTarget(EUpgradeType UpgradeType, ABuilding * Researcher, FMacroCommandReason CommandReason);

	void OnSavingsTargetPrereqNotMet(EBuildingType MissingPrereq);
	void OnSavingsTargetPrereqNotMet(EUpgradeType MissingPrereq);

	/** 
	 *	Note down that we are waiting for a full queue to become unfull so we can place 
	 *	something in it 
	 *
	 *	@param InQueueOwner - the building that owns the queue that we will wait on 
	 *	@param TargetQueue - the queue that we will wait on to become unfull 
	 *	@param WhatWeWantToBuild - the building/unit/upgrade we want to produce from queue when it 
	 *	becomes unfull 
	 *	@param CommandReason - the reason we want to produce the building/unit/upgrade 
	 */
	void SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue,
		EBuildingType WhatWeWantToBuild, FMacroCommandReason CommandReason);
	void SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue,
		EUnitType WhatWeWantToBuild, FMacroCommandReason CommandReason);
	void SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue,
		EUpgradeType WhatWeWantToBuild, FMacroCommandReason CommandReason);

	//=========================================================================================
	//	 State Queries
	//=========================================================================================

	// Return whether unit is in a state we consider idle
	bool IsUnitIdle(AActor * Unit) const;

	/* These 3 func below: need to check where I call them and see what exactly I want them 
	to check */

	/* Check to see if a building can be used to produce another building. Assumes that resources 
	and prereqs have been checked and that the building is a construction yard */
	bool CanUseConstructionYard(ABuilding * ConstructionYard) const;

	/** 
	 *	Excluding prerequisites, resources and building's validity, check that a building can be 
	 *	used to train a unit 
	 *	
	 *	@param Barracks - building to test for 
	 *	@param UnitWeWantToBuild - type of unit we want to train
	 *	@return - true if the building can be used to train the requested unit 
	 */
	bool CanTrainFromBuilding(ABuilding * Barracks, EUnitType UnitWeWantToBuild) const;
	bool CanResearchFromBuilding(ABuilding * ResearchLab, EUpgradeType UpgradeType) const;

	/** 
	 *	Excluding prerequisites, resources and validity, check to see if a worker can be used 
	 *	to build a building 
	 *	
	 *	@param Worker - worker to check 
	 *	@param BuildingType - building we want to build 
	 *	@return - true if worker can be used 
	 */
	bool CanUseIdleWorker(AInfantry * Worker, EBuildingType BuildingType) const;
	bool CanUseWorker(AInfantry * Worker, EBuildingType BuildingType) const;

	/* Return true if the player is trying to place a building each tick */
	bool IsTryingToPlaceBuilding() const;
	
	/** 
	 *	Return true if the player is waiting on a production queue to become unfull so it can 
	 *	queue an item up in it 
	 */
	bool IsWaitingForQueueToBecomeUnFull() const;

	/**
	 *	Return true if the player is saving up their resources for something 
	 */
	bool IsSavingUp() const;

	/* State queries that get the number of things we currently have. */
	int32 GetNumAquiredResourceSpots(EResourceType ResourceType) const;
	int32 GetNumConstructionYards() const;
	int32 GetNumWorkers() const;
	int32 GetNumBarracks() const;
	int32 GetNumBaseDefenses() const;
	int32 GetArmyStrength() const;

	/** 
	 *	Getters that drive when the player should build more of something. They do not take into 
	 *	account any pending commands
	 */

	int32 GetExpectedNumDepots(EResourceType ResourceType) const;
	int32 GetExpectedNumConstructionYards() const;
	int32 GetExpectedNumWorkers() const;
	int32 GetExpectedNumBarracks() const;
	int32 GetExpectedNumBaseDefenses() const;
	int32 GetExpectedArmyStrength() const;


	//=========================================================================================
	//	Utility functions
	//=========================================================================================

	/**
	 *	Try and build something. If the thing requested to be built cannot be then the player will 
	 *	build something to work towards building it. 
	 *	e.g. if want to build a viking but have no starport then player will try build starport. 
	 *	If no SCV to build starport then player will try and build an SCV etc hence the recursive 
	 *	part. 
	 *	
	 *	@return - true if an action was taken to moving closer to building what we want to build. 
	 *	@See comment above RecursiveTryBuild
	 */
	bool InitialRecursiveTryBuildWrapper(EBuildingType BuildingType, FMacroCommandReason CommandReason);
	bool InitialRecursiveTryBuildWrapper(EUnitType UnitType, FMacroCommandReason CommandReason);
	bool InitialRecursiveTryBuildWrapper(EUpgradeType UpgradeType, FMacroCommandReason CommandReason);

	/**
	 *	Never call this anywhere other than inside InitialRecursiveTryBuildWrapper
	 *
	 *	Try and issue a command to build this building. If the a prereq is not met then the
	 *	player will try and build that prereq. If prereq of prereq is not met then player will
	 *	try and build that etc hence recursive name. If not enough resources for building then
	 *	player will set it as their savings target. If no building or unit can produce it then
	 *	they will be built
	 *	@param ReasonForProducing - the reason why we want to build it
	 *	@return - true if any of the following happen:
	 *		- a command is issued to produce what we want
	 *		- a command is issued to produce something that will work towards producing what we
	 *		want e.g. a prerequisite, a barracks that trains what we want
	 *		- what we want to produce is set as our savings target. It will be bought later when
	 *		we can afford it
	 *		- what we want to produce is set as a queue waiting target. When a queue becomes unfull
	 *		we will start producing it
	 *
	 *	For buildings currently this can actually return true even if a command isn't given out to
	 *	produce building, only because we never find a buildable location to place building, so
	 *	case 1 and 2 above are not 100% true.
	 */
	bool RecursiveTryBuild(EBuildingType BuildingType, FMacroCommandReason CommandReason);

	bool RecursiveTryBuild(EUnitType UnitType, FMacroCommandReason CommandReason);

	bool RecursiveTryBuild(EUpgradeType UpgradeType, FMacroCommandReason CommandReason);

	/** 
	 *	Given a resource spot return how many collectors should be assigned to it for max resource 
	 *	gathering rate. e.g. in SCII a vespene gyser is 3 but can be larger depending on how far 
	 *	away the closest nexus is.
	 *	This function cannot really be 100% correct though without taking into account if the 
	 *	collectors have different gathering speeds and the distance from the nearest depot 
	 *
	 *	@param ResourceSpot - resource spot we want to know how many collectors is optimal
	 *	@return - optimal number of collectors
	 */
	int32 GetOptimalNumCollectors(AResourceSpot * ResourceSpot) const;

	/* Given a resource spot get the number of collectors that have been assigned to it */
	int32 GetNumCollectorsAssigned(AResourceSpot * ResourceSpot) const;

	//~ Begin variables for trying to find buildable locations for buildings

	/** Number of entries in BuildLocMultipliers */
	static const int32 BUILD_LOC_MULTIPLERS_NUM = 33;
	static_assert(BUILD_LOC_MULTIPLERS_NUM > 0, "FTryingToPlaceBuildingInfo::TryPlaceForTick relies "
		"on this being at least 1");
	
	/** 
	 *	An array of different multipliers to apply to build locations when trying to find a viable
	 *	build location for a building 
	 */
	static const FVector2D BuildLocMultipliers[BUILD_LOC_MULTIPLERS_NUM];

	/* Number of spots to try for resource depots. Basically these will be locations in a circle
	around the resource spot */
	static const int32 NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY = 16;
	static const float CIRCLE_PORTIONS_IN_RADIANS;

	/* Contains unit vectors for different rotations in a circle. Precomputed to perhaps save 
	some time at runtime, but we're only really doing a cos and sin calc and approximate 
	would even be acceptable too */
	static const FVector2D ResourceDepotBuildLocs[NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY];

	/** 
	 *	This variable is only relevent when placing non-resource depot buildings. 
	 *	NUM_GENERAL_AREAS_TO_TRY * BUILD_LOC_MULTIPLERS_NUM = total areas tried per building.
	 *	This is how many general areas to try before 'giving up'. A general area will try 
	 *	BUILD_LOC_MULTIPLERS_NUM many different locations.
	 */
	static const int32 NUM_GENERAL_AREAS_TO_TRY = 5;

	/** 
	 *	The maximum number of build locations that will be tried to be found each tick. This 
	 *	is here to limit the amount of sweeps/raycasts done each tick.
	 */
	static const int32 MAX_NUM_BUILD_LOC_TRIES_PER_TICK = 10;

	//~ End variables for trying to find buildable locations for buildings

	/** 
	 *	Code common to both overloaded functions of TryPlaceResourceDepot. Do not call this 
	 *	function except inside TryPlaceResourceDepot 
	 *	
	 *	@param DepotInfo - building info for the depot we want to build 
	 *	@param ResourceSpot - resource spot we want to build  depot near 
	 *	@param OutLocation - the location to build building at. Only relevant if successful 
	 *	@param OutRotation - 
	 *	@return - true if successful in finding a location to place building 
	 */
	bool TryPlaceResourceDepotInner(const FBuildingInfo & DepotInfo, AResourceSpot * ResourceSpot,
		FVector & OutLocation, FRotator & OutRotation);

	/* 
	 *	Try place a resource depot near a resource spot by checking lots of areas around it.
	 *
	 *	@param DepotInfo - building info for the depot we're trying to build
	 *	@param ResourceSpot - the resource spot to try and build depot near 
	 *	@param bInitialTry - if this is being called during tick then this should be false
	 *	@return - true if a command is issued to build the depot
	 */
	bool TryPlaceResourceDepot(const FBuildingInfo & DepotInfo, AResourceSpot * ResourceSpot,
		ABuilding * ConstructionYard, const FMacroCommandReason & CommandReason);
	bool TryPlaceResourceDepot(const FBuildingInfo & DepotInfo, AResourceSpot * ResourceSpot,
		AInfantry * Worker, const FMacroCommandReason & CommandReason);

	/* Get a general area to try place building near */
	FVector GenerateGeneralArea(EBuildingType BuildingWeWantToPlace);

	/** 
	 *	Code common to both overloaded versions of TryPlaceBuilding. Never call this function 
	 *	except inside TryPlaceBuilding 
	 *
	 *	@param OutLocation - the location of the building. Only relevant if successful 
	 *	@param OutRotation - the rotation of the building. Only relevant if successful 
	 */
	bool TryPlaceBuildingInner(const FBuildingInfo & BuildingInfo, FVector & OutLocation,
		FRotator & OutRotation);

	/** 
	 *	Check if a location is ok to place a building (e.g. no selectables in the way, ground flat
	 *	etc) and place it there if it is
	 *
	 *	@param BuildingInfo - info for building we're trying to place
	 *	@return - whether building was placed successfully 
	 */
	bool TryPlaceBuilding(const FBuildingInfo & BuildingInfo, ABuilding * ConstructionYard, 
		FMacroCommandReason CommandReason);
	bool TryPlaceBuilding(const FBuildingInfo & BuildingInfo, AInfantry * Worker, 
		FMacroCommandReason CommandReason);

	/* Give up trying to place building each tick */
	void GiveUpPlacingBuilding();

	void OnGiveUpPlacingBuilding();

	/** 
	 *	Call function that returns void after delay
	 *
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ACPUPlayerAIController:: *Function)(), float Delay);

	/* Increment/decrement how many commands of a certain type are being carried out */
	void IncrementNumPendingCommands(const FMacroCommandReason & CommandReason);
	void DecrementNumPendingCommands(const FMacroCommandReason & CommandReason);

	/** 
	 *	Increment all the state variables that apply to a building/unit
	 *
	 *	@param BuildingJustCompleted - building type of the building that was just completed 
	 *	@param BuildingInfo - building info of building that was just completed for convenience
	 */
	void IncrementState(EBuildingType BuildingJustCompleted, const FBuildingInfo * BuildingInfo);
	void IncrementState(EUnitType UnitJustCompleted, const FUnitInfo * UnitInfo);

	static uint8 GetNumMacroCommandTypes();
	static EMacroCommandSecondaryType ArrayIndexToMacroCommandType(int32 ArrayIndex);

	static uint8 GetNumMacroCommandReasons();
	static EMacroCommandType ArrayIndexToMacroCommandReason(int32 ArrayIndex);

public:

	/** 
	 *	Called by production queues this player owns when they complete an item in the queue. 
	 *	
	 *	@param Queue - queue that just finished production
	 *	@param ItemProduced - the item that was completed
	 *	@param RemovedButNotBuilt - items removed from the queue because they can no longer be 
	 *	produced e.g. because their prereqs are no longer met. They were NOT built
	 */
	void OnQueueProductionComplete(ABuilding * Producer, const FProductionQueue * Queue, 
		const FCPUPlayerTrainingInfo & ItemProduced, const TArray < FCPUPlayerTrainingInfo > & RemovedButNotBuilt);

	//===========================================================================================
	//	Trivial getters and setters
	//===========================================================================================

	ECPUDifficulty GetDifficulty() const;

	/* Get this player's player state */
	ARTSPlayerState * GetPS() const;

	/* Get faction info for the faction this player is playing as */
	AFactionInfo * GetFI() const;
};
