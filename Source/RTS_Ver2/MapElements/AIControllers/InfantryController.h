// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "InfantryController.generated.h"

class AInfantry;
class ARTSPlayerState;
class ARTSGameState;
class ISelectable;
class URTSGameInstance;
class ABuilding;
class AFactionInfo;
class UCharacterMovementComponent;
class UCanvas;
class AResourceSpot;
struct FContextButtonInfo;
struct FContextButton;
struct FSelectableAttributesBase;
class AInventoryItem;
struct FInventoryItemInfo;
struct FSelectableAttributesBasic;
struct FBuildingInfo;
struct FBuildingTargetingAbilityPerSelectableInfo;
struct FBuildingTargetingAbilityStaticInfo;
struct FBuildingGarrisonAttributes;
struct FVisibilityInfo;


/** 
 *	This holds behavior states. 
 *	Make sure if you add any new values to this enum that you add a TickAI_ func too and then assign 
 *	it in SetupTickFunctions()
 */
UENUM()
enum class EUnitState : uint8
{
	// State AI controller is spawned with
	BehaviorNotStarted,

	// Was just spawned and is moving to the initial location for building
	MovingToBarracksInitialPoint,

	// Moving to the barracks rally point
	MovingToBarracksRallyPoint,

	// State when possessed unit has reached zero heath
	PossessedUnitDestroyed,

	// Has no command and no target within aquire range
	Idle_WithoutTarget,

	/* Has no command so is idle but has aquired a target within aquire range and will persue
	and engage */
	Idle_WithTarget,

	/* Has no command and is returning to leash location after chasing enemy */
	Idle_ReturningToLeashLocation,

	HoldingPositionWithoutTarget,
	HoldingPositionWithTarget,

	// Player has right clicked on world
	MovingToRightClickLocation,

	/* Move to a static selectable e.g. building */
	MovingToPointNearStaticSelectable,

	// Player has right clicked on another selectable that is owned or friendly to player that 
	// can also move itself i.e. not building
	MoveCommandToFriendlyMobileSelectable,

	// Right click or attack move command on an enemy selectable
	RightClickOnEnemy,

	// Attack move command when clicked on location in world
	AttackMoveCommand_WithNoTargetAquired,

	// Doing attack move and has aquired a target within range 
	AttackMoveCommand_WithTargetAquired,

	AttackMoveCommand_ReturningToLeashLocation,

	// Heading to location on map to carry out context command
	HeadingToContextCommandWorldLocation,

	// Heading towards a target to carry out context command
	ChasingTargetToDoContextCommand,

	/* Heading towards a building to do a special building targeting ability */
	HeadingToBuildingToDoBuildingTargetingAbility, 

	HeadingToBuildingToEnterItsGarrison, 

	/* This is the only 'inside garrison' enum value I have. If/when I add being 
	able to attack from inside garrisons then I might want to add another */
	InsideBuildingGarrison,

	HeadingToResourceSpot,

	// Standing at a resource spot waiting our turn to gather from it
	WaitingToGatherResources,

	GatheringResources,

	ReturningToResourceDepot,

	// Playing drop off resources anim
	DroppingOfResources,

	// Heading to a construction site that already exists to work on it
	HeadingToConstructionSite,

	// Heading to a location to try and place building foundations when there
	HeadingToPotentialConstructionSite,

	// Recently tried to place building and waiting for result from player controller
	WaitingForFoundationsToBePlaced,

	ConstructingBuilding,

	// Doing the animation for a context action
	DoingContextActionAnim,

	DoingSpecialBuildingTargetingAbility, 

	/* Moving to an inventory item on ground to pick it up */
	GoingToPickUpInventoryItem, 

	/* Playing animation to pick up inventory item off ground */
	PickingUpInventoryItem, 

	z_ALWAYS_LAST_IN_ENUM
};


/* Some animations unit can play but not all. Unit is always playing an animation. This
enum has some which are important to behavior specifically what to do when commands and
events such as taking damage happen. Any animation not listed in this enum will just use
the value NotPlayingImportantAnim */
UENUM()
enum class EUnitAnimState : uint8
{
	/* The unit may not actually be playing this animation because it may not be set. In
	that case it will just probably stand idle but it doesn't matter - this enum is for behavior
	purposes only */

	/* Not playing an animation that will affect behavior */
	NotPlayingImportantAnim,

	/* Playing attack animation and have not reached anim notify that says anim is finshed */
	DoingAttackAnim,

	// A context action anim that can be interrupted by a player command
	DoingInterruptibleContextActionAnim,

	// A context action anim that cannot be interrupted by a player command
	DoingUninterruptibleContextActionAnim,

	/* A special ability that targets buildings */
	DoingInterruptibleBuildingTargetingAbilityAnim, 

