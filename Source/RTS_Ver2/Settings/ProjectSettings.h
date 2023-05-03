// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h" // For EAffiliation


/**
 *	A file that holds settings for the project.
 */


//===============================================================================================
//	Enum Definitions
//===============================================================================================

/* Rule that decides how to change the color of a player's selectables */
UENUM()
enum class ESelectableColoringRule : uint8
{
	/* Colors will not change */
	NoChange, 

	/* Color will be set based on the selectable's owner i.e. all the selectables you own are 
	one color, the selectables another player owns are another color */
	Player, 

	/* Color will be set based on the selectable's owner's team i.e. selectables on the same 
	team share the same color */
	Team
};


/** 
 *	Rule that decides how to convert a float to FText  
 *	
 *	These are all implemented using FText::Format. Enum contains the formats I anticipate most 
 *	people using.
 *	@See USingleBuffOrDebuffWidget::GetDisplayTextForDuration for implementation
 */
UENUM()
enum class EFloatDisplayMethod : uint8
{
	/* Display as an integer but always round it up e.g. 4 sec duration is shown as 4, 3, 2, 1 */
	ZeroDP_RoundDown,
	
	/* Display as an integer but always round it down, e.g. 4 sec duration is shown as 3, 2, 1, 0 */
	ZeroDP_RoundUp,
	
	/* Display as an integer but round to closest integer <= 0.5 will be 0, > 0.5 will be 1, 
	e.g. 4 sec duration shown as 4, 3, 2, 1, 0 where 4 and 0 are only shown for 0.5 sec each */
	ZeroDP_RoundToNearest, 

	/* Display as a number with 1 decimal place of accuracy and round down. Trailing zeros are not chopped 
	e.g. a 4 sec duration shown as 3.9, 3.8, 3.7, ... 0.2, 0.1, 0.0 */
	OneDP_RoundDown_KeepTrailingZeros, 

	/* Display as a number with 1 decimal place of accuracy and round up. Trailing zeros are not chopped 
	e.g. a 4 sec duration shown as 4.0, 3.9, 3.8, ...  0.3, 0.2, 0.1 */
	OneDP_RoundUp_KeepTrailingZeros,

	/* Display as a number with 1 decimal place of accuracy and round to nearest. Trailing zeros 
	are not chopped 
	e.g. a 4 sec duration shown as 4.0, 3.9, 3.8, ... 0.2, 0.1, 0.0 where 4.0 and 0.0 are only 
	shown for 0.05 sec each */
	OneDP_RoundToNearest_KeepTrailingZeros,

	//=========================================================================================
	/* These 3 methods same as above expect they remove trailing zeros */
	
	/* e.g. a 4 sec duration is shown as 3.9, 3.8, 3.7, ... 0.2, 0.1, 0 */
	OneDP_RoundDown_DiscardTrailingZeros,
	
	/* e.g. a 4 sec duration is shown as 4, 3.9, 3.8, ... 0.3, 0.2, 0.1 */
	OneDP_RoundUp_DiscardTrailingZeros,
	
	/* e.g. a 4 sec duration is shown as 4, 3.9, 3.8, ... 0.2, 0.1, 0 where 4 and 0 are only shown 
	for 0.05 sec each */
	OneDP_RoundToNearest_DiscardTrailingZeros,

	//==========================================================================================
	//	Same as above expect these have 2 decimal places of precision

	TwoDP_RoundDown_KeepTrailingZeros,
	TwoDP_RoundUp_KeepTrailingZeros,
	TwoDP_RoundToNearest_KeepTrailingZeros,
	TwoDP_RoundDown_DiscardTrailingZeros,
	TwoDP_RoundUp_DiscardTrailingZeros,
	TwoDP_RoundToNearest_DiscardTrailingZeros
};


UENUM() 
enum class EMultiplierDisplayMethod : uint8
{
	/* e.g. 0.8f will show as 80% */
	Percentage_WithPercentageSymbol, 

	/* e.g. 0.8f will show as 80 */
	Percentage_NoPercentageSymbol, 
	
	/* e.g. 0.8f will show as 20% */
	InversePercentage_WithPercentageSymbol, 
	
	/* e.g. 0.8f will show as 20 */
	InversePercentage_NoPercentageSymbol
};


/* Needs a better name */
UENUM()
enum class ESingleCommandIssuingRule : uint8
{
	/* Issue the command to the first selectable found via array iteration. This is the most 
	performant option */
	First, 

	/* Issue command to the selectable that is closest to the target */
	Closest
};


/* When the player issues a command to go pick up an inventory item off the ground, what happens */
UENUM()
enum class EPickUpInventoryItemCommandIssuingRule : uint8
{
	/* Of all selected units a maximum of one will be given a command to go pick up the item.
	Only whether the selectable has an inventory or not will be checked before issuing the
	command */
	Single_QuickHasInventoryCheck,
	
	/* Of all selected units a maximum of one will be given a command to go pick up the item. 
	The chosen selectable must be able to pick up the item at the time of command issuing. 
	More costly performance wise than quick has inventory check */
	Single_CheckIfCanPickUp, 

	/* Of all selected units a maximum of one will be given a command to go pick up the item. 
	The chosen selectable must have an inventory. The rest of the selected selectables will be 
	told to move closish to the item */
	Single_QuickHasInventoryCheck_AndMoveEveryoneElse,

