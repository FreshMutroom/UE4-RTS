// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


//==============================================================================================
//	Other Enums. Enums that will rarely need to have values added to them
//==============================================================================================

/* Type of player in a lobby slot */
UENUM()
enum class ELobbySlotStatus : uint8
{
	/* Default value. Never to be used */
	JustInitialized,

	/* Another human player */
	Human,

	/* Computer controlled player */
	CPU,

	/* Slot is open. Players can occupy it */
	Open,

	/* Slot cannot be occupied. Will be made invisible */
	Closed,
};


/* Type of match */
UENUM()
enum class EMatchType : uint8
{
	None,
	Offline,
	LAN,
	SteamOnline
};


/* Status of loading a match */
UENUM()
enum class ELoadingStatus : uint8
{
	None    UMETA(Hidden),

	/* Before new persistent map has even been loaded - game mode has not switched yet */
	LoadingPersistentMap,

	//~ These 3 happen concurrently

	/* Waiting for all human players to reach game modes PostLogin. Because maps are streamed in
	and no travel is used this will likely never to set */
	WaitingForAllPlayersToConnect,
	/* Waiting for all players to signal they have done their player controller's
	Client_SetupForMatch */
	WaitingForPlayerControllerClientSetupForMatchAcknowledgementFromAllPlayers,
	/* Waiting for all players to stream in the sub-level */
	WaitingForAllPlayersToStreamInLevel,

	/* Waiting for all players to say they have received every players player state info
	including CPU player state info */
	WaitingForInitialValuesAcknowledgementFromAllPlayers,

	/* Waiting for all AMyPlayerState::Client_FinalSetup to complete */
	WaitingForFinalSetupAcks,

	/* Spawn the starting buildings/units for each player. Once this is done each players
	input is enabled and the match starts */
	SpawningStartingSelectables,

	/* When showing a black screen for one second before match starts. */
	ShowingBlackScreenRightBeforeMatchStart,

	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};


/* Whether a selectable is hostile, friendly etc. Can be derived from
other values but added for readability */
UENUM()
enum class EAffiliation : uint8
{
	/* Note: if adding values in here consider UStatics::AffiliationToCommandTargetType */

	/* Another note: possibly do not edit the the ordering of these values. For one I use 
	an EAffiliation variable and operator<= to decide who can shop there */

	/* Cannot derive affiliation yet */
	Unknown    UMETA(Hidden),

	/* Built by you. Under your control */
	Owned,

	/* Built by a player you're allied with */
	Allied,

	/* Not controlled by any player */
	Neutral,

	/* Built by a player you are not allied with */
	Hostile,

	//~ Should always be 2nd last in enum
	/* Local player is an observer of match, not actually playing */
	Observed,

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


/* Menu widgets only */
UENUM()
enum class EWidgetType : uint8
{
	//~ If here then widget appears under everything

	/* A lobby for either single or multiplayer */
	Lobby,
	
	/* Widget that appears for password entry */
	PasswordEntry,
	
	/* The 'Are you sure you want to exit lobby?' widget */
	ConfirmExitFromLobby,
	
	/* The screen for setting up a multiplayer lobby */
	LobbyCreationScreen,
	
	/* Widget for when selecting a map to play on */
	MapSelectionScreen,
	
	/* Networked game browser */
	LobbyBrowser,
	
	MainMenu,
	
	/* Menu for video/audio/game/control settings */
	Settings,
	
	/* The screen to adjust things like nickname etc */
	Profile,
	
	/* Widget that shows warning/error messages like "lobby full" */
	GameMessage,
	
	/* The widget that lets user enter their nickname */
	NicknameEntryScreen,
	
	/* Menu that appears after pessing 'Play' from the main menu */
	PlayMenu,
	
	/* Screen for loading a match */
	LoadingScreen,
	
	/* 'Are you sure you want to exit to OS?' widget */
	ConfirmExitToOS,

	// If here then widget appears above everything

	// This is always last in the enum
	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};


/* Widgets that appear during a match */
UENUM()
enum class EMatchWidgetType : uint8
{
	None	UMETA(Hidden),