	// Playing resource gathering anim
	PlayingGatheringResourcesAnim,

	// Playing drop off resources anim
	PlayingDropOffResourcesAnim,

	// Constructing building
	ConstructingBuildingAnim
};

/* In shipping these macros just set the new value. When developing they will add a log entry
to the visual logger then set the value */
#if ENABLE_VISUAL_LOG

#define SetUnitState(StateToSet) if (StateToSet == UnitState) \
			{ UE_VLOG(this, RTSLOG, Verbose, TEXT("UnitState: %s --> %s, NO CHANGE, " \
			"function caller: %s"), \
			TO_STRING(EUnitState, UnitState), \
			TO_STRING(EUnitState, StateToSet), \
			*FString(__FUNCTION__)); } \
			else { UE_VLOG(this, RTSLOG, Verbose, TEXT("UnitState: %s --> %s, " \
			"function caller: %s"), \
			TO_STRING(EUnitState, UnitState), \
			TO_STRING(EUnitState, StateToSet), \
			*FString(__FUNCTION__)); } UnitState = StateToSet

#define SetAnimStateForBehavior(StateToSet) if (StateToSet == AnimStateForBehavior) \
			{ UE_VLOG(this, RTSLOG, Verbose, TEXT("AnimStateForBehavior: %s --> %s, NO CHANGE, " \
			"function caller: %s"), \
			TO_STRING(EUnitAnimState, AnimStateForBehavior), \
			TO_STRING(EUnitAnimState, StateToSet), \
			*FString(__FUNCTION__)); } \
			else { UE_VLOG(this, RTSLOG, Verbose, TEXT("AnimStateForBehavior: %s --> %s, " \
			"function caller: %s"), \
			TO_STRING(EUnitAnimState, AnimStateForBehavior), \
			TO_STRING(EUnitAnimState, StateToSet), \
			*FString(__FUNCTION__)); } AnimStateForBehavior = StateToSet

#else // ENABLE_VISUAL_LOG

#define SetUnitState(StateToSet) UnitState = StateToSet
#define SetAnimStateForBehavior(StateToSet) AnimStateForBehavior = StateToSet

#endif // !ENABLE_VISUAL_LOG


// Do a 'ready attack' anim?


/**
 *	Class that controls behavior of infantry. Has many editable properties that will need to
 *	change as your pawn changes.
 *
 *	Some how tos:
 *	Change movement speed: UCharacterMovementComponent::MaxWalkSpeed
 *	Enable/disable acceleration: UCharacterMovementComponent::bRequestedMoveUseAcceleration
 *	Change movement acceleration: UCharacterMovementComponent::MaxAcceleration
 *	Change rate pawn turns: UCharacterMovementComponent::RotationRate.Z for yaw, .Y or .X for pitch can't remember which one
 *
 *
 */
UCLASS()
class RTS_VER2_API AInfantryController : public AAIController
{
	GENERATED_BODY()

public:

	AInfantryController();

	// 4.22: changed this from Possess to OnPossess since Possess is final
	virtual void OnPossess(APawn * InPawn) override;

	void SetReferences(ARTSPlayerState * InPlayerState, ARTSGameState * InGameState,
		AFactionInfo * InFactionInfo, FVisibilityInfo * InTeamVisInfo);

#if !UE_BUILD_SHIPPING

	// Display debugging info
	virtual void DisplayDebug(UCanvas * Canvas, const FDebugDisplayInfo & DebugDisplay, float & YL, float & YPos) override;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry * Snapshot) const override;
#endif // ENABLE_VISUAL_LOG

protected:

	// Typedef for function pointer 
	typedef void (AInfantryController:: *FunctionPtrType)(void);

	/* Path following result of the last move completed */
	FString LastCompletedMoveResult;

	FString FunctionPtrToString(FunctionPtrType FunctionPtr) const;

#endif // !UE_BUILD_SHIPPING