	/* Of all selected units a maximum of one will be given a command to go pick up the item. 
	The chosen selectable must be able to pick up the item. The rest of the selected selectables 
	will be told to move closish to the item. 
	More costly performance wise than quick has inventory check */
	Single_CheckIfCanPickUp_AndMoveEveryoneElse,
};


/* Rule for the ordering of buttons on the HUD persistent panel's tabs */
UENUM()
enum class EHUDPersistentTabButtonDisplayRule : uint8
{
	/* All these methods apply to both prereqs and queue support */
	
	//==========================================================================================
	/* These first two methods add all the buttons on setup */
	
	/* All buttons are added on setup. Buttons that that should not be clickable are made hidden */
	NoShuffling_HideUnavailables,
	
	/* Basically the same as NoShuffling_HideUnavailables but the opacity of the button can be 
	set instead of making it fully hidden. Using opacity as 0 basically makes it the same as 
	NoShuffling_HideUnavailables but I don't know whether there is a performance penalty for 
	choosing to use 0 opacity instead of just making it hidden so that is why this value exists */
	NoShuffling_FadeUnavailables,

	//===========================================================================================
	/* These next two methods add/remove buttons as they become available/unavailable */

	/* The order of the buttons will be the same every time. 
	E.g. if you have 10 items that appear in the tab and you give them priorties 1 through to 
	10. Then item 10 always comes after item 9, 9 after 8 etc. 
	An example: 
	You produce a building that lets you train items 1, 4, 6, and 10. 
	Then the tab will show 1, 4, 6, 10 in that order. 
	You then produce a building that can train 2 and 8. Now the tab will show 1, 2, 4, 6, 8, 10 
	in that order. */
	SameEveryTime, 

	/* I honestly can't remember but I think RA2 does it this way (and maybe every other C&C)
	Quick explanation: the order buildings are built affects the ordering of the buttons. 
	E.g. if you build a war factory you can build rhinos, terror drones, flak tracks 
	and war miners. Then if you build a barracks you can build constripts and attack dogs. On 
	the units tab it will show in this order: war miner, rhino, flak track, terror drone, 
	constricpt and finally attack dog. But if you were to build the barracks first then build 
	the war factory then it will show conscript, attack dog, war miner, rhino, flak track and 
	finally terror drone. So that's what this option is. I think units and vehicles tab is 
	actually seperate in RA2 so pretend I said war factory and shipyard instead of war factory
	and barracks. */
	OrderTheyBecomeAvailable
};


UENUM()
enum class EMarqueeBoxOutlinePositioningRule : uint8
{
	/* My implementation. The pixel you click on will be where the outer corner of the border 
	always is. When the border thickness is large then this method looks bad when switching 
	between lessthan/greaterthan the original click location for both X and Y axis. */
	Mine, 

	/* How Starcraft does it. Possibly how other RTSs do it, I haven't checked. 
	My notes: haven't actually tested whether this works as expected */
	Standard
};


//\\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
//=============================================================================================
//	Actual Stuff you can edit
//=============================================================================================
//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

/* Whether to use a minimap on HUD */
#define MINIMAP_ENABLED_GAME 1

/* If true then minimap's image will be calculated on game thread. If false ... I don't know */
#define MINIMAP_CALCULATED_ON_GAME_THREAD 1

/** 
 *	Whether to have buffs/debuffs in your game that expire after a certain amount of time 
 *	e.g. a 10 sec speed boost, a heal that heals 10 health every 2 sec for 6 sec, etc. Setting 
 *	this to 0 is an optimization.
 *
 *	This will not delete the timed buff/debuff definitions you have defined in your game 
 *	instance blueprint in case you change your mind about timed buffs/debuffs. 
 */
#define TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME 1

/* Whether to allow buffs/debuffs on buildings. This includes both static and tickable buffs/debuffs */
#define ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS 1


namespace ProjectSettings
{
	/**
	 *	Max number of players including CPU players. This should not be set to insanely high
	 *	unnecessary values because some arrays will use default allocators with this value. Changing
	 *	this may require adding more slots to your lobby widget. 
	 *	CPU player AI controllers tick rate is dependent on this number.
	 */
	constexpr uint8 MAX_NUM_PLAYERS = 4;

	/**
	 *	DO NOT MODIFY THIS VALUE DIRECTLY. 
	 *
	 *	To increase/decrease edit ETeam enum in CommonEnums.h instead. 
	 *
	 *	Use lower of:
	 *	- number of teams defined in ETeam enum
	 *	- max players defined in project options
	 *
	 *	To increase the amount of teams supported add new entries to the ETeam enum in CommonEnums.h.
	 *
	 *	Number of teams is limited by the number of custom collision channels allowed (18 minus 
	 *	some that may be used by this project for other things).
	 *
	 *	TODO get this variable out of this file. Although it's labelled "do not touch" I 
	 *	still don't like it here since
	 *
	 *	DO NOT MODIFY THIS VALUE DIRECTLY.
	 */
	constexpr uint8 MAX_NUM_TEAMS = FMath::Min<uint8>(static_cast<uint8>(ETeam::Neutral) - 1, ProjectSettings::MAX_NUM_PLAYERS);