	//~ Lowest Z order here

	/* Displays things like current resources */
	HUD,

	/* _Ver2 can be deleted from name. Note Z ordering via enum value means nothing for this widget. 
	See HUDStatics::COMMANDER_SKILL_TREE_Z_ORDER instead */
	CommanderSkillTree_Ver2,

	/* Screen that will only appear if you have been defeated but the match is still going */
	Defeat,
	
	/* Screen that appears when the match has finished */
	EndOfMatch,
	
	/* Menu that shows when you press pause button */
	PauseMenu,
	
	/* Settings menu like video, audio etc accessable from the pause menu */
	Settings,
	
	/* The 'are you sure you want to return to main menu?' widget */
	ConfirmReturnToMainMenu,

	//~ Highest Z order here

	z_ALWAYS_3RD_LAST_IN_ENUM    UMETA(Hidden), 

	/* Has cheat actions for development */
	DevelopmentCheats    UMETA(Hidden), 

	/* Popup widget used by development cheats widget */
	DevelopmentCheatsPopupMenu   UMETA(Hidden)
};


/* Type of queue to produce something in */
UENUM()
enum class EProductionQueueType : uint8
{
	// Default value. Never to be used 
	None,

	/* Queue that is only used when producing buildings from the HUD persistent panel. */
	Persistent,

	/* Queue that is used when producing either units or upgrades */
	Context
};


/* Different types of messages that can be displayed to the HUD */
UENUM()
enum class EMessageType : uint8
{
	None   UMETA(Hidden),

	GameNotification,
	GameWarning,

	z_ALWAYS_LAST_IN_ENUM
};


/* Notifications that happen during game */
UENUM()
enum class EGameNotification : uint8
{
	None   UMETA(Hidden),

	/* Resources at a resource spot have depleted */
	ResourceSpotDepleted,

	/* A player has been defeated */
	PlayerDefeated,

	NukeLaunchedByOurselves, 
	NukeLaunchedByAllies, 
	NukeLaunchedByHostiles,

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


UENUM()
enum class EGameWarning : uint8
{
	/* This should not get a message assigned to it */
	None    UMETA(Hidden),

	/* Action is on cooldown */
	ActionOnCooldown,

	/* Not enough mana/energy/whatever to use an ability. This is a generalized message. 
	I should possibly remove this and add more specific messages like NotEnoughMana and 
	NotEnoughEnergy so users can be more specific with their warning messages. Will probably 
	require creating yet another overload for PS::OnGameWarningHappened that takes a 
	ESelectableResourceType */
	NotEnoughSelectableResources,

	/* The prerequisites are not met yet */
	PrerequisitesNotMet,

	/* Same situation as trying to add more than 5 units to a queue in Starcraft II */
	TrainingQueueFull,

	/* For upgrades only. If trying to click a button to research something and it is already
	being researched. */
	AlreadyBeingResearched,

	/* Upgrade cannot be researched anymore */
	FullyResearched,

	//~ Begin building placement warnings

	/* Trying to place ghost building inside fog of war */
	Building_InsideFog,

	/* Trying to place ghost but another building/unit is in the way */
	Building_SelectableInTheWay,

	/* Trying to place building but building is too far away from other owned/allied buildings */
	Building_NotCloseEnoughToBase,

	/* Trying to place building but the ground is not flat enough */
	Building_GroundNotFlatEnough,

	//~ End building placement warnings

	/* Basically means you don't have a construction yard type building yet you are trying to
	build a building. Should only appear as a message from the server. Otherwise clients should
	not be allowed to click the button in the first place */
	CannotProduce,

	/* For HUD persistent panel only. Means that you are already building something and cannot
	build any more */
	BuildingInProgress,

	/* When trying to place a building using BuildsInTab but the server says it's not actually ready yet even
	though it was on the client. Only happens because of timer drift and is expected to be rare */
	BuildingNotReadyYet,

	/* The internal selectable cap has been reached, which is usually around the size of UINT8_MAX.
	Have to wait until a selectable is destroyed before another can be built */
	InternalSelectableCapReached,