public:

	/* @param BuildingSpawnedFrom - the building the infantry was produced from */
	void StartBehavior(ABuilding * BuildingSpawnedFrom);

	virtual void Tick(float DeltaTime) override;

	/* Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult & Result) override;

	void TickBehavior();

private:

	/* Functions for what to do on AI ticks based on state */
	const TMap<EUnitState, FunctionPtrType> TickFunctions = SetupTickFunctions();

	void TickAI_MovingToBarracksInitialPoint();
	void TickAI_MovingToBarracksRallyPoint();
	void TickAI_IdleWithoutTarget();
	void TickAI_IdleWithTarget();
	void TickAI_Idle_ReturningToLeashLocation();
	void TickAI_HoldingPositionWithoutTarget();
	void TickAI_HoldingPositionWithTarget();
	void TickAI_MovingToRightClickLocation();
	void TickAI_MovingToPointNearStaticSelectable();
	void TickAI_MoveCommandToFriendlyMobileSelectable();
	void TickAI_RightClickOnEnemy();
	void TickAI_AttackMoveCommandWithNoTargetAquired();
	void TickAI_AttackMoveCommandWithTargetAquired();
	void TickAI_AttackMoveCommand_ReturningToLeashLocation();
	void TickAI_HeadingToContextCommandWorldLocation();
	void TickAI_ChasingTargetToDoContextCommand();
	void TickAI_HeadingToBuildingToDoBuildingTargetingAbility();
	void TickAI_HeadingToBuildingToEnterItsGarrison();
	void TickAI_InsideBuildingGarrison();
	void TickAI_HeadingToResourceSpot();
	void TickAI_WaitingToGatherResources();
	void TickAI_GatheringResources();
	void TickAI_ReturningToResourceDepot();
	void TickAI_DroppingOffResources();
	void TickAI_HeadingToConstructionSite();
	void TickAI_HeadingToPotentialConstructionSite();
	void TickAI_WaitingForFoundationsToBePlaced();
	void TickAI_ConstructingBuilding();
	void TickAI_DoingContextActionAnim();
	void TickAI_DoingSpecialBuildingTargetingAbility();
	void TickAI_GoingToPickUpInventoryItem();
	void TickAI_PickingUpInventoryItem();

	/* Once aquired and attacked target at least once, Range * RangeLenience = range this unit
	can keep attacking enemy from. e.g. RangeLenience = 1.1 means the enemy
	can walk outside Range by 10% and still be attacked if they were being
	attacked previously. Prevents very small movements from causing attacks
	to stop. If changing targets then this lenience doesn't apply until the
	unit has attacked that new target at least once */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Attacks", meta = (ClampMin = "1.0"))
	float RangeLenienceFactor;

	/* What to add to acceptance radius when making a move. Want this negative to make unit
	try and move closer. Problem is if it makes the acceptance radius too small and the move is
	to another actor and not the ground then the move will not happen at all because pathfinding
	will not allow it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_MoveToActor;

	// For abilities that require world location
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_MoveToAbilityLocation;

	/* For building targeting abilities (e.g. engineers, spies in C&C) */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_MoveToBuildingTargetingAbilityTarget;

	/* For moves to enter building garrisons */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_MoveToEnterBuildingGarrison;

	// Same as above but for moves to resource spots
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_ResourceSpot;

	// Same as above but for moves to resource depots
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_ResourceDepot;

	// Same as above but for moves to locations where building foundations will be placed
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_PotentialConstructionSite;

	// Same as above but for moves to construction sites
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float AcceptanceRadiusModifier_ConstructionSite;

	//--------------------------------------------------------------------------------------
	//	***** Move Leniences *****
	//--------------------------------------------------------------------------------------
	//	Because often moves do not get as close as expected these allow certain actions to be 
	//	carried out even if the move does not get close enough. Do not crank these to insane 
	//	values though. You want then high enough that the action rarly/never fails but low 
	//	enough so the distance stays realistic
	//--------------------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement", meta = (ClampMin = 0))
	float Lenience_GroundLocationTargetingAbility;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement", meta = (ClampMin = 0))
	float Lenience_ResourceSpotCollection;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement", meta = (ClampMin = 0))
	float Lenience_ResourceDepotDropOff;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement", meta = (ClampMin = 0))
	float Lenience_PotentialConstructionSite;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement", meta = (ClampMin = 0))
	float Lenience_ConstructionSite;


	//------------------------------------------------------------
	//	Idle/AttackMoveCommand properties
	//------------------------------------------------------------

	/* How close enemies have to be to get 'picked up' by this unit when in a state that
	tries to find enemies. This can be larger than units sight radius but unit will still not
	pick up any units hidden in fog - they would need a friendly to reveal that fog for them.
	Also this is the radius of a capsule sweep so you should not make it an uneccessarily
	large number */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Idle", meta = (ClampMin = "0"))
	float Idle_TargetAquireRange;

	/* Same as above but for attack moves */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Attack Move Commands", meta = (ClampMin = "0"))
	float AttackMove_TargetAquireRange;

	/* How far away from encounter point unit will go before giving up
	persuing enemy and returning to complete previous command. 0 means will never move */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Idle", meta = (ClampMin = "0"))
	float Idle_LeashDistance;

	/* How far away from encounter point unit will go before giving up
	persuing enemy and returning to complete previous command. 0 means will never move */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Attack Move Commands", meta = (ClampMin = "0"))
	float AttackMove_LeashDistance;

	/* If unit is idle, the distance from encounter point unit will go before considering
	looking for another target. If no closer targets than current target then unit will
	continue chasing and persuing it. Setting this to 0 basically means that if unit's current
	target is out of range then they will TRY to switch to another target, but if this is positive
	then they will chase for a bit before trying to find other targets */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Idle", meta = (ClampMin = "0"))
	float Idle_GiveUpOnTargetDistance;

	/* Same as above except for attack moves. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Attack Move Commands", meta = (ClampMin = "0"))
	float AttackMove_GiveUpOnTargetDistance;

	/* When was previously idle and are now returning to leash location, how long to wait before
	choosing to aquire targets again. 0 means do not wait at all. Less than 0 implies do not
	aquire any targets until have returned to leash location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Idle", meta = (ClampMin = "-1.0"))
	float Idle_LeashTargetAquireCooldown;

	/* Same as above except for attack moves */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Attack Move Commands", meta = (ClampMin = "-1.0"))
	float AttackMove_LeashTargetAquireCooldown;

	// Time towards calling TickAI
	float AccumulatedTickBehaviorTime;

	// Time towards resetting attack. If less than or equal to 0 then unit can attack
	float TimeTillAttackResets;

	/* The rate at which potentially heavy behavior operations happen */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.004"))
	float BehaviorTickRate;

	/* Reference to player state that owns unit */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Faction info of PS */
	UPROPERTY()
	AFactionInfo * FI;

	/* Reference to unit this is controlling */
	UPROPERTY()
	AInfantry * Unit;

	/* Reference to unit's movement component */
	UCharacterMovementComponent * CharacterMovement;

	/* State variables. These variables will change a lot depending on what the unit is doing
	and should be taken into account when commands come in or events like taking damage happen.
	TimerHandle_Attack and bCanFire are also part of state and can be queried but not modified. */

	/* Unit state */
	EUnitState UnitState;

	// Animation unit is playing for behavior purposes only - may not always be strictly accurate
	EUnitAnimState AnimStateForBehavior;

	/* Where the player clicked when giving a command if that click was not on a selectable */
	FVector ClickLocation;

	/* Location of unit when it encounters an enemy during an attack move command or while
	standing idle. It is the location the unit will return to at some point when it decides
	to stop chasing target or there are no more targets nearby */
	FVector LeashLocation;

	/* Location of unit when another unit was aquired */
	FVector EncounterPoint;

	/* Visibility info for the team this AI controller is on */
	FVisibilityInfo * TeamVisInfo;

	/* The selectable this is currently trying to attack */
	UPROPERTY()
	AActor * AttackTarget;

	/* The length of the bounds of AttackTarget. Used in distance calculations
	because we measure from the edge of our bounds to the edge of their bounds
	to see if we are in range */
	float AttackTargetBoundsLength;

	/* Selectable who was fired at last. Used to know if range lenience should be used */
	UPROPERTY()
	AActor * TargetOfLastAttack;

	// Target of a targeting context action. Also will be used for special building targeting abilities
	UPROPERTY()
	AActor * ContextActionTarget;

	/* Like AttackTargetBoundsLength but for ContextActionTarget */
	float ContextActionTargetBoundsLength;

	/* If true then will not rotate to face focus during UpdateControlRotation */
	bool bIgnoreLookingAtFocus;

	/* Whether the controlled unit has an attack or not. High Templars in SCII are an example of a
	unit that does not have an attack */
	bool bUnitHasAttack;

	/* Whether the controlled unit has the ability to construct buildings. Does not mean 
	unit can repair them though */
	bool bCanUnitBuildBuildings;

	/* Timer handle that if active means we cannot aquire targets */
	FTimerHandle TimerHandle_TargetAquiring;

	/* True if current target was attacked and has not moved out of Range * Leneince */
	bool bIsTargetStillInRange;

	/* Pointer to function. Called in OnMoveCompleted */
	FunctionPtrType DoOnMoveComplete;

	/* Pointer to function. Sets the action to be carried out when
	context action animation reaches anim notify */
	FunctionPtrType PendingContextAction;

	/* The type of action to carry out when anim notify triggers casted to a uint8. This will 
	either be a EContextButton or EBuildingTargetingAbility */
	uint8 PendingContextActionType;

	/* How the pending context action type is being carried out */
	EAbilityUsageType PendingAbilityUsageCase;

	/* When pending ability usage case is selectable's inventory then we store the inventory 
	slot index here */
	uint8 PendingAbilityAuxilleryData;

	/* One use for this is to store the inventory item type when a use item type action is 
	pending */
	uint8 PendingAbilityMoreAuxilleryData;

	/* State resource gathering properties */

	/* Resource spot this unit is assigned to or null if none */
	UPROPERTY()
	AResourceSpot * AssignedResourceSpot;

	/* Depot that unit is returning to to drop off resources. Will get assigned every time
	resource collecting from spot completes */
	UPROPERTY()
	ABuilding * Depot;

	/* Timer handle to monitor when resource gathering is complete */
	FTimerHandle TimerHandle_ResourceGathering;

	/* Clear the properties related to gathering resources when a command is issued */
	void OnCommand_ClearResourceGatheringProperties();

	//------------------------------------------------------------------
	//	State building construction variables
	//------------------------------------------------------------------

	/* Building to work on */
	UPROPERTY()
	ABuilding * AssignedConstructionSite;

	/* The type of building unit is going to put foundations down for when it reaches correct
	location */
	EBuildingType FoundationType;

	/* Rotation to place foundation when at site */
	FRotator FoundationRotation;

	/* Clear the properties for constructing buildings when a command is issued */
	void OnCommand_ClearBuildingConstructingProperties();

	//------------------------------------------------------------
	//	Other
	//------------------------------------------------------------

	void OnCommand_ClearTargetFocus();

