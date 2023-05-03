// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//==============================================================================================
//	Common User Editable Enums
//==============================================================================================
/**
 *	You can and should edit these enums a lot to make your game. This file is really easy: 
 *	- just always make sure to only add/delete values in between the lines specified
 */


/** 
 *	Different playable factions. 
 *	
 *	Examples of factions from RTS games: Terran, Zerg, Soviets, Allies, etc 
 */
UENUM()
enum class EFaction : uint8
{
	None    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Humans,
	Monsters,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	// This should always be last in the enum
	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	Allied players share the same team
 */
UENUM()
enum class ETeam : uint8
{
	Uninitialized    UMETA(Hidden),

	//=========================================
	/* *** Add new teams below here *** */
	//=========================================

	Team1,
	Team2,
	Team3,
	Team4,

	//=========================================
	/* *** Add new teams above here *** */
	//=========================================

	/* Examples of neutral: map resource spots */
	Neutral    UMETA(Hidden),

	/* Someone spectating the match */
	Observer
};


/* Different types of buildings */
UENUM()
enum class EBuildingType : uint8
{
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	HumanMain,
	HumanSupplyDepot,
	HumanPowerPlant,
	HumanBarracks, 
	HumanRadar, 
	HumanAirForceBase, 
	HumanMissleSilo, 
	HumanTurret, 
	HumanMissleTurret, 
	HumanSniperNest, 
	MonstersMain, 
	MonstersBarracks,  
	MonstersAdvancedBarracks, 
	MonstersItemShop, 
	MonstersSpecialBarracks, 
	MonstersSandDepot,
	MonstersSuperweapon, 
	MonstersObelisk, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	// Do not use this as one of your faction buildings
	ResourceSpot   UMETA(Hidden),
	// This is always 2nd last in the enum
	z_ALWAYS_2ND_LAST_IN_ENUM   UMETA(Hidden),
	// This is always last in the enum
	NotBuilding   UMETA(Hidden)
};


/** 
 *	Different types of units. 
 *	
 *	The ordering of values slightly matters. When selecting multiple units at once the highest 
 *	value (lower down the list) will always be chosen as the "PrimarySelected". 
 *	PrimarySelected is the unit whose context menu is shown. e.g. in SCII when you select a 
 *	zealot and high templar it is the high templar's context menu that is shown. 
 *
 *	This scheme doesn't play nice with using the same types over multiple factions though. 
 *	Could perhaps be moved onto AFactionInfo. Although users can just create copies of the 
 *	value if required e.g. BasicInfantry gets replaced with BasicInfantry_Terran and 
 *	BasicInfatry_Zerg
 */
UENUM()
enum class EUnitType : uint8
{
	// Here to pick up uninitialized values
	None    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/*~ Units here will be given lowest priority when choosing
	a context menu to show when selecting multiple units */

	HumanWorker, 
	HumanCollector, 
	HumanBasic,
	HumanSniper, 
	HumanRecon, 
	HumanSpellcaster, 
	HumanSpecial, 
	MonstersCollector, 
	MonstersMelee, 
	MonstersBasicInfantry, 
	MonstersHeavyInfantry, 
	MonstersMedic,
	MonstersSpecial, 

	/*~ Units here will be given highest priority when choosing
	a context menu to show when selecting multiple units */

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	//~ This is always 2nd last in the enum
	z_ALWAYS_2ND_LAST_IN_ENUM	   UMETA(Hidden),
	//~ This is always last in the enum
	NotUnit		UMETA(Hidden)
};


/* Different types of upgrades */
UENUM()
enum class EUpgradeType : uint8
{
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	ImprovedBasicInfantryDamage, 
	ImprovedSniperFireRate, 
	ImprovedSpellcasterManaRegenRate, 
	ImprovedCollectorResourceCapacity, 
	UnlockTrainMonstersMelee, 
	UnlockTrainSnipers, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_2ND_LAST_IN_ENUM    UMETA(Hidden),