	/* The faction has a limit on the quantity of a building and you are trying to build 
	another that would send you over the limit e.g. in RA2 you are only allowed to build one 
	weather device */
	BuildingTypeQuantityLimit,

	/* The faction has a limit on the the qunatity of a unit and you are trying to build another 
	that would send you over the limit e.g. in RA2 you are only allowed to build one tanya */
	UnitTypeQuantityLimit,

	/* Target is not an acceptable target for ability */
	InvalidTarget,

	AbilityCannotTargetHostiles, 

	AbilityCannotTargetFriendlies,

	/* Broad message when cannot use an ability */
	CannotUseAbility,

	/* No target hovered for an ability that requires a target */
	NoTarget,

	/* A unit is trying to target itself with an ability but this is not allowed */
	AbilityCannotTargetSelf,

	/* Ability is out of range */
	AbilityOutOfRange,

	/* Tried to place an ability inside fog of war but we are not allowed to do that */
	AbilityLocationInsideFog,

	/* Added for items. Trying to use an item but the selectable that is ment to use it is no 
	longer alive */
	UserNoLongerAlive,

	/* Rare but can happen for remote clients. This is when they try to issue a context command 
	but server says the units are now invalid */
	UserNoLongerValid,

	TargetNoLongerAlive,

	/* Right-clicked on an inventory item in the world but nothing selected can pick it up */
	NothingSelectedCanPickUpItem,

	/* The unit trying to pick up or purchase an item cannot because the item type does not 
	allow the unit type to do so */
	TypeCannotAquireItem,

	/* Rare but basically remote client requested doing something to a target that the server 
	says they can't actually see */
	TargetNotVisible,

	/* Another pretty rare warning as of right now. Kind of a modified client alert if this happens */
	SelectionNotUnderYourControl,

	/* For Tried to place a building but the builder was destroyed. Can probably code to avoid this
	situation but is here for now */
	BuilderDestroyed,

	/* Remote client trying to purchase from a shop that is no longer valid. Rare warning */
	NotValid_Shop,

	/* Trying to buy inventory item in shop but item can only be browsed not bought */
	ItemNotForSale,

	/* Inventory item in shop has a limited quantity and has sold out */
	ItemSoldOut, 

	/* Trying to use an inventory item but it's used up all it's charges */
	ItemOutOfCharges,

	/* Trying to use an inventory item but it is no longer in inventory. This warning will only 
	be seen if you implement abilities/effects that can remove items from inventories */
	ItemNoLongerInInventory,

	/* Tried to pick up an inventory item in the world but it is no longer there */
	ItemNoLongerInWorld,

	/* Trying to use an inventory item but it is on cooldown */
	ItemOnCooldown,

	/* This one will likely only come from the server. It could be evidience of a modified 
	client but can also be the result of inventory being modified on server before RPC has 
	arrived to client */
	ItemNotUsable, 

	/* Trying to purchase an item from a shop but no selectable is in range to accept the item */
	NotInRangeToPurchaseFromShop,

	/* Trying to sell an item but no shop is in range */
	NotInRangeToSellToShop,

	/* Trying to sell an item but it is the type of item that cannot be sold */
	ItemCannotBeSold,

	/* Trying to buy an item from a shop but everyone that is in range has a full inventory */
	InventoryFull, 

	/* Trying to put an item in inventory but we are not allowed to carry anymore of it. Means 
	the item is unique or something */
	CannotCarryAnymoreOfItem, 

	/* The player is tryingto gain another rank for an ability but has already gained the max 
	rank of the ability */
	CommanderSkillTree_MaxAbilityRankObtained,

	/* The player is trying to aquire an ability but their rank is not high enough */
	CommanderSkillTree_CommanderRankNotHighEnough,

	/* The player is trying to aquire an ability but the prerequisites have not been aquired */
	CommanderSkillTree_PrerequisitesNotMet,

	/* The player is trying to aquire an ability but they cannot afford its skill point cost */
	CommanderSkillTree_NotEnoughSkillPoints,