public:

	/* Called when given hold position command */
	void OnHoldPositionCommand();

	/** 
	 *	Called when given attack move command
	 *	
	 *	@param Location - world coords of command
	 *	@param Target - the selectable that was clicked on. Null if something
	 *	like the ground was clicked on
	 *	@param TargetInfo - selection info about the target 
	 */
	void OnAttackMoveCommand(const FVector & Location, ISelectable * TargetSelectable, 
		const FSelectableAttributesBase * TargetInfo);

	/** Called when player issues a right-click command and they did not click on another selectable */
	void OnRightClickCommand(const FVector & Location);
	
	/** 
	 *	Called by owning unit
	 *	
	 *	@param TargetAsSelectable - right-click target as selectable
	 *	@param TargetInfo - selection info about Target 
	 */
	void OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo);

	void OnRightClickOnResourceSpotCommand(AResourceSpot * ClickedResourceSpot);

	/* Called by unit when a bIsIssuedInstantly command comes through */
	void OnInstantContextCommand(const FContextButton & Button);

	/* Called when given command that requires a location in the world */
	void OnTargetedContextCommand(EContextButton ActionType, const FVector & Location);

	/* Called when given a command that requires another target */
	void OnTargetedContextCommand(EContextButton ActionType, ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo);

	/* Called when given command to use an item in inventory */
	void OnInstantUseInventoryItemCommand(uint8 InventorySlotIndex, EInventoryItem ItemType,
		const FContextButtonInfo & AbilityInfo);
	void OnLocationTargetingUseInventoryItemCommand(uint8 InventorySlotIndex, EInventoryItem ItemType,
		const FContextButtonInfo & AbilityInfo, const FVector & TargetLocation);
	void OnSelectableTargetingUseInventoryItemCommand(uint8 InventorySlotIndex, EInventoryItem ItemType,
		const FContextButtonInfo & AbilityInfo, ISelectable * ItemUseTarget);

	void OnSpecialBuildingTargetingAbilityCommand(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo);

	/* Called when command is given to enter a building garrison */
	void OnEnterGarrisonCommand(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes);

	/* Called when the possessed unit leaves a garrison */
	void OnUnitExitGarrison();