	/* For making adding to TMap in editor less painful. Leave this always
	as last in enum */
	None    UMETA(Hidden)
};


/* Inventory items */
UENUM()
enum class EInventoryItem : uint8
{
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Shoes,
	SimpleBangle, 
	RedGem, 
	GreenGem, 
	RottenPumpkin, 
	Apple,
	AverageCrown, 
	GoldenCrown, 
	Necklace, 
	GoldBar, 
	GoldenDagger, 
	StrongSniperRifle, 
	ArtilleryBeacon,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	None    UMETA(Hidden)
};


/** 
 *	Different types of resources that can be aquired, like vespene gas and minerals in SCII.
 *
 *	Adding new ones of these requires a little more setup inside the player state. 
 *
 *	"Housing" type resources like the population resource in starcraft should not be added here. 
 *	They should be added to the EHousingResourceType enum.
 */
UENUM()
enum class EResourceType : uint8
{
	// Always first in enum
	None	UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Cash,
	Sand,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	Resource types similar to the population resource in starcraft II.  
 *
 *	For now buildings can provide it and units and use it. Could possibly allow buildings to 
 *	also use it aswell but just keeping it basic for now. 
 */
UENUM()
enum class EHousingResourceType : uint8
{
	None    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Population,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};


UENUM()
enum class ECPUDifficulty : uint8
{
	None	UMETA(Hidden),

	/* CPU player does nothing. Useful for testing. Note that only the CPU player's behavior 
	is disabled but each selectable's behavior still runs */
	DoesNothing,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Easy, 
	Medium, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	The type that a selectable is for targeting purposes. Each selectable has exactly one of 
 *	these. But an attack/ability/whatever can target more than one type.
 *	
 *	By default the "Default" value is given to everything both as its type and as one of the 
 *	types it can attack.
 *
 *	Flying does not need to be taken into account here and is taken care of elsewhere
 *
 *	E.g. in Command and Conquer Red Alert 2 a sniper can only attack infantry
 *	but not vehicles or buildings. This is basically what this is. 
 */
UENUM()
enum class ETargetingType : uint8
{
	/* Uninitialized value never to be used */
	None    UMETA(Hidden),

	/** 
	 *	This value is given to every selectable by default to allow 'everything works as 
	 *	expected' out of the box. This is given both as their type and the types they can target. 
	 *	Made it hidden assuming the editor values will be reset to this if user sets a custom enum 
	 *	value then later deletes it. If not the case then hidden meta should be removed. You can 
	 *	actually rename this to something else if you like. 
	 */
	Default    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	BiologicalInfantry,
	MechanicalInfantry,
	BiologicalAndMechanicalInfantry,
	Building, 
	

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	Armour type of selectable. Used for calculating damage. Similar to Mechanical, Biological etc
 *	in SCII. 
 *	
 *	Currently only one armour type can be assigned to each selectable so if you want a selectable 
 *	to have multiple types you would need to create entries like TallGreenBiologicalInfatry
 */
UENUM()
enum class EArmourType : uint8
{
	None    UMETA(Hidden),

	/* Default value given to everything that needs an armour value. This can be removed but 
	may have some references to it in code that will need changing (replacing those instances 
	with EArmourType() + 1 could be a possibility as long as user has at least one custom 
	enum value */
	Default    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Building, 
	SuperweaponBuilding, 
	Infantry, 
	HeavyInfantry, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM	   UMETA(Hidden)
};


/** 
 *	In regards to the HUD, if using persistent build tabs like in C&C red alert 2, the types
 *	of tabs there are (e.g. buildings, aircraft, boats etc). Ordering is important. The first button
 *	found when iterating widget children will be assigned the first value in enum (excluding None),
 *	2nd the 2nd and so on 
 */
UENUM()
enum class EHUDPersistentTabType : uint8
{
	/* Use this if you do not want the button for this to appear on the HUD persistent tab.
	This is a special case value; if you assign a persistent tab to use this then it will not
	work correctly */
	None,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* Tab for buildings */
	Buildings,
	/* Tab for all units */
	Units,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Used to play animations (generally for units, not buildings) */
UENUM()
enum class EAnimation : uint8
{
	/* Here for removing key clashes when adding entries to TMap */
	None    UMETA(Hidden),