	/* Ability has a limited number of charges and cannot be used anymore. This was added for 
	commander abilities */
	AllAbilityChargesUsed,

	/* Trying to target a player with an ability but they have been defeated */
	TargetPlayerHasBeenDefeated,

	/* Trying to target a hostile player but the ability does not allow this */
	TargetPlayerIsHostile, 

	/* Trying to target an allied player but the ability does not allow this */
	TargetPlayerIsAllied, 

	/* Trying to target ourself (meaning the player) but the ability does not allow this */
	TargetPlayerIsSelf,

	/* Trying to remap a game action to some key but the action isn't allowed to be remapped */
	NotAllowedToRemapAction,

	/* Trying to remap a game action to another key. However that key is mapped to some other 
	action that isn't allowed to be changed ever */
	WouldUnbindUnremappableAction,

	z_ALWAYS_2ND_LAST_IN_ENUM   UMETA(Hidden), 

	NotEnoughResources    UMETA(Hidden)
};


/* The way a building is built */
UENUM()
enum class EBuildingBuildMethod : uint8
{
	// Not to be used
	None    UMETA(Hidden),

	/* The first 2 methods are more suited for HUD persistent tab while the last 3 are suited for
	context menus, although for something like a city builder game LayFoundationsInstantly could
	be used - the foundations get placed and the player must order a unit to go work on it */

	/* Like classic Command and Conquer games or Allies in Red Alert 3. Building builds in your
	HUD and when complete you can place it somewhere at which point it will construct very quickly
	(or however slow you want it to, use FBuildingAttributes::ConstructTime to change this) */
	BuildsInTab,

	/* Like Soviets in Red Alert 3. Building is placed in world and builds itself */
	BuildsItself,

	/* Like Command and Conquer Generals. The foundation of the building is placed the moment the
	order is given. When the builder unit reaches the foundation it will start constructing the
	building */
	LayFoundationsInstantly,

	/* Like Terran and Zerg in Starcraft II. When builder unit gets to the build location they lay
	the foundation and start constructing it */
	LayFoundationsWhenAtLocation,

	/* Like Protoss in Starcraft II. Requires the worker to get to order location, then building
	builds itself. Possibly can be merged with BuildsItself unless buildings context menus can
	build other buildings from it */
	Protoss


	/* TODO: before doing this change all attribute properties to use category RTS, then change
	attribute declarations to use meta that makes them auto expanded like building attributtes */

	// 1, 2 and 5 can be placed and do not need build % updates ever. 3 and 4 will need updates 
	// when the building gets into view only - then start a TH client-side and use that.
	// Also build speed upgrades will be an issue. Probably gonna just use replicated build % 
	// var but update it less frequently than each tick

	/* In UStatics::SpawnBuilding need to pass in build type. It will determine start location
	of building. In buildings begin play have it set its location based on the build mode and
	if  mode 1, 2 or 5 then it does not  need updates on build %. If modes 3 or 4 then a
	build % repped variable will need to be added. It will be updated from server on regualr
	intervals but not every tick (too many updates). Clients will continue production progress
	between updates */
};


/* Rule for showing unit rally point on building */
UENUM()
enum class EBuildingRallyPointDisplayRule : uint8
{
	// Show even when being constructed
	Always,

	// Do not show when being constructed
	OnlyWhenFullyConstructed,
};


/* The type of a selectable */
UENUM()
enum class ESelectableType : uint8
{
	Building,
	Unit, 
	InventoryItem
};


/* Status of a fog of war tile for a team */
UENUM()
enum class EFogStatus : uint8
{
	/* Stuff on this tile not visible */
	Hidden = 0x00,

	/* Stuff on this tile visible */
	Revealed = 0x01,

	/* Stuff on this tile close enough to a stealth detector to be seen but tile
	is not actually revealed by regular sight. Only used by fog of war manager. No
	visible difference from Hidden to players */
	StealthDetected = 0x02,

	/* Stuff on this tile revealed even if stealthed */
	StealthRevealed = 0x03
};


/* Result of match */
UENUM()
enum class EMatchResult : uint8
{
	// Default value, never to be used
	None,