protected:

	/** 
	 *	3 functions to try and reduce the amount of code written. These functions are called 
	 *	whenever the selectable is given a command to do something that requires doing an ability 
	 *	effect such as an action bar skill or using an inventory item. 
	 *	
	 *	@param UsageCase - action bar, inventory item use, etc 
	 *	@param AuxilleryData - when the usage case is a invenrory item use this is the inventory 
	 *	slot index.
	 */
	void OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, uint8 AuxilleryData, uint8 MoreAuxilleryData);
	void OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, uint8 AuxilleryData, uint8 MoreAuxilleryData, const FVector & Location);
	void OnAbilityCommandInner(EContextButton AbilityType, EAbilityUsageType UsageCase, uint8 AuxilleryData, uint8 MoreAuxilleryData, ISelectable * AbilityTarget);

public:

	/* Pick up an inventory item that is on the ground */
	void OnPickUpInventoryItemCommand(const FVector & SomeLocation, AInventoryItem * TargetItem);

	/* Called when given command to go to location and put down a building foundation there. Only
	applies to build methods LayFoundationsWhenAtLocation and Protoss */
	void OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation);

	/* Called when a building was successfully placed from the context menu of the unit. Unit
	should go over and start working on it */
	void OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, 
		EBuildingBuildMethod BuildMethod);

	/* Called if unit is working on a building an it completes construction */
	void OnWorkedOnBuildingConstructionComplete();

	void OnUnitEnterStealthMode();
	void OnUnitExitStealthMode();

	/* Called from Unit when it takes damage
	@param DamageCauser - selectable that caused the damage
	@param DamageAmount - amount of damage taken for this event */
	void OnUnitTakeDamage(AActor * DamageCauser, float DamageAmount);

	/* Called when possessed unit reaches zero health */
	void OnPossessedUnitDestroyed(AActor * DamageCauser);

	/* Called when possessed unit is consumed */
	void OnPossessedUnitConsumed(AActor * ConsumptionCauser);

	/* Called when the unit destroys another selectable */
	void OnControlledPawnKilledSomething(const FSelectableAttributesBasic & EnemyAttributes);

	/* Called when fire animation fires weapon. */
	void AnimNotify_OnWeaponFired();

	/* Called when weapon firing animation is finished. This signals that
	the unit can move again */
	void AnimNotify_OnAttackAnimationFinished();

	/* Called when a context action animation reaches point where it should
	carry out the action */
	void AnimNotify_OnContextActionExecuted();

	/* Called when a context animation reaches the point where the unit will resume behavior. */
	void AnimNotify_OnContextAnimationFinished();

	/* Called by the optional DropOffResources animation to signal that they have been dropped off */
	void AnimNotify_OnResourcesDroppedOff();

	void AnimNotify_TryPickUpInventoryItemOffGround();