	/* Should be looping */
	Idle,
	/* Should be looping */
	Moving,
	/** 
	 *	Should have 2 anim notifies in it: 
	 *	- FireWeapon 
	 *	- OnAttackAnimationFinished. 
	 *	Should also have a looping idle animation at the end of it 
	 */
	Attack,
	Destroyed,
	
	/* Only relevant if unit can gather resources and is optional */
	GatheringResources,

	/* Should be looping */
	ConstructBuilding,

	/* Picking up an inventory item that's in the world */
	PickingUpInventoryItem,

	/* Optional for when moving with resources held. Should be looping */
	MovingWithResources,

	/* Optional. The animation to play having returned to a depot with resources. */
	DropOffResources,

	/* Note: if adding more non-custom animations here make sure to update 
	Statics::InitWarningIfNotSetAnims to use it */

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* These can be deleted if you like and replaced with whatever you want. You can be 
	as specific as you like e.g. "CastFireball", "CallArtilleryStrike" */ 
	ContextAction_1,
	ContextAction_2,
	ContextAction_3, 
	
	Throw,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Animations for buildings */
UENUM()
enum class EBuildingAnimation : uint8
{
	None    UMETA(Hidden),

	/* The animation to play when being constructed */
	Construction,
	/* When nothing is happening. Should be looping */
	Idle,
	/* Right before a unit is built from it */
	OpenDoor,

	/* This will need the anim notify OnZeroHealthAnimationFinished on it */
	Destroyed,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* Open the door for the missle silo in preparation for launching a nuke. */
	OpenMissleSiloDoor, 
	/* Close the missle silo door after the nuke has been launched. 
	Probably want idle looping at the end of this anim */
	CloseMissleSiloDoor,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_2ND_LAST_IN_ENUM    UMETA(Hidden),

	/* This is not an animation, but is used by the server to signal that the building should
	sink into the ground */
	SinkIntoGround    UMETA(Hidden)
};


/** 
 *	Different actions context menu buttons can do. 
 */
UENUM()
enum class EContextButton : uint8
{
	// Not a value that should be used
	None    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* From range heal a single target */
	Heal,

	ArtilleryStrike,

	/* Increase own move speed by 50% for 10 secs */
	Dash,

	/* Increase own move speed by 30% for some amount of time */
	Haste,

	Corruption,

	StealthSelf,

	/* Removes 'The plague' debuff from a friendly target */
	Cleanse,

	/* Create a ice type barrier around the user that damages enemies while they stand in it */
	IceBarrier, 

	DeathAura,

	Death,

	/* Call down a storm of ice shards */
	Blizzard,

	// Burn mana from a single target
	ManaBurn,

	EatRottenPumpkin, 
	EatApple, 

	LightningStrike, 

	// Only usable on targets below 50% health. Kills them
	FinishingBlow,

	// Similar to axe's culling blade
	CullingBlade,

	/* Reveal some part of the map for some amount of time */
	RadarScan,

	/* Launch a nuke. Instigated by a building */
	Nuke,

	HealOverTime, 
	IncreasingHealOverTime, 

	Beserk, 

	/* Destroy self and deal massive damage to those around you */
	Sacrifice, 

	/* Target a friendly unit and deal damage to them. If they die the ability instigator 
	gains mana */
	Tribute, 

	/* Artillery strike that comes from the artillery beacon item */
	BeaconArtilleryStrike,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	//~ Currently using AttackMove in UStatics to define NUM_CUSTOM_CONTEXT_ACTIONS
	AttackMove,
	HoldPosition,