	/** 
	 *	Maximum number of selectables per player. 
	 *
	 *	The reason this exists is because we send a single byte over the wire to identify 
	 *	selectable actors instead of sending raw pointers because I assume that the raw pointers 
	 *	are larger than 1 byte so this saves some bandwidth. You will likely want this as UINT8_MAX 
	 *	but if you want to impose some kind of limit lower than this you can lower this number.
	 *	Note 255 is the right amount not 256 since 0 is reserved to pickup on uninitialized
	 *	values + other things
	 *
	 *	TODO: make sure this is enforced on CPU players
	 */
	constexpr uint8 MAX_NUM_SELECTABLES_PER_PLAYER = UINT8_MAX;

	/** 
	 *	Currently this is only used for the regen of selectable resources like mana. 
	 *
	 *	The higher this value the less frequent mana is updated and the more "chunkier" the 
	 *	updates are.
	 *	
	 *	Whenever this amount of time passes a replicated variable in the game state will be 
	 *	incremented. To reduce bandwith and minimize chance of ARTSGameState::TickCounter 
	 *	overflowing in a bad way it is recommended to not set this lower than 0.1. With this 
	 *	limit it would take approx 25/2 seconds of a replicated variable never reaching clients 
	 *	before we run into issues. 
	 *
	 *	My notes: post edit needs to run after changing this, but that's on my agenda anyway
	 */
	constexpr float GAME_TICK_RATE = 0.2f; 
}


namespace FogOfWarOptions
{
	/* If true then fog of war is used in your game. If false then no fog of war will be used
	in your game. */
#define FOG_OF_WAR_ENABLED_GAME 1
	// TODO above bool needs a lot of implementing. Remember buildings and units rely on getting 
	// revealed by fog manager. When they spawn would need to check if fog is enabled. If not 
	// then should reveal self

	/** 
	 *	If true then fog of war will be calculated on a thread(s) seperate from the game thread. 
	 *	As a result it will likely be 1+ frames behind. Check out 
	 *	MultithreadedFogOfWarManager::GetNumFogOfWarThreadsToCreate to edit how many threads 
	 *	are created for fog of war.
	 */
#define MULTITHREADED_FOG_OF_WAR 0

	/**
	 *	The length and width of a fog of war tile.
	 *	Larger = better performance but fog of war is more 'chunker'
	 *
	 *	TODO After changing this have to make sure post edit runs on each building blueprint
	 */
	constexpr float FOG_TILE_SIZE = 64.f;
}


/* Options to do with selectables gaining experience and leveling up */
namespace LevelingUpOptions
{
	/** 
	 *	Max level/rank commander (commander == player) can reach. Commander's rank starts 
	 *	at 0. Set this to 0 to essentially disable commander leveling up. 
	 */
	constexpr uint8 MAX_COMMANDER_LEVEL = 5;

	/** 
	 *	@See - the selectable version below. This is the same thing except it's for commander 
	 *	experience 
	 */
	constexpr float COMMANDER_EXPERIENCE_GAIN_RANDOM_FACTOR = 0.2f;

	/** 
	 *	Whether selectables are capable of gaining experience and ultimately leveling up. If 
	 *	you set this to false then the rest of the options in this namespace are irrelevant. 
	 *	
	 *	You can safely toggle this value and any editor exposed variables that relate to experience 
	 *	and leveling up will remain intact in case you change your mind later. 
	 */
#define EXPERIENCE_ENABLED_GAME 1
	
	/* The level/rank units start at. Haven't tested whether numbers greater than 0 will work */
	constexpr uint8 STARTING_LEVEL = 0;

	/** 
	 *	Max level/rank unit can obtain. 
	 *	
	 *	Do not use UINT8_MAX but instead use at least 1 lower than it.
	 */
	constexpr uint8 MAX_LEVEL = 3;

	/** 
	 *	EXPGain *= FMath::RandRange(1 - this, 1 + this). 
	 *	Range = [0, 1]
	 *
	 *	Example values:
	 *	0 = no randomness to experience gain
	 *	0.25 = gain between 75% to 125% experience
	 */
	constexpr float SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR = 0.30f;
}


namespace PlayerOptions
{
	/* Max length of player's alias/nickname */
	const int32 MAX_ALIAS_LENGTH = 20;
}


namespace MapOptions
{
	/** 
	 *	These names need to be the names of the maps as they appear in editor 
	 *	e.g. Blank_Persistent, Entry, etc. Just name required, path not necessary. 
	 *
	 *	My notes: The implication of getting one of these wrong is that PIE will crash while 
	 *	standalone keeps running but lobby widget is invisible
	 */
	
	/* Name of map that is for main menu. Will need to be the level set as the default
	start up level in project settings in editor */
	const FName MAIN_MENU_LEVEL = FName("MainMenu");

	/* The name of the blank persistent level where all matches are played on. It's sub-levels
	should all be the maps you want to play matches on */
	const FName BLANK_PERSISTENT_LEVEL = FName("BlankPersistent");

	/* Name of map to stream in for lobby */
	const FName LOBBY_MAP_NAME = FName("Lobby");
}


/* Settings for creating a lobby */
namespace LobbyOptions
{
	//==========================================================================================
	//	Lobby Creation Options
	//==========================================================================================
	
	/** Texts to display on UI for different network types. Also appear when browsing lobbies */
	const FText LAN_TEXT = NSLOCTEXT("Lobby Creation", "LAN Text", "LAN");
	const FText ONLINE_TEXT = NSLOCTEXT("Lobby Creation", "Online Text", "Online");

	/** 
	 *	Max length a lobby name can be 
	 */
	const uint32 MAX_NAME_LENGTH = 50;