	Defeat,
	Draw,
	Victory
};


/* How to make mouse look when choosing location for a context button ability */
UENUM()
enum class EAbilityMouseAppearance : uint8
{
	/* Mouse cursor will not change when using this ability */
	NoChange,

	/* Show a custom mouse cursor. At least default cursor will need a valid path while the other
	2 are optional */
	CustomMouseCursor,

	/* Hide the mouse cursor and draw a decal onto the world */
	HideAndShowDecal
};


/* Different types of right click commands targets */
UENUM()
enum class ECommandTargetType : uint8
{
	/* Note: if adding values in here consider UStatics::AffiliationToCommandTargetType */

	/* No target. The ground for example */
	NoTarget,

	OwnedSelectable,
	AlliedSelectable,
	NeutralSelectable,
	HostileSelectable,

	//~ Should always be 2nd last in enum
	ObservedSelectable,

	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};


/* Who will receive an in-match chat message */
UENUM()
enum class EMessageRecipients : uint8
{
	None    UMETA(Hidden),

	// Only people on our own team
	TeamOnly,

	// Everyone in the match 
	Everyone,

	z_ALWAYS_LAST_IN_ENUM    UMETA(Hidden)
};


UENUM()
enum class EMarqueeBoxDrawMethod : uint8
{
	/* Only draw an opaque border line like in starcraft */
	BorderOnly, 
	
	/* Only draw a transparent filled rectangle */
	FilledRectangleOnly,
	
	/* Draw an opaque border and a transparent filled rectangle */
	BorderAndFill
};


/* Enum to define whether something is a buff or debuff */
UENUM()
enum class EBuffOrDebuffType : uint8
{
	Buff,
	Debuff
};


//==============================================================================================
//	CPU Player AI Controller Enums
//==============================================================================================

/* CPU player AI controller only 
Different types of commands the player can issue. Macro is an ok name, but a more specific
name would be Infastructure or economy. 

These type of commands are commands that the player initially decides to do */
UENUM()
enum class EMacroCommandType : uint8
{
	// Default value
	None,

	/* Training a unit for the purpose of harvesting resources */
	TrainCollector,

	/* Building a building to serve as a resource depot */
	BuildResourceDepot,

	/* Building a building so it can serve as a construction yard */
	BuildConstructionYard,

	/* Training a worker unit so they can build buildings */
	TrainWorker,

	/* Building a building for the purpose of training army units from it */
	BuildBarracks,

	/* Research an upgrade */
	ResearchUpgrade,

	/* Base a base defense building */
	BuildBaseDefense,

	/* Training a unit to use as an army unit */
	TrainArmyUnit,

	z_ALWAYS_LAST_IN_ENUM
};


/* CPU player AI controller only */
UENUM()
enum class EMacroCommandSecondaryType : uint8
{
	// Default value
	None,

	/* Training unit either as army unit or worker or collector */
	TrainingUnit,

	/* Building a building */
	BuildingBuilding, 

	ResearchingUpgrade, 

	z_ALWAYS_LAST_IN_ENUM
};


UENUM()
enum class EAbilityDecalType : uint8
{
	/* The decal to show when the mouse is at a location where if clicked then a command will 
	be issued */
	UsableLocation, 

	/* The decal to show when the mouse is at a location where if clicked a command will not 
	be issued */
	NotUsableLocation
};


/* Rule that says how to adjust current health when max health is changed. Actually we use 
this for selectable resources (mana) too */
UENUM()
enum class EAttributeAdjustmentRule : uint8
{
	/* If max health increases by 200 then current health will not change. 
	If max health decreases by 200 then current health will not change unless it is now more 
	than max health in which case it will be set to max health */
	NoChange,

	/* If max health increases by 30% then current health increases by 30%. 
	If max health decreases by 30% then current health decreases by 30% */
	Percentage, 