	//~ This should always be 6th last in enum 
	BuildBuilding,
	//~ This should always be 5th to last in enum
	Train,
	//~ This should always be 4th last in enum
	Upgrade,
	// This should always be 3rd last in the enum
	z_ALWAYS_3RD_LAST_IN_ENUM    UMETA(Hidden),
	PlacingGhost    UMETA(Hidden),
	/* Set in LMB press and changed on LMB release */
	RecentlyExecuted    UMETA(Hidden),
};


/** 
 *	Abilities that are instigated by the player as opposed to a selectable 
 *	e.g. in C&C Generals: fuel air bomb, artillery strike, etc 
 */
UENUM()
enum class ECommanderAbility : uint8
{
	None    UMETA(Hidden), 

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Human_ArtilleryStrike_Rank1,
	Human_ArtilleryStrike_Rank2, 
	 
	/* Steals some resources from a hostile player */
	Human_ResourceSteal,

	/* Causes an explosion to happen somewhere */
	Monsters_Explosion_Rank1, 
	Monsters_Explosion_Rank2, 

	/* Buffs all units in an AoE with something */
	Monsters_Frenzy_Rank1, 
	Monsters_Frenzy_Rank2, 
	Monsters_Frenzy_Rank3,

	Monsters_GlobalFrenzy, 
	
	/* Destroy all the collectors for a player */
	Monsters_DestroyAllPlayersCollectors,

	Monsters_MakeUnitInvulnerable,

	Monsters_KillUnit, 

	Human_AirshipBarrage, 

	Monsters_UnlockTrainMelee,

	Humans_Snipers_Rank1,
	Humans_Snipers_Rank2, 

	Human_Warthog,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Different types of nodes that appear on a commander's skill tree */
UENUM()
enum class ECommanderSkillTreeNodeType : uint8
{
	None    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Human_ArtilleryStrike, 
	Human_ResourceSteal,
	Monsters_Explosion, 
	Monsters_Frenzy, 
	Monsters_GlobalFrenzy, 
	Monsters_DestroyAllPlayersCollectors, 
	Monsters_MakeUnitInvulnerable,
	Monsters_KillUnit, 
	Human_AirshipBarrage,
	Monsters_UnlockTrainMelee,
	Humans_UnlockTrainSnipers, 
	Human_Warthog, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	Building targeting abilities are abilities that can only target buildings. This enum 
 *	was added to do custom behavior when right-clicking on buildings e.g. in C&C engineers will 
 *	capture the building, spies will show you what is being produced, etc 
 */
UENUM()
enum class EBuildingTargetingAbility : uint8
{
	None    UMETA(Hidden), 

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	StealResources, 
	DealDamage,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/** 
 *	Reasons an ability cannot be used e.g. "HasteBuffNotPresent", "TargetNotBelow30PercentHealth".
 *
 *	This enum is primarily used for displaying messages on the HUD so you can choose to make the 
 *	values as general as you like e.g. "AbilityNotUsable" 
 */
UENUM()
enum class EAbilityRequirement : uint8
{	
	/* Default value, never to be used */
	Uninitialized    UMETA(Hidden),
	
	/* This is used as a return value for the requirements checking function and signals that 
	all requirements were met */
	NoMissingRequirement    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	BuffOrDebuffNotPresent, 
	
	// Ability must be used on a mana using target
	TargetMustUseMana, 

	RequiresTargetWithHealth,

	TargetsHealthNotLowEnough,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


// Timed == false && NeedsTickLogic == false
/**
 *	Different types of buffs and debuffs that do not have a duration and do not require any 
 *	kind of tick logic. These types of buff/debuffs can only be removed by some other kind of 
 *	event and never remove themselves over time. 
 *	
 *	Examples of these: 
 *	- an aura you stand in that just increases the damage you deal
 *	- holding onto resources 
 *	- any buff/debuff that isn't timed and doesn't do anything over time
 *	- ghost invis in SCII because it's the event of running out of energy that triggers invis to 
 *	fall off 
 */
UENUM()
enum class EStaticBuffAndDebuffType : uint8
{
	/* Default value, never to be used */
	None    UMETA(Hidden),
	