	/** 
	 *	Max length a lobby password can be 
	 */
	const uint32 MAX_PASSWORD_LENGTH = 20;

	//==========================================================================================
	//	Default values
	//==========================================================================================
	
	// Name to call singleplayer lobby
	const FString DEFAULT_SINGLEPLAYER_LOBBY_NAME = FString("nr10 singleplayer lobby");
	
	// Name to call multiplayer lobby
	const FText DEFAULT_MULTIPLAYER_LOBBY_NAME = NSLOCTEXT("Lobby Creation", "Default Multiplayer Lobby Name", "NR10");
	
	// Password for lobby
	const FText DEFAULT_PASSWORD = FText::GetEmpty();
	
	// Network type e.g. LAN, Online
	const FText DEFAULT_NETWORK_TYPE = ONLINE_TEXT;
	
	const uint8 DEFAULT_NUM_SLOTS = FMath::Min<uint8>(ProjectSettings::MAX_NUM_PLAYERS, 4);

	// Singleplayer lobby only: Default number of CPU players to put in lobby
	const uint8 DEFAULT_NUM_CPU_OPPONENTS = 1;

	/* This is based on TMap iteration */
	const uint8 DEFAULT_MAP_ID = 0;

	//-------------------------------------------------
	//	Enum Defaults
	//-------------------------------------------------

	/* The team anyone who joins the lobby will get */
	const ETeam DEFAULT_TEAM = ETeam::Team1;
	const EDefeatCondition DEFAULT_DEFEAT_CONDITION = EDefeatCondition::AllBuildingsDestroyed;
	const EStartingResourceAmount DEFAULT_STARTING_RESOURCES = EStartingResourceAmount::Medium;

	//==========================================================================================
	//	Options for once already inside lobby
	//==========================================================================================

	// Text to appear for different slot statuses
	const FText CPU_PLAYER_SLOT_NAME = NSLOCTEXT("Lobby", "CPU Player Text", "CPU");
	const FText OPEN_SLOT_NAME = NSLOCTEXT("Lobby", "Open Slot Text", "Open");
	const FText CLOSED_SLOT_NAME = NSLOCTEXT("Lobby", "Closed Slot Text", "Closed");

	// Default CPU player difficulty when a new CPU player is added
	const ECPUDifficulty DEFAULT_CPU_DIFFICULTY = ECPUDifficulty::Easy;
}


namespace CameraOptions
{
	/**
	 *	With regards to the players camera whether to adjust its height to the environments 
	 *	height. Requires a ray trace each tick. 
	 *
	 *	If you don't like the camera adjusting to the height of the landscape then you can 
	 *	turn this off. This is useful if your landscape has a lot of bumps in it and you don't 
	 *	like the camera changing its height everytime you go over them, but be aware that the 
	 *	camera height will stay the same hight that it was when the match started. You can still 
	 *	zoom in/out though.
	 *	Alternatively you can probably adjust a setting in the game user settings 
	 *	to at least slow the rate at which the camera changes its height (but this setting 
	 *	will also affect the left/right/up/down movement too unfortunately)
	 */
	const bool bGlueToGround = true;
}


namespace ControlOptions
{
	/** 
	 *	e.g. 
	 *	Starcraft II: you have clicked the button for a ghost's EMP and are now selecting the 
	 *	location for it. You hover your mouse over the button for stealth and click it. 
	 *	True = the EMP placement is cancelled and your ghost will go stealth. 
	 *	False = the EMP placement is not cancelled and your ghost will go stealth 
	 *	
	 *	Regardless of what this is set to if the ability you click cannot be used i.e. your ghost 
	 *	does not have enough energy to go stealth then the EMP will not be cancelled. 
	 *
	 *	Actions that can be cancelled: 
	 *	- ghost building placement
	 *	- selectable action bar abilities 
	 *	- inventory item use abilities 
	 *	- global skills panel abilities 
	 *
	 *	My notes: I think Build/Train/Upgrade buttons both on the selectable action bar and 
	 *	persistent panel have been excluded from this rule. Should probably implement this 
	 *	for them, could maybe make it a seperate bool 
	 */
	constexpr bool bInstantActionsCancelPendingOnes = true;
}


/* Settings for the ghost building */
namespace GhostOptions
{
	//==========================================================================================
	//	Button Click Checks
	//==========================================================================================
	//	These options are for checking certain things when a button that will spawn a ghost 
	//	building is clicked. 
	//
	//	These checks are optional. Bear in mind that whether true or false they will not allow 
	//	building buildings that are not allowed to be built. These are purely for preferences.
	//------------------------------------------------------------------------------------------
	
	/* Whether to check if at selectable cap when attempting to spawn ghost */
	constexpr bool bPerformChecksBeforeGhostSpawn_SelectableCap = true;

	/* Whether to check for building uniqueness when attempting to spawn ghost e.g. in C&C
	generals sometimes you are only allowed to build one nuke silo */
	constexpr bool bPerformChecksBeforeGhostSpawn_Uniqueness = true;

	/** 
	 *	Whether to check resources when attempting to spawn ghost
	 */
	constexpr bool bPerformChecksBeforeGhostSpawn_Resources = true;

	/** 
	 *	Whether to perform checks to see if there is a production queue that is actually free 
	 *	that can produce the building requested. 
	 *
	 *	This option is only relevant if the building will be built using the BuildsItself build 
	 *	method (and therefore the button is on the HUD persistent panel)
	 */
	constexpr bool bPerformChecksBeforeGhostSpawn_UnfullQueue = true;