	/* If max health increases by 200 then so does current health. 
	If max health decreases by 200 then current health will not change unless it is now more 
	than max health in which case it will be set to max health */
	Absolute,
};


/**
 *	How a selectable is created. 
 *	
 *	Users can add more values here if desired. Perhaps Spawned is too vague and you need to be 
 *	more specific like SpawnedUsingRaiseDead or SpawnedUsingSummon
 */
UENUM()
enum class ESelectableCreationMethod : uint8
{
	/* Currently it is unknown how it was created yet */
	Uninitialized,
	
	/* The selectable is a selectable that the player started the match with */
	StartingSelectable,
	
	/* The selectable was created using a production queue or from a worker */
	Production, 

	/* Spawned using an ability or something */
	Spawned,
};


/* What happens when a charge on an inventory item is gained/lost */
UENUM()
enum class EInventoryItemNumChargesChangedBehavior : uint8
{
	/* Do nothing e.g. diffusal blade */
	AlwaysDoNothing, 

	/* Destroy the item when it reaches zero charges e.g. TP scroll */
	DestroyAtZeroCharges,

	/* Use your own custom behavior. You will need to assing a function pointer for this usually 
	it is done in some GI function */
	CustomBehavior
};


/* Different types of ways an ability actor creates its effect */
UENUM()
enum class EAbilityUsageType : uint8
{
	/* The ability is on the action bar of a selectable */
	SelectablesActionBar, 

	/* The ability is from a selectable using an item in their inventory */
	SelectablesInventory, 

	/* One of those abilities like engineers capturing buildings or spies revealing what the building 
	is producing */
	SpecialBuildingTargetingAbility
};


/* Enum that says how a selectable has their selection decal setup */
UENUM()
enum class ESelectionDecalSetup : uint8
{
	// Not good. Means uninitialized value
	Unknown, 

	// Does not use a selection decal
	DoesNotUse, 

	/* Uses a selection decal and the material is a non-dynamic material instance */
	UsesNonDynamicMaterial, 

	/* Uses a selection decal and the material is a dynamic material instance */
	UsesDynamicMaterial
};


/* Different buttons on the UI */
UENUM()
enum class EUIElementType : uint8
{
	/* Something on a selectable's action bar */
	SelectablesActionBar,
	
	/* Something on the C&C like persistent panel */
	PersistentPanel,

	/* Slot in a selectables inventory */
	InventorySlot, 

	/* Slot in a shop */
	ShopSlot, 

	/* A slot in a production queue */
	ProductionQueueSlot,

	/* Button on the commander's global skills panel */
	GlobalSkillsPanelButton,

	/* A node on the commander's skill tree */
	CommanderSkillTreeNode, 

	/* A button type that does not show a tooltip when hovered */
	NoTooltipRequired,

	None
};


/* What a unit does after it is produced from a building */
UENUM()
enum class EUnitInitialSpawnBehavior : uint8
{
	/* Move towards the building's rally point */
	MoveToRallyPoint, 
	
	/* If the unit can collect resources then it will collect from the nearest resource spot 
	that it can. If there are no resource spots that it can collect from then it will move 
	to the building's rally point. 
	This is ambiguous. Is it the closest discovered resource spot or can we see through fog with this? */
	CollectFromNearestResourceSpot
};


/* Rules a sound obeys in relation to fog of war. 
Maybe think ofa slightly better name like ESoundHearingRules. Because some of the options 
aren't related to fog of war at all */
UENUM()
enum class ESoundFogRules : uint8
{
	/**
	 *	Ignore fog of war completely. If player positions camera over sounds location then they
	 *	will be able to hear it, whether the location is revealed or not. 
	 *	Exactly the same as if you were to take a UAudioComponent and play the sound on that.
	 *
	 *	My note: this doesn't fully work. If an actor has not replicated yet then the sound 
	 *	is never known which is not the correct behavior. Don't think many people will use this 
	 *	though.
	 */
	Ignore,

	/**
	 *	If spawn location is inside fog of war then sound will never be able to be heard. If spawn
	 *	location is outside fog of war then sound will always be able to be heard. 
	 *	Worse performance than Ignore. 
	 *	A good use for this would be if the sound is quite short (say less than 1 second).
	 */
	DecideOnSpawn,