private:

	// These 2 functions are commonly called when moves complete

	// Play idle anim
	void PlayIdleAnim();

	// Play idle anim and enter idle unit state. Does not stop movement
	void GoIdle();

	// Null DoOnMoveComplete, Stop movement, play idle anim and enter idle unit state
	void StopMovementAndGoIdle();

	// Play idle anim, null DoOnMoveComplete and call StopMovement() but do not change unit state
	void StandStill();

	// Continue attack move command to click location
	void AttackMove_OnReturnedToLeashLoc();

	/* Move to rally location of barracks unit was built from */
	void MoveToRallyPoint();


	//----------------------------------------------------------
	//	Picking up inventory items off ground
	//----------------------------------------------------------

	/* Get the acceptance radius to use when moving to an inventory item on the ground to pick 
	it up */
	float GetPickUpItemAcceptanceRadius(AInventoryItem * ItemOnGround) const;

	void OnMoveToPickUpInventoryItemComplete();


	//###############################################
	//	Resource Gathering
	//###############################################

	/* Start the routine of going back and forth between depot and resource spot
	@param ResourcesToGatherFrom - the resource spot to gather from */
	void StartResourceGatheringRoute(AResourceSpot * ResourcesToGatherFrom);

	/* Called when move to resource spot to gather resources completes */
	void OnMoveToResourceSpotComplete();

	/* Queue and possibly start gathering resources from AssignedResourceSpot. Assumes close enough */
	void TryGatherResources();

public:

	/* Returns true if close enough to a assigned resource spot to gather from it */
	bool IsAtResourceSpot() const;

private:

	// Get how close unit has to be to resource spot to be able to collect from it
	float GetDistanceRequirementForResourceSpot() const;

	// Get acceptance radius to use when moving to a resource spot
	float GetResourceSpotAcceptanceRadius() const;

public:

#if !UE_BUILD_SHIPPING
	/* Whether the unit is waiting at a resource spot to collect from it */
	bool IsWaitingToCollectResources() const;
#endif

	/* Start collecting resources from assigned resource spot */
	void StartCollectingResources();

private:

	/* Called when finished gathering resources from spot. Return to depot now */
	void OnResourceGatheringComplete();

	/* Return to a depot to drop off resources */
	void ReturnToDepot();

	/* Returns true if close enough to depot to drop off resources */
	bool IsAtDepot() const;

	float GetDistanceRequirementForDepot() const;

	// Get the acceptance radius to use when moving to a resource depot
	float GetDepotAcceptanceRadius() const;

	void OnReturnedToDepotMoveComplete();

	void OnReturnedToDepot();

	/* Drop off collected resources at depot */
	void DropOffResources();

	void ReturnToResourceSpot();


	//###############################################
	//	Building Construction 
	//###############################################

	/* Return true if close enough to the location where a command was given to go place a
	foundation for a building */
	bool IsAtPotentialConstructionSite() const;

	float GetDistanceRequirementForPotentialConstructionSite() const;

	float GetPotentialConstructionSiteAcceptanceRadius() const;

	/* Try put down foundations for building cached as FoundationType at location ClickLocation
	with rotation FoundationRotation */
	void TryLayFoundations();

	/* Return true if unit is close enough to a building to work on it */
	bool IsAtConstructionSite() const;

	// Distance unit has to be from construction site to work on it
	float GetDistanceRequirementForConstructionSite() const;

	// Accpetance radius to put into MoveToLocation when moving to a construction site
	float GetConstructionSiteAcceptanceRadius() const;

	void OnMoveToConstructionSiteComplete();

	/* Start working on AssignedConstructionSite i.e. constructing it */
	void WorkOnBuilding();


	//###############################################
	//	Context Command Functions 
	//###############################################

	/* Set PendingContextActionType */
	void SetPendingContextActionType(EContextButton InActionType);
	void SetPendingContextActionType(EBuildingTargetingAbility InActionType);

	EContextButton GetPendingContextActionType() const;
	EBuildingTargetingAbility GetPendingBuildingTargetingAbilityType() const;

	/* Try do instant context command provided all the requirements such as selectable 
	resource cost etc are fulfilled */
	void TryDoInstantContextCommand();

	/* Do the action saved in PendingContextActionType. Instant means a bIsIssuedInstantly type
	of action */
	void DoInstantContextCommand();