	//===========================================================================================
	//	Rotation Methods
	//===========================================================================================

	//~ Begin different ghost rotation methods

	/**
	 *	No one will want this.
	 *	Moving mouse left = rotate ghost clockwise, mouse right = rotate counter-clockwise
	 *	Up and down movement are ignored.
	 */
#define SNAP_GHOST_ROT_METHOD 1 

	 /** The angle of the building will follow the mouse. Kind of like C&C Generals I think */
#define BASIC_GHOST_ROT_METHOD 2 

	 /** The default implementation */
#define STANDARD_GHOST_ROT_METHOD 3

	//~ End different ghost rotation methods

	/** 
	 *	The method that dictates how the ghost building rotates while the LMB is pressed. 
	 *	Choose from one of the 3 methods above 
	 */
#define GHOST_ROTATION_METHOD STANDARD_GHOST_ROT_METHOD

	//============================================================================================
	//	Settings for the "Standard" ghost rotation method
	//============================================================================================
	//	Should hardly need to touch these. Comments probably need updating
	//--------------------------------------------------------------------------------------------
	namespace StdRot
	{
		/* This is only relevant when mouse moves in such a way that the rotation direction 
		is not completely obvious. 
		Inner = the max angle mouse loc can be with ghost loc and not have rotation rate
		slowed. Once above inner then rotation rate falls off linearly and will be AngleMinFalloff 
		when at outer or above
		Range: [0, AngleLenienceOuter) */
		constexpr float AngleLenienceInner = 15.f;
		// Range: (AngleLenienceInner, 45]
		constexpr float AngleLenienceOuter = 40.f;
		/* What rotation should be multiplied by when at AngleLenienceOuter
		Range = [0, 1] */
		constexpr float AngleMinFalloffMultiplier = 0.4f;

		//======================================================================================
		//	These distance based variables may need to be dependent on screen resolution

		/* How low GhostAccumulatedMovementTowardsDirection must get before a direction
		change will happen. Kind of similar to MaxAccumulatedMovementTowardsDirection */
		constexpr float ChangeDirectionThreshold = -15.f;

		/* How low GhostAccumulatedMovementTowardsDirection must get before "NoRotation" 
		will be set meaning the ghost will not rotate until a direct has been established. 
		Should be equal to or larger than ChangeDirectionThreshold */
		constexpr float NoRotationThreshold = -5.f;

		/* Max value GhostAccumulatedMovementTowardsDirection can be. Higher values means
		more oppisite direction mouse movement is required to get ghost rotating in the
		other direction. Similar to ChangeDirectionThreshold */
		constexpr float MaxAccumulatedMovementTowardsDirection = 15.f;

		/*   @         @         @                      @                    @
		   Ghost     Inner     Middle                 Outer       PhysicalEdgeOfScreen
		   Mouse always lies in one of these 4 areas. 
		   - Between Ghost and Inner rot multi is linear between InnerDistanceMinFalloffMultiplier 
		   and 1. 
		   - Between Inner and Middle rot multi is 1. 
		   - Between Middle and Outer rot multi is linear between 1 and OuterDistanceMinFalloffMultiplier 
		   - Between Outer and PhysicalEdgeOfScreen rot multi is OuterDistanceMinFalloffMultiplier
		*/
		constexpr float FalloffDistanceInner = 100.f;
		constexpr float FalloffDistanceMiddle = 250.f;
		constexpr float FalloffDistanceOuter = 700.f;
		// Range [0, 1] @See AngleMinFalloffMultiplier
		constexpr float DistanceMinFalloffMultiplierInner = 0.5f;
		constexpr float DistanceMinFalloffMultiplierOuter = 0.2f;
	}
}


/* Settings for coloring selectables based off of their owner */
namespace SelectableColoringOptions
{
	/**
	 *	Whether to change the color of a selectable based on their owner or owner's team. 
	 *
	 *	Each selectable must opt-in to this by having a vector parameter named the variable 
	 *	below on any materials they want to be affected
	 */
	constexpr ESelectableColoringRule Rule = ESelectableColoringRule::Player;

	/**
	 *	The name of the vector parameter to edit for SelectableColoringRule. Any selectable's
	 *	materials that have a vector parameter named this will be affected by the rule
	 */
	const FName ParamName = FName("RTS_Color");

	/**
	 *	The colors to use. Used for both ESelectableColoringRule::Player and
	 *	ESelectableColoringRule::Team. 
	 */
	const FLinearColor Colors[(Rule == ESelectableColoringRule::Player)
		? ProjectSettings::MAX_NUM_PLAYERS : ProjectSettings::MAX_NUM_TEAMS] = {
		FLinearColor(1.0f, 0.05f, 0.05f, 1.f),
		FLinearColor(0.05f, 0.05f, 1.0f, 1.f),
		FLinearColor(0.05f, 1.0f, 0.05f, 1.f),
		FLinearColor(1.0f, 1.0f, 0.05f, 1.f),
		//FLinearColor(0.1f, 0.7f, 0.8f, 1.f),
		//FLinearColor(0.9f, 0.9f, 0.9f, 1.f),
		//FLinearColor(0.8f, 0.46f, 0.74f, 1.f),
		//FLinearColor(0.55f, 0.8f, 0.45f, 1.f)
	};
}