	/* Unit is holding resources */
	HoldingResources    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	ThePlague, 
	Disease,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


// Timed == true || NeedsTickLogic == true
/** 
 *	Buffs/debuffs that fall off after a certain amount of time or that require tick type logic. 
 *	They can also be removed by other means if desired 
 *	
 *	Examples of these: 
 *	- SCII marine stimpack because it wears off after 15 sec
 *	- an aura you stand in that decreases health every 2 sec
 *	- any kind of damage over time effect 
 */
UENUM()
enum class ETickableBuffAndDebuffType : uint8
{
	/* Default value, never to be used */
	None    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Dash, 
	Haste, 
	BasicHealOverTime,
	IncreasingHealOverTime,
	CleansersMight,
	PainOverTime, 
	Corruption,
	NearInvulnerability, 
	RottenPumpkinEatEffect,
	
	/* Enter stealth mode for 10 seconds */
	TempStealth, 

	/* Increase damage done by 50%. Increases move speed by 30%. Increases attack speed by 50% */
	Beserk,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/** 
 *	Subtypes of buffs/debuffs. E.g. magic, curse, poison etc 
 *	
 *	These may not be used by a lot of people. 
 *	The only "out of the box" functionality they have is that buff/debuff widgets can optionally 
 *	be setup to have their border display as a certain color depending on the buff/debuff subtype. 
 *	This behavior can also be achieved by just having your image already have the border in it by 
 *	editing it outside UE4. 
 *
 *	Another use for them could be to implement abilities that do something like "remove 1 poison 
 *	type debuff from a target"
 */
UENUM() 
enum class EBuffAndDebuffSubType : uint8
{
	/* This should not be referenced anywhere in code. It is only here so the editor default 
	value looks prettier than z_ALWAYS_LAST_IN_ENUM (which I cannot get to change with 
	the DisplayName and ScriptName specifiers in the UMETA macro) */
	Default    UMETA(Hidden),
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================



	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* The outcome of an ability. Can add more values if needed */
UENUM()
enum class EAbilityOutcome : uint8
{
	Default, 

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	CullingBlade_BelowThreshold, 
	CullingBlade_AboveThreshold, 

	DidNotKill, 
	DidKill,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/* The result of trying to apply a buff/debuff. Can add more values if needed */
UENUM()
enum class EBuffOrDebuffApplicationOutcome : uint8
{
	Failure,
	ResetDuration,
	Success, 
	AlreadyHasIt,

	/* An example of a couple of values that could be added for axe's culling blade if it 
	were to ever be implemented */
	CullingBlade_Kill, 
	CullingBlade_DealDamage,
};


/* The result of tick logic of a buff/debuff. Add more values if needed */
UENUM()
enum class EBuffOrDebuffTickOutcome : uint8
{
	StandardOutcome,

	PainOverTime_KilledTargetAndGotHeal, 
	PainOverTime_KilledTargetButNoHeal
};


/* The result of trying to remove buff/debuff from a selectable. Add more values if needed */
UENUM()
enum class EBuffOrDebuffRemovalOutcome : uint8
{
	NotPresent,
	Failure, 
	Success, 

	Cleanse_ResetDurationOfCleansersMight, 
	Cleanse_FreshApplicationOfCleansersMight, 
};


/* The reason a buff/debuff is removed from its target. */
UENUM()
enum class EBuffAndDebuffRemovalReason : uint8
{
	/* All ticks have happened */
	Expired    UMETA(Hidden),

	/* Target reached zero health most likely not because of this buff/debuff but because of
	some other reason */
	TargetDied    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	CleanseSpell, 