public:

	float GetDistanceFromLocationForAbility(const FContextButtonInfo & AbilityInfo,
		const FVector & Location) const;

protected:

	/* Get the acceptance radius for a world location targeting ability */
	float GetLocationTargetedAcceptanceRadius(const FContextButtonInfo & AbilityInfo) const;

	/* Check if in range and start animation for context action that requires world location */
	void OnMoveToLocationTargetedContextActionComplete();

	// Play anim for ability or do it straight away if no anim required for ability
	void StartLocationTargetedContextAction(const FContextButtonInfo & AbilityInfo);

	/* Try spawn ability that required world location */
	void TryDoLocationTargetedContextAction();

	/* Spawn ability that required world location */
	void DoLocationTargetedContextAction();

	void OnMoveToBuildingForBuildingTargetingAbilityComplete();

	/* Play anim for building targeting ability or do it straight away if no anim. Building 
	targetingabilities are the ones like engineers or spies in C&C */
	void StartBuildingTargetingAbility();

	void TryDoBuildingTargetingAbility();

	void DoBuildingTargetingAbility();

public:

	float GetDistanceFromAnotherSelectableForAbility(const FContextButtonInfo & AbilityInfo, 
		ISelectable * Target) const;

	float GetDistanceFromAnotherSelectableForAbility(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		ABuilding * TargetBuilding) const;

	float GetDistanceFromBuildingForEnteringItsGarrison(ABuilding * TargetBuilding) const;