namespace CommandIssuingOptions
{
	/** 
	 *	This setting is relevant only for abilities that:
	 *	1. require selecting a target/location 
	 *	2. are only ever issued to one selectable at a time. 
	 *	This rule says who gets the command. 
	 */
	constexpr ESingleCommandIssuingRule SingleIssueSelectionRule = ESingleCommandIssuingRule::First;

	/** 
	 *	Only releveant when multiple selectables are selected and we right-click on an inventory 
	 *	item that has been placed in the world. This rule says how we issue that command. 
	 *	If a "Single" type value is selected here then it will also obey the SingleIssueSelectionRule 
	 *	defined above. Currently all of them are single types. 
	 */
	constexpr EPickUpInventoryItemCommandIssuingRule PickUpInventoryItemRule = EPickUpInventoryItemCommandIssuingRule::Single_CheckIfCanPickUp;
}


namespace UIOptions
{
	/* Rule governing how a selectable's current health value is displayed on the UI */
	constexpr EFloatDisplayMethod CurrentHealthDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundUp;

	/* Rule governing how a selectable's max health value is displayed on the UI */
	constexpr EFloatDisplayMethod MaxHealthDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundUp;

	/* Current amount of selectable resource (mana) */
	constexpr EFloatDisplayMethod SelectableResourceAmountDisplayMethod = EFloatDisplayMethod::OneDP_RoundDown_KeepTrailingZeros;

	/** 
	 *	Rule governing how a selectable's selectable resource regeneration rate is displayed. 
	 *	By default the UI will show amount regenerated per second but this is easy to change. 
	 *	
	 *	Selectable resources are things like mana or energy 
	 */
	constexpr EFloatDisplayMethod SelectableResourceRegenRateDisplayMethod = EFloatDisplayMethod::OneDP_RoundToNearest_KeepTrailingZeros;

	/* Rule governing how a selectable's attack damage is displayed on the UI. */
	constexpr EFloatDisplayMethod DamageDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundToNearest;

	/* Rule governing how a selectable's attack rate is displayed on the UI */
	constexpr EFloatDisplayMethod AttackRateDisplayMethod = EFloatDisplayMethod::TwoDP_RoundToNearest_KeepTrailingZeros;

	/* Rule governing how a selectable's attack range is displayed on the UI */
	constexpr EFloatDisplayMethod AttackRangeDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundToNearest;

	/* Rule governing how a selectables's defense multiplier should be displayed on the UI. 
	E.g. DefenseMultiplier = 0.8f. So we have a 20% damage reduction. What text should we show 
	to represent this */
	constexpr EMultiplierDisplayMethod DefenseMultiplierSpecificDisplayMethod = EMultiplierDisplayMethod::InversePercentage_WithPercentageSymbol;

	/* Rule governing how a selectable's defense multiplier is displayed on the UI in regards 
	to rounding values */
	constexpr EFloatDisplayMethod DefenseMultiplierDisplayMethod = EFloatDisplayMethod::TwoDP_RoundToNearest_KeepTrailingZeros;

	/* Rule governing how to display the amount of time left before an ability comes off 
	cooldown */
	constexpr EFloatDisplayMethod AbilityCooldownDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundDown;

	/* How to display the floats that are the cooldown remaining for an inventory item use */
	constexpr EFloatDisplayMethod ItemUseCooldownDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundDown;

	/* Rule governing how to display the percentage of construction complete of a building */
	constexpr EFloatDisplayMethod ConstructionProgressDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundDown;

	/* Rule governing how to display an experience value that is the amount gained towards 
	the next rank */
	constexpr EFloatDisplayMethod ExperienceGainedDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundDown;

	/* Rule governing how to display an experience value that is how much we need to have to reach 
	the next rank */
	constexpr EFloatDisplayMethod ExperienceRequiredDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundUp;

	/* How to display player experience aquired */
	constexpr EFloatDisplayMethod PlayerExperienceGainedDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundDown;

	/* How to display player experience required */
	constexpr EFloatDisplayMethod PlayerExperienceRequiredDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundUp;

	/* How to display volume */
	constexpr EFloatDisplayMethod AudioVolumeDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundToNearest;
}


/* I mean HUD is a part of UI, so perhaps make this a nested namespace inside UIOptions */
namespace HUDOptions
{
	/* Rule governing the ordering of buttons in the persistent panel's tabs */
	constexpr EHUDPersistentTabButtonDisplayRule PersistentTabButtonDisplayRule = EHUDPersistentTabButtonDisplayRule::NoShuffling_FadeUnavailables;

	/* In regards to persistent tab buttons, text to appear on the button when production of a 
	BuildsInTab building has completed and it is now ready to be placed */
	const FText TEXT_READY_TO_PLACE = NSLOCTEXT("Persistent Tab Button", "Ready to Place", "Ready");

	/* How much to multiply original opacity by when the shop slot runs out of purchases */
	constexpr float SHOP_SLOT_OUT_OF_PURCHASES_OPACITY_MULTIPLIER = 0.5f;

	/* This setting is getting really picky, why is this even here? 
	This option is only relevant if you are choosing to draw an outline for your marquee 
	selection box. This option says how it is positioned */
	constexpr EMarqueeBoxOutlinePositioningRule MarqueeBoxPositioningRule = EMarqueeBoxOutlinePositioningRule::Mine;
}


namespace TooltipOptions
{
	//=====================================================================================
	//	Delays showing tooltips and associated variables
	//=====================================================================================