	Test,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/* A point on an actor to know where to attach/spawn things relative to it */
UENUM()
enum class ESelectableBodySocket : uint8
{
	None    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* The part of the selectable that makes contact with the ground. */
	Floor,

	/* The part of the body you consider the middle */
	Middle,

	Head,
	AboveHead_DoesNotRotate,

	Floor_DoesNotRotate,

	// Where to launch nuke from
	NukeLaunchSite, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/** 
 *	Different starting resource amounts. Integer would probably work just as well. 
 */
UENUM()
enum class EStartingResourceAmount : uint8
{
	// Default value used if no value specified by user
	Default   UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Low,
	Medium,
	High,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_2ND_LAST_IN_ENUM   UMETA(Hidden), 

	/* Use whatever is defined in development settings */
	DevelopmentSettings   UMETA(Hidden)
};


/** 
 *	Condition that determines if a player has been defeated. 
 *	In addition to adding a value here you will need to add a function to ARTSGameMode that 
 *	decides when a player is defeated
 */
UENUM()
enum class EDefeatCondition : uint8
{
	/* Ordering here is ordering they will appear in UI */

	/* Here to pickup uninitialized values - should never be used */
	None    UMETA(Hidden),

	/* Cannot be defeated. Match will never end. Great for testing with editor. Probably 
	safe to delete this but need to change the references to it in code to something */
	NoCondition,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* You are defeated when you have no buildings left. Having building foundations placed will
	count as a building */
	AllBuildingsDestroyed,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Different resources that selectables can have e.g. mana, energy */
UENUM()
enum class ESelectableResourceType : uint8
{
	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	Mana,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	None,
};


/** 
 *	For each player buildings with the same building network share the same garrisoned units. 
 *	Examples: 
 *	- in SCII nydus networks and nydus worms share the same network 
 *	- in C&C generals tunnel networks and sneak attacks share the same network 
 *	- the 2 examples I gave above each have 2 buildings in each network but if sneak attacks 
 *	didn't exist just having tunnel networks with their own network is still meaningful
 */
UENUM()
enum class EBuildingNetworkType : uint8
{
	/* This values means 'not part of any network' 
	e.g. terran bunkers, C&C generals china bunkers */
	None, 

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	HumanBarracks,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Different types of mouse cursors */
UENUM()
enum class EMouseCursorType : uint8
{
	/* Default value to make it easier to add entries to TMaps in editor */
	None,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	SciFi_Default, 
	SciFi_BlueOutline,
	SciFi_RedCross, 
	SciFi_QuestionMark, 
	SciFi_BlueCrosshair, 
	SciFi_EdgeScrollUp, 
	SciFi_EdgeScrollUpRight,
	SciFi_EdgeScrollRight,
	SciFi_EdgeScrollDownRight,
	SciFi_EdgeScrollDown,
	SciFi_EdgeScrollDownLeft,
	SciFi_EdgeScrollLeft,
	SciFi_EdgeScrollUpLeft, 
	EnterGarrison, 
	CashHack, 
	CommandoBomb, 

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Different settings for controls */
UENUM()
enum class EControlSettingType : uint8
{
	None    UMETA(Hidden),

	//=========================================
	/* *** Add new values below here *** */
	//=========================================

	/* Speed camera moves when using keyboard to move it */
	CameraPanSpeed_Keyboard,

	/* Speed camera moves when moving mouse to edge of screen */
	CameraPanSpeed_Mouse,

	/* Max speed of camera */
	CameraMaxSpeed,

	bEnableCameraMovementLag, 

	CameraMovementLagSpeed,

	/* How fast changes in camera move direction should happen with regards to WASD and mouse
	edge movement */
	CameraTurningBoost,

	/* Amount a single mouse wheel scroll will change zoom */
	CameraZoomIncrementalAmount,

	/* Speed camera zooms in/out */
	CameraZoomSpeed,

	/* Speed the X axis changes when the middle mouse button is held down and looking around */
	MMBLookYawSensitivity,