protected:

	/* Get the move acceptance radius for an ability that requires another target */
	float GetSelectableTargetingAbilityAcceptanceRadius(const FContextButtonInfo & AbilityInfo) const;

	/* Get move acceptance radius for a special building targeting ability */
	float GetBuildingTargetingAbilityAcceptanceRadius(const FBuildingTargetingAbilityStaticInfo & AbilityInfo) const;

	/* Get move acceptance radius for a enter garrison command */
	float GetBuildingEnterGarrisonAcceptanceRadius(const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes) const;

	/* Check if in range for context action that requires another target and start it if so */
	void OnMoveToSingleTargetContextActionTargetComplete();

	// Play anim for ability if it uses one or do ability instantly
	void StartSingleTargetContextAction(const FContextButtonInfo & AbilityInfo);

	/* Try do single target ability given every check passes */
	void TryDoSingleTargetContextAction();

	/* Spawn ability that requires another target */
	void DoSingleTargetContextAction();


	//###############################################
	//	Other Functions 
	//###############################################

	/* Move towards attack target, stopping when in attack range */
	void MoveTowardsAttackTarget();

	float GetMoveToAttackTargetAcceptanceRadius() const;

	float GetNonHostileMoveToLocationAcceptanceRadius(ISelectable * SelectableToMoveTo) const;

	// Move towards context action target
	void MoveTowardsContextActionTarget(const FContextButtonInfo & AbilityInfo);

	/* Try move in range of a building to use a building targeting ability */
	void MoveTowardsBuildingTargetingAbilityTarget(const FBuildingTargetingAbilityStaticInfo & AbilityInfo);

	/* Try move in range of a building to enter it's garrison */
	void MoveTowardsBuildingToEnterItsGarrison(const FBuildingGarrisonAttributes & BuildingsGarrisonAttributes);

	void OnMoveToBuildingToEnterItsGarrisonComplete();

	/* Called when possessed unit takes damage. Given a damage causer check if we should change
	targets to them */
	virtual bool ShouldChangeTargetOnDamage(const AActor * DamageCauser, float DamageAmount) const;

	/* Can a selectable be aquired as a new target when performing a capsule sweep for nearby
	targets */
	bool CanTargetBeAquired(const AActor * PotentialNewTarget) const;

	bool HasTarget() const;

	bool IsTargetAttackable() const;

	/* Check the dynamic properties of a selectable to see if they can be attacked. Does not check
	range or if attack timer handle has reset */
	bool IsSelectableAttackable(const AActor * Selectable) const;

	/* Start animation to attack with weapon */
	void BeginAttackAnim();

	/* Make attack at AttackTarget */
	void OnAttackMade();

	/* Called when unit can fire again */
	void OnResetFire();

	/* Returns closest enemy within Radius that can be aquired by this unit. This means checking
	things like whether we can attack air and whether enemy flies, if they are outside fog of war
	etc
	@param Radius - how far to check
	@param bForceOnTargetChangeCall - if true then if a target is found then OnTargetChanged will
	be called. If false then it will compare if new target equals current target (AttackTarget)
	before deciding to call OnTargetChanged. This is here because sometimes AttackTarget isn't
	nulled at times when it should be. Currently I never use this param but when I get
	confident enough that I could then don't need to null AttackTarget on every command, then
	in states like Idle_WithNoTarget make sure to set this param to true
	@return - the closest enemy within Radius */
	AActor * FindClosestValidEnemyInRange(float Radius, bool bForceOnTargetChangeCall);
	// If above 2nd param dont solve rotation issue then try nulling AttackTarget often

	/* returns true if moving */
	bool IsMoving() const;

	/* Returns true if AAIController::StopMovement() will actually do something when called.
	If this returns false then you know that OnMoveComplete will not be called if you call
	StopMovement */
	bool WillStopMovementGoThrough() const;

	/*########################################################################################

	List of things to check for to know if attack can be made:

	- Is attack off cooldown? ( CanFire() )
	- Is AttackTarget not null + valid? ( UStatics::IsValid(AttackTarget) )
	- Is AttackTarget outside fog, not stealthed etc? ( IsTargetAttackable() )
	- Is AttackTarget in range? ( IsTargetInRange() )
	- Is unit facing AttackTarget? ( IsFacingAttackTarget() )

	If all these pass then AttackTarget is attackable. But keep one other thing in mind:
	Make sure whenever you assign a non-null value to AttackTarget that you use
	CanTargetBeAquired(AActor *) first to see if it is possible to aquire it. This will
	check conditions like if the actor is on the enemy team - things that don't need
	re-checking every time you try attack AttackTarget. FindClosestValidEnemyInRange
	will do this automatically + check UStatics::IsValid(). That leaves 3 things that
	need to be done after calling it: CanFire(), IsTargetInRange() and IsFacingAttackTarget()

	########################################################################################*/

	/* Returns true if attack has reset */
	bool CanFire() const;

	/* Returns true if AttackTarget is within attack range */
	bool IsTargetInRange();

	/* Returns true if facing attack target enough to attack them */
	bool IsFacingAttackTarget() const;

	/* Call whenever AttackTarget changes */
	void OnTargetChange(AActor * NewTarget);

	// Call when context action target changes
	void OnContextActionTargetChange(AActor * NewContextActionTarget, ISelectable * TargetAsSelectable);

	/* Return true if unit is in a behavior state that wants to make attacks */
	bool WantsToAttack() const;

	/* Make unit face the direction they are moving */
	void SetFacing_MovementDirection();

	/* Make unit face their focus (usually another enemy selectable) */
	void SetFacing_Focus();

	void StopBehaviour();

	float Range;
	float LenienceRange;
	float AttackFacingRotationRequirement;

	EPathFollowingRequestResult::Type Move(const FVector & Location, float AcceptanceRadius = 1.f);

	/* Call function with no return after delay, or crash if delay is <= 0
	@param TimerHandle timer handle to use
	@param Function function to call
	@param Delay delay for calling function */
	void Delay(FTimerHandle & TimerHandle, void(AInfantryController:: *Function)(), float Delay);

	/* Call function only if delay is greater than 0, otherwise do nothing */
	void DelayZeroTimeOk(FTimerHandle & TimerHandle, void(AInfantryController:: *Function)(), float Delay);

	void SetupReferences(APawn * InPawn);

	/* Setup function pointers */
	TMap <EUnitState, FunctionPtrType> SetupTickFunctions() const;

	// For timer handles
	void DoNothing();

public:

	/* Functions to call from unit when its attributes change during a match, possibly because
	of upgrades */

	void OnUnitHasAttackChanged(bool bNewHasAttack);
	void OnUnitRangeChanged(float NewRange);
	void OnUnitAttackFacingRotationRequiredChanged(float NewAngleRequired);

	/* These were added for the debug widget */
	EUnitState GetUnitState() const { return UnitState; }
	EUnitAnimState GetUnitAnimState() const { return AnimStateForBehavior; }

#if WITH_EDITOR
	void DisplayOnScreenDebugInfo();
#endif
};