	//-------------------------------------------------------------------------------------
	//	UI Elements
	//-------------------------------------------------------------------------------------

	/** 
	 *	Whether tooltips for UI elements should show instantly when you hover your mouse over them. 
	 *	If this is true then the next 5 variables are irrelevant 
	 */
#define INSTANT_SHOWING_UI_ELEMENT_TOOLTIPS 0
	
	/**
	 *	Only relevant if UI tooltips don't show instantly.
	 *	How long the mouse should be hovered over a UI element before its tooltip will be shown.
	 */
	constexpr float UI_ELEMENT_HOVER_TIME_REQUIREMENT = 0.3f;

	/**
	 *	Only relevant if UI tooltips don't show instantly. 
	 *	Whether to use a decay delay.
	 */
#define USING_UI_ELEMENT_HOVER_TIME_DECAY_DELAY 1

	/**
	 *	Only relevant if UI tooltips don't show instantly.
	 *	How long after the mouse stops hovering a UI element before accumulated hover time starts 
	 *	to decay.
	 *
	 *	0 = start decaying it the moment a UI element becomes unhovered
	 */
	constexpr float UI_ELEMENT_HOVER_TIME_DECAY_DELAY = 0.4f;

	/** 
	 *	Only relevant if UI tooltips don't show instantly.
	 *	How fast accumulated hover time decays once the delay has passed.
	 */
	constexpr float UI_ELEMENT_HOVER_TIME_DECAY_RATE = 1.f;

	/** 
	 *	Only relevant if UI tooltips don't show instantly.
	 *	When switching between UI elements how much to subtract from accumulated hover time. 
	 */
	constexpr float UI_ELEMENT_HOVER_TIME_SWITCH_PENALTY = 0.f;

	//-------------------------------------------------------------------------------------
	//	Selectables e.g. inventory items that have been dropped on the ground
	//-------------------------------------------------------------------------------------

	/** 
	 *	When the player hovers their mouse over a tooltip showing selectable, whether the 
	 *	tooltip for it should show instantly.
	 *	If this is true then the next 5 variables are irrelevant
	 */
#define INSTANT_SHOWING_SELECTABLE_TOOLTIPS 0

	/**
	 *	Only relevant if selectable tooltips don't show instantly.
	 *	How long the mouse should be hovered over the inventory item before its tooltip will be shown.
	 */
	constexpr float SELECTABLE_HOVER_TIME_REQUIREMENT = 0.3f;

	/**
	 *	Only relevant if selectable tooltips don't show instantly.
	 *	Whether to use a decay delay.
	 */
#define USING_SELECTABLE_HOVER_TIME_DECAY_DELAY 1

	/**
	 *	Only relevant if selectable tooltips don't show instantly.
	 *	Decay delay amount.
	 */
	constexpr float SELECTABLE_HOVER_TIME_DECAY_DELAY = 0.2f;

	/**
	 *	Only relevant if selectable tooltips don't show instantly.
	 *	How fast accumulated hover time decays once the delay has passed.
	 */
	constexpr float SELECTABLE_HOVER_TIME_DECAY_RATE = 1.f;

	/**
	 *	Only relevant if selectable tooltips don't show instantly.
	 *	When switching between selectables how much to subtract from accumulated hover time.
	 */
	constexpr float SELECTABLE_HOVER_TIME_SWITCH_PENALTY = 0.f;


	//====================================================================================
	//	Other stuff
	//====================================================================================

	/* How to display the float that is for the total production time of a building/unit/upgrade */
	constexpr EFloatDisplayMethod ProductionTimeDisplayMethod = EFloatDisplayMethod::ZeroDP_RoundToNearest;
}


namespace InventoryItemOptions
{
	/** 
	 *	This variable says who can see a selectable's inventory.
	 *	
	 *	Owned = only the owning player can see what is in a selectable's inventory 
	 *	Allied = the owning player + their allies can see what is in a selectable's inventory 
	 *	Hostile = the owning player + their allies + enemies can see what is in a selectable's inventory 	
	 *
	 *	No matter what this variable is set to only the owning player can move/drop/sell/use etc 
	 *	the items in a selectable's inventory, but it is possible to have abilities or 
	 *	whatnot manipulate a selectable's inventory.
	 *
	 *	This variable has nothing to do with items for display and/or sale in shops. 
	 */
	constexpr EAffiliation WorstAffiliationThatCanSeeInventory = EAffiliation::Hostile;

	/* How close a selectable has to be to a shop to be able to sell items to it. 
	This should really be on each shop instead of as one global variable TODO */
	constexpr float SELL_TO_SHOP_DISTANCE = 600.f;

	/* How much of the default cost of item you get back when you sell it to a shop. 
	Possibly this too could go on each shop instead of being one global variable */
	constexpr float REFUND_RATE = 0.5f;
}


/* Building garrison options */
namespace GarrisonOptions
{
	/**
	 *	Who can see the units inside a building garrison
	 *
	 *	e.g. 
	 *	Owned = you can only see the units garrisoned inside buildings you own 
	 *	Allied = you can see inside buildings you own and buildings your allies own 
	 *	Hostile = you can see inside buildings you own and buildings your allies own and buildings 
	 *	your enemies own 
	 *
	 *	My notes: Maybe this variable is better suited on each garrison instead
	 */
	constexpr EAffiliation HighestAffiliationAllowedToSeeInsideOf = EAffiliation::Owned;
}