	/* Speed the Y axis changes when the middle mouse button is held down and looking around */
	MMBLookPitchSensitivity,

	/* Whether to invert the X axis middle mouse button free look */
	bInvertMMBLookYaw,

	/* Whether to invert the Y axis middle mouse button free look */
	bInvertMMBLookPitch,

	/* If true, when MMB is pressed view will update gradually. Makes looking around with MMB
	pressed less repsonsive. False = view updates instantly */
	bEnableMMBLookLag,

	/* Only relevant if bEnableMMBLookLag is true. The amount of lagginess. Lower = more laggy */
	MMBLookLagAmount,

	/* The camera pitch to start a match with and to return to when using the 'return to default
	rotation' button */
	DefaultCameraPitch,

	/* Amount of camera zoom to start match with */
	DefaultCameraZoomAmount,

	/* Spee camera resets to default rotation/zoom */
	ResetCameraToDefaultSpeed,

	/* How close mouse must be to the edge of the screen for camera panning to happen */
	CameraEdgeMovementThreshold,

	/* Acceleration when moving camera */
	CameraAcceleration,

	/* How fast camera stops */
	CameraDeceleration,

	/* Time allowed for double clicks */
	DoubleClickTime,

	MouseMovementThreshold,

	GhostRotationRadius,

	GhostRotationSpeed,

	//=========================================
	/* *** Add new values above here *** */
	//=========================================

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


//////////////////////////////////////////////////////////////////////////////////////////////////
//===============================================================================================
//===============================================================================================
//	Enums that I have anticipated you will not need to edit often
//===============================================================================================
//===============================================================================================
//////////////////////////////////////////////////////////////////////////////////////////////////


/** 
 *	Reason for an item entering inventory. 
 *	
 *	You may want to add mor entries to this enum if you create abilities/effects that add items 
 *	to inventories.
 */
UENUM()
enum class EItemAquireReason : uint8
{
	/* Purchased the item from a shop */
	PurchasedFromShop,

	/* The item was created by combining other items e.g. divine rapier, boots of travel */
	CombinedFromOthers,

	/* Found it in the world and picked it up */
	PickedUpOffGround,

	/* This is for development. Item was added using the development cheat widget */
	MagicallyCreated,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================



	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/**
 *	The reason for an item being removed from an inventory. 
 *	
 *	You may want to add more entries to this enum if you create abilities/effects that remove items 
 *	from inventories.
 */
UENUM()
enum class EItemRemovalReason : uint8
{
	/* Note: the ordering these values might be dependent on EItemEntersWorldReason ordering. 
	Look for static_casts between the two for confirmation */
	
	/* The item drops when it's weilder dies which is what has happened e.g. divine rapier, 
	gem of true sight */
	DroppedOnDeath,
	
	/* The item is being removed because it + other items are being combined to create another 
	item e.g. claymore is removed to make a divine rapier */
	Ingredient,

	/* When item reaches zero charges it is removed which is what happened e.g. clarity potions, 
	TP scrolls */
	RemovedOnZeroCharges,

	/* Item was sold to a shop */
	Sold,

	//=========================================
	/* *** Add new values below here *** */
	//=========================================



	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};


/* Reasons for an inventory item appearing in the world */
UENUM()
enum class EItemEntersWorldReason : uint8
{
	/* Item is the type of item that drops when it's owner dies which is 
	what has happened */
	DroppedOnDeath, 

	/* Adding this because I anticipate it will be used at some point.  
	Selectable was commanded to drop the item */
	ExplicitlyDropped, 
};


/* The reason for an inventory item reaching zero charges. */
UENUM()
enum class EItemChangesNumChargesReason : uint8
{
	/* The owner used it which consumed a charge */
	Use,
	
	//=========================================
	/* *** Add new values below here *** */
	//=========================================



	//=========================================
	/* *** Add new values above here *** */
	//=========================================
};