	/* Only the team making the sound can hear it */
	InstigatingTeamOnly,

	/* Once the sound is heard once it can be heard forever. Worse performance than DecideOnSpawn */
	AlwaysKnownOnceHeard,

	/**
	 *	Sound will become unheard if it is inside fog of war, except for the team that instigated
	 *	the sound. For them it will always be heard even if the sound's location becomes inside
	 *	fog of war. Worse performance than DecideOnSpawn.
	 */
	DynamicExceptForInstigatorsTeam,

	/**
	 *	Sound will change from being heard/unheard based on its current locations fog status. This
	 *	has worse performance than all above but it's really no biggie.
	 */
	Dynamic, // OMG Dynamic, everyone's favourite word

	/* The sound can be heard through fog of war if it is close enough, even if the location of the
	sound is inside fog */
	//DynamicAndProximityChecking
};


/* What world widget (health bars, mana bars, etc) locations should be when the player's 
camera either moves, rotates or zooms */
UENUM()
enum class EWorldWidgetViewMode : uint8
{
	/* Do nothing */
	NoChange,
};


/* Whether an ability requires a target or not */
UENUM()
enum class EAbilityTargetingMethod : uint8
{
	/* Does not require any target */
	DoesNotRequireAnyTarget, 
	
	/* Requires another selectable as a target e.g. SCII ghost snipe */
	RequiresSelectable, 
	
	/* Requires a location in the world as a target e.g. artillery strike */
	RequiresWorldLocation, 
	
	/* Can target either another selectable or a world location e.g. attack move */
	RequiresWorldLocationOrSelectable, 

	/* Targets a player */
	RequiresPlayer
};


/* How units inside a garrison leave when told to all get out */
UENUM()
enum class EGarrisonUnloadAllMethod : uint8
{
	/* Everyone leaves all at once. I think SCII terran bunkers do it this way. 
	The units are tried to be positioned in a grid on the side of the building */
	AllAtOnce_Grid, 
};


/* Actions we may want to do that help with development */
UENUM()
enum class EDevelopmentAction : uint8
{
	/* Do no action */
	None, 

	/* Close the secondary popup menu widget if it is showing */
	ClosePopupMenu,

	IgnoreAllLMBRelease,
	IgnoreNextLMBRelease,

	/* Give a selectable zero health by dealing a massive amount of damage to it */
	DealMassiveDamageToSelectable, 
	CancelDealMassiveDamage, 

	/* Deal a small amount of damage to a selectable */
	DamageSelectable, 
	CancelDamageSelectable, 

	/* Award experience to a selectable */
	AwardExperience, 
	CancelAwardExperience,

	/* Award enough experience to a selectable such that they level up once or twice or something */
	AwardLotsOfExperience, 
	CancelAwardLotsOfExperience, 

	AwardExperienceToLocalPlayer, 

	/* Try give a random inventory item to a selectable */
	GiveRandomInventoryItem, 
	CancelGiveRandomInventoryItem, 

	GiveSpecificInventoryItem_SelectionPhase, 
	GiveSpecificInventoryItem_SelectTarget, 
	CancelGiveSpecificInventoryItem_SelectTarget, 

	GetUnitAIInfo, 
};


/** What to skip when starting a PIE/standlone session */
UENUM()
enum class EEditorPlaySkippingOption : uint8
{
	/* Skip nothing so opening movie will play then main menu will be shown */
	SkipNothing,

	/* Skip just the opening movie so main menu will be shown */
	SkipOpeningCutsceneOnly,

	/* Skip opening cutscene and main menu so you will play a match on the map open in editor 
	straight away */
	SkipMainMenu,

	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};


/* For a PIE/standalone seesion, what to do if the owner index of a selectable placed in the 
map is invalid */
UENUM()
enum class EInvalidOwnerIndexAction : uint8
{
	/* Selectable will not be respawned and will be excluded from PIE */
	DoNotSpawn,

	/* Selectable will be respawned belonging to the server player */
	AssignToServerPlayer,

	z_ALWAYS_LAST_IN_ENUM   UMETA(Hidden)
};