namespace BuffAndDebuffOptions
{
	/* Rule governing how to display the remaining duration of a buff/debuff as a number on the 
	UI */
	constexpr EFloatDisplayMethod DurationDisplayMethod = EFloatDisplayMethod::OneDP_RoundDown_KeepTrailingZeros;
}


//================================================================================================
// Everything below here do not touch
//================================================================================================

//================================================================================================
//	------- Not settings - Do not touch. Doesn't belong in this file in the first place -------
//================================================================================================

/** 
 *	Macro for converting a float to an FText
 *	
 *	@param Float - the float to convert to FText 
 *	@param ConversionMethod - a EFloatDisplayMethod to use 
 *	Returns float as FText
 */
#define FloatToText(Float, ConversionMethod) \
\
FNumberFormattingOptions NumberFormat; \
NumberFormat.SetMinimumIntegralDigits(1); \
NumberFormat.SetMaximumIntegralDigits(9); \
\
if (ConversionMethod == EFloatDisplayMethod::ZeroDP_RoundDown) \
{ \
	NumberFormat.SetMaximumFractionalDigits(0); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToNegativeInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::ZeroDP_RoundUp) \
{ \
	NumberFormat.SetMaximumFractionalDigits(0); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToPositiveInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::ZeroDP_RoundToNearest) \
{ \
	NumberFormat.SetMaximumFractionalDigits(0); \
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToZero); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundDown_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(1); \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToNegativeInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundUp_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(1); \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToPositiveInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundToNearest_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(1); \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToZero); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundDown_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToNegativeInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundUp_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToPositiveInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::OneDP_RoundToNearest_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(1); \
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToZero); \
} \
\
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundDown_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(2); \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToNegativeInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundUp_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(2); \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToPositiveInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundToNearest_KeepTrailingZeros) \
{ \
	NumberFormat.SetMinimumFractionalDigits(2); \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToZero); \
} \
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundDown_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToNegativeInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundUp_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::ToPositiveInfinity); \
} \
else if (ConversionMethod == EFloatDisplayMethod::TwoDP_RoundToNearest_DiscardTrailingZeros) \
{ \
	NumberFormat.SetMaximumFractionalDigits(2); \
	NumberFormat.SetRoundingMode(ERoundingMode::HalfToZero); \
} \
\
return FText::AsNumber(Float, &NumberFormat);


//==============================================================================================
//	------- Not a setting to be edited - do not touch -------
//==============================================================================================

#if FOG_OF_WAR_ENABLED_GAME
#if MULTITHREADED_FOG_OF_WAR
#define GAME_THREAD_FOG_OF_WAR 0
#else
#define GAME_THREAD_FOG_OF_WAR 1
#endif
#else
#undef MULTITHREADED_FOG_OF_WAR
#define GAME_THREAD_FOG_OF_WAR 0
#define MULTITHREADED_FOG_OF_WAR 0
#endif

/* This doesn't need to happen. But I just want to see where in code it is used so I can adjust 
for it if necessary. */
#undef FOG_OF_WAR_ENABLED_GAME



//==============================================================================================
//	------- Static Asserts -------
//==============================================================================================

static_assert(LevelingUpOptions::STARTING_LEVEL <= LevelingUpOptions::MAX_LEVEL,
	"LevelingUpOptions: selectables cannot start at a level higher than max level.");

static_assert(LevelingUpOptions::MAX_LEVEL < UINT8_MAX,
	"Must be at least 1 less than UINT8_MAX. See comment above LevelingUpOptions::MAX_LEVEL.");

// Not really that serious of an assert, more just because it is intuitive to use one of these 3 values
static_assert(InventoryItemOptions::WorstAffiliationThatCanSeeInventory == EAffiliation::Owned
	|| InventoryItemOptions::WorstAffiliationThatCanSeeInventory == EAffiliation::Allied
	|| InventoryItemOptions::WorstAffiliationThatCanSeeInventory == EAffiliation::Hostile,
	"InventoryItemOptions::WorstAffiliationThatCanSeeInventory must be one of either \"Owned\", "
	"\"Allied\" or \"Hostile\". Please change this.");

static_assert(LevelingUpOptions::SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR >= 0.f 
	&& LevelingUpOptions::SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR <= 1.f,
	"SELECTABLE_EXPERIENCE_GAIN_RANDOM_FACTOR must be between 0 and 1");

static_assert(TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_DELAY >= 0.f, "Must be at least zero");
static_assert(TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_RATE > 0.f, "Must be greater than zero");
static_assert(TooltipOptions::UI_ELEMENT_HOVER_TIME_DECAY_RATE <= 10.f, "Keep this below 10 to avoid "
	"any chance of overflow");
static_assert(TooltipOptions::UI_ELEMENT_HOVER_TIME_SWITCH_PENALTY >= 0.f, "Must be equal to or "
	"greater than zero");

static_assert(TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_DELAY >= 0.f, "Must be at least zero");
static_assert(TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE > 0.f, "Must be greater than zero");
static_assert(TooltipOptions::SELECTABLE_HOVER_TIME_DECAY_RATE <= 10.f, "Keep this below 10 to avoid "
	"any chance of overflow");
static_assert(TooltipOptions::SELECTABLE_HOVER_TIME_SWITCH_PENALTY >= 0.f, "Must be equal to or "
	"greater than zero");
