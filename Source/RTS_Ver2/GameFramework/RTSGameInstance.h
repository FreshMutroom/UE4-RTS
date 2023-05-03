// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Components/SlateWrapperTypes.h" // For ESlateVisibility

#include "Statics/Structs_2.h"
#include "Statics/Structs_1.h"
#include "GameFramework/DamageType.h" // Now need this include for 4.20 for UDamageType FIXME if possible
//#include "Statics/Structs_4.h"
#include "Statics/Structs/Structs_7.h"
#include "RTSGameInstance.generated.h"

class AFactionInfo;
class UMainMenuWidgetBase;
class FOnlineSessionSettings;
struct FLobbySettings;
struct FMatchInfo;
class ACPUPlayerAIController;
class ADevelopmentSettings;
class UInGameWidgetBase;
class AObjectPoolingManager;
class UWorldWidget;
class UDamageType;
class USoundMix;
class USoundClass;
struct FMinimalPlayerInfo;
class UPopupWidget;
class UHeavyTaskManager;
class UBuildingTargetingAbilityBase;
class UInMatchDeveloperWidget;
struct FPIEPlayerInfo;
struct FPIECPUPlayerInfo;
enum class EInvalidOwnerIndexAction : uint8;
class UEditorPlaySettingsWidget;
class UInfantryControllerDebugWidget;


/**
 *	Rule for choosing what player start players get when they we're assigned to one before match 
 *	started
 */
UENUM()
enum class EPlayerSpawnRule : uint8
{
	// Pick a random player start on the map
	Random, 
	
	/* Try and spawn players near their teammates */
	NearTeammates
};


UENUM()
enum class EPropertyOverrideMode : uint8
{
	/* Only use the override if one is not set */
	UseDefaultIfNotSet, 
	
	/* Always use the override */
	AlwaysUseDefault
};


/**
 *	A very important class. Contains a lot of properties for your game. 
 */
UCLASS(Abstract)
class RTS_VER2_API URTSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	URTSGameInstance();

private:

	/**
	 *	Holds FactionInfo blueprints for the different factions. Needs one entry for every type
	 *	of faction
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", EditFixedSize, meta = (DisplayName = "Faction Roster"))
	TMap <EFaction, TSubclassOf <AFactionInfo>> FactionInfo_BP;

	/**
	 *	Map that stores information about context button types.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS"/*, EditFixedSize*/, meta = (DisplayName = "Context Actions"))
	TMap <EContextButton, FContextButtonInfo> ContextActionsMap;

	/* Essentially the contexts of ContextActionsMap but sorted. Using this because it's faster
	to lookup in array than hashmap given array index is known */
	UPROPERTY()
	TArray <FContextButtonInfo> ContextActions;

	//---------------------------------------------------------
	//	Commander Abilities
	//---------------------------------------------------------

	/** Different abilities the commander can use */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Commander Abilities"))
	TMap<ECommanderAbility, FCommanderAbilityInfo> CommanderAbilitiesTMap;

	/** 
	 *	Contents of CommanderAbilitiesTMap but in an array because it's faster to access an 
	 *	array instead of a hashmap if the index is known 
	 */
	UPROPERTY()
	TArray<FCommanderAbilityInfo> CommanderAbilities;

	/** Different types of nodes that can appear on a commander's skill tree */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Commander Skill Tree Nodes"))
	TMap <ECommanderSkillTreeNodeType, FCommanderAbilityTreeNodeInfo> CommanderSkillTreeNodeTMap;

	/** 
	 *	Contents of the TMap transfered to an array because array lookup is faster than 
	 *	hashmap when the index is known
	 */
	UPROPERTY()
	TArray <FCommanderAbilityTreeNodeInfo> CommanderSkillTreeNodes;

	//------------------------------ End Commander Abilities --------------------------------

	//----------------------------------------------------------------------
	//	Building Targeting Abilities
	//----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Building Targeting Abilities"))
	TMap<EBuildingTargetingAbility, FBuildingTargetingAbilityStaticInfo> BuildingTargetingAbilities;

	//------------------------- End Building Targeting Abilities --------------------------

	/* Object pooling manager */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	UPROPERTY()
	UHeavyTaskManager * HeavyTaskManager;

	/* Gets filled with information about each faction */
	UPROPERTY()
	TArray < AFactionInfo * > FactionInfo;

	/**
	 *	Holds damage multipliers for each attack type towards each armour type.
	 *
	 *	To add new damage types see DamageTypes.h. 
	 *
	 *	0 and negative values can be used to create damage types that do nothing/heal. That way 
	 *	you never need to pass a negative value into TakeDamage - just pass the amount and let the 
	 *	multiplier decide whether it should heal or not
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < TSubclassOf <UDamageType>, FDamageMultipliers > DamageMultipliers;

	/** Info about building garrison networks */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EBuildingNetworkType, FBuildingNetworkAttributes> BuildingNetworkInfo; 

	/** 
	 *	Static buffs and debuffs for your project. 
	 *	
	 *	Only some of their info can be filled in here - the rest of their behavior will need to 
	 *	be implemented in C++. See BuffAndDebuffManager.h 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EStaticBuffAndDebuffType, FStaticBuffOrDebuffInfo> StaticBuffsAndDebuffs;

	/**
	 *	Tickable buffs and debuffs for your project.
	 *
	 *	Only some of their info can be filled in here - the rest of their behavior will need to
	 *	be implemented in C++. See BuffAndDebuffManager.h
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < ETickableBuffAndDebuffType, FTickableBuffOrDebuffInfo > TickableBuffsAndDebuffs;

	/* Info about subtypes of buffs/debuffs */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < EBuffAndDebuffSubType, FBuffOrDebuffSubTypeInfo > BuffAndDebuffSubTypeInfo;

	/* Info about different selectable resources. Selectable resources are things like mana 
	or energy */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < ESelectableResourceType, FSelectableResourceColorInfo > SelectableResourceInfo;

	/* Info about each inventory item */
	UPROPERTY(EditDefaultsOnly, /*EditFixedSize Should be this but leaving out for now,*/ Category = "RTS")
	TMap < EInventoryItem, FInventoryItemInfo > InventoryItemInfo;

	/** The image to display for an inventory slot that does not have any item in it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush EmptyInventorySlotImage;

	/* The image to display for an inventory slot that does not have any item in it and the 
	mouse is hovered over it. This will be ignored if you use a unified hover image. 
	@See UnifiedMouseHoverImage_InventoryItems */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush EmptyHoveredInventorySlotImage;

	/* The image to display for an inventory slot that does not have any item in it and the
	mouse is pressed on it. This will be ignored if you use a unified pressed image */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush EmptyPressedInventorySlotImage;

	/* The sound to play when hovering over an emtpy inventory slot */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * EmptyInventorySlotHoverSound;

	/* The sound to play when pressing an empty inventpry slot with LMB */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * EmptyInventorySlotPressedByLMBSound;

	/* The sound to play when pressing an empty inventory slot with RMB */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * EmptyInventorySlotPressedByRMBSound;

	/**
	 *	How much experience bounty is multiplied by when a unit levels up.
	 *	e.g. if this = 1.2 then a level 1 unit when destroyed will award 20% more experience
	 *	to the unit that destroyed them
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float ExperienceBountyMultiplierPerLevel;

	/* Spawn and set a sound mix and find all sound classes */
	void SetupInitialSettings();

	/* Spawn actor that holds settings for development */
	void InitDevelopmentSettings();

	/* Spawn object pooling manager */
	void CreatePoolingManager();

	/* Create all the structs for each type of building/unit and store
	them in arrays. Call at least before playing a match */
	void InitFactionInfo();

	/* True if initializing faction info. Prevents spawned actors from running
	their BeginPlay which could rely on things that have not been set up yet. */
	bool bIsInitializingFactionInfo;

	/* True if Initialize has been called */
	bool bHasInitialized;

	/* True if info actors have been spawned. Reset on every map change */
	bool bHasSpawnedInfoActors;

	/* Initialize ContextInfo on begin play by checking
	if it contains duplicates with the same type (crash
	if it does), and then sorting it */
	void InitContextActions();

	void InitCommanderAbilityInfo();
	void InitCommanderSkillTreeNodeInfo();

	void InitBuildingTargetingAbilityInfo();

	/* Set the function pointers for the buff/debuff infos */
	void SetupBuffsAndDebuffInfos();

	/* Do setupy stuff for inventory items */
	void InitInventoryItemInfo();

	void InitKeyInfo();

	void CreateHeavyTaskManager();

	/**	
	 *	If you choose to use a cursor here then it will override anything you have set in project
	 *	settings under hardware cursors. This cursor will show up when using the main menu.
	 *
	 *	If tesing in PIE and using bSkipMainMenu this still needs to be set. Make it the same as
	 *	your match cursor 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType MenuMouseCursor;

	/**
	 *	Mouse cursor to use during matches. This can be overriddem on a per-faction basis.
	 *	If choosing to use custom mouse cursors for abilities then this or faction infos will
	 *	need to set a cursor here for the game to roll back to it - I know no way of setting the
	 *	hardware cursor back to the default white arrow cursor.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType MatchMouseCursor;

	/** 
	 *	8 edge scrolling cursors. The cursors that appear when the mouse is close enough to 
	 *	the edge of the screen that the camera moves 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_Top;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_TopRight;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_Right;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_BottomRight;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_Bottom;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_BottomLeft;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_Left;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType EdgeScrollingCursor_TopLeft;

	/* Default mouse cursor to use if ability does not have one set */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultAbilityCursor_Default;
	/* Default mouse cursor to use if ability does not have one set */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultAbilityCursor_AcceptableTarget;
	/* Default mouse cursor to use if ability does not have one set */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultAbilityCursor_UnacceptableTarget;

	FCursorInfo MenuMouseCursor_Info;
	FCursorInfo DefaultAbilityCursor_Default_Info;
	FCursorInfo DefaultAbilityCursor_AcceptableTarget_Info;
	FCursorInfo DefaultAbilityCursor_UnacceptableTarget_Info;

	/* Setup custom mouse cursors */
	void InitMouseCursorInfo();

	/**
	 *	Selection decal to appear under a selectable when a match observer selects something
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Match Observers")
	FSelectionDecalInfo ObserverSelectionDecal;

	/**
	 *	Widget to appear on a selectable when it is selected and the player is a match observer. 
	 *	Not used for resource spots
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Match Observers")
	TSubclassOf <UWorldWidget> ObserverSelectableSelectionWorldWidget;

	/**
	 *	Widget to always appear on selectables when the player is a match observer. Widget can
	 *	show things like health, construction status etc. Not used for resource spots
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Match Observers")
	TSubclassOf <UWorldWidget> ObserverSelectablePersistentWorldWidget;

	/** 
	 *	Widget to appear on a resource spot when it is selected and the player is a match 
	 *	observer
	 *
	 *	Made these seperate because the ObserverSelectableSelectionWorldWidget will likely have 
	 *	a health bar and resource spots won't need that
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Match Observers")
	TSubclassOf <UWorldWidget> ObserverResourceSpotSelectionWorldWidget;

	/** Widget to always appear on a resource spot and the player is a match observer */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Match Observers")
	TSubclassOf <UWorldWidget> ObserverResourceSpotPersistentWorldWidget;

	/** 
	 *	How world widget (like health bars) are positioned whenever the player's camera moves, 
	 *	rotates or zooms 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EWorldWidgetViewMode WorldWidgetViewMode;

	/**
	 *	Maps each defeat condition to info about it. To actually change what function is called
	 *	to check if condition is met will require adding code to game mode
	 */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < EDefeatCondition, FDefeatConditionInfo > DefeatConditions;

	void SetupDefeatConditionInfo();

	/* Maps integer to FName for less bandwidth across wire when changing map */
	UPROPERTY()
	TMap <FMapID, FName> MapPoolIDs;

	/**
	 *	List of maps that matches can be played on. Maps map name (the name it was given in 
	 *	editor minus any extensions e.g. Minimal_Default, Entry, etc) to info about it
	 *	
	 *	Limit 256 maps
	 *
	 *	If changing the name of this variable then update ALevelVolume::StoreMapInfo 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <FName, FMapInfo> MapPool;

	/** 
	 *	Mouse cursors. Map enum type to the path/hotspot info. 
	 *	
	 *	Note: if your mouse cursor appears to be off of its location make sure to adjust the host spot 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EMouseCursorType, FCursorInfo> MouseCursors;

	void SetupMapPool();

	/**
	 *	The rule for choosing player starts for players that weren't assigned one before 
	 *	match started
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EPlayerSpawnRule PlayerSpawnRule;

	/* A sound mix which will modify the volume of all sounds */
	UPROPERTY()
	USoundMix * SoundMix;

	/* Map of all sound classes. Maps their GetName() to the UObject. The TMap is so a user can
	enter the type of sound class they want a audio settings widget to modify */
	UPROPERTY()
	TMap <FString, USoundClass *> SoundClasses;

	/* Match loading screen messages */
	UPROPERTY()
	TMap < ELoadingStatus, FText> LoadingScreenMessages;

	void SetupLoadingScreenMessages();

	/** Maps CPU player difficulty info about them */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS|CPU Players")
	TMap <ECPUDifficulty, FCPUDifficultyInfo> CPUPlayerInfo;

	/**
	 *	Widgets to use during a match. Faction info that has not specified a specific widget will
	 *	default to using the one from here
	 */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (DisplayName = "In-Game Widgets"))
	TMap <EMatchWidgetType, TSubclassOf < UInGameWidgetBase >> MatchWidgets_BP;

	/* Information about resources */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < EResourceType, FResourceInfo > ResourceInfo;

	/* Defines how much resources players start with at start of match */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < EStartingResourceAmount, FStartingResourceConfig > StartingResourceConfigs;

	/* Game notification messages to appear in HUD + sound to play */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS|HUD")
	TMap < EGameNotification, FGameNotificationInfo > NotificationMessages;

	/**
	 *	Warning messages to appear when things happen in game. Can be overridden on a per-faction
	 *	basis
	 */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS|HUD")
	TMap < EGameWarning, FGameWarningInfo > HUDMessages;

	/* Warning messages for custom reasons abilities can fail */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS|HUD")
	TMap < EAbilityRequirement, FGameWarningInfo > AbilityHUDMessages;

	/* Warning messages to display when you try to spend more of a resource than you have */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	TMap < EResourceType, FGameWarningInfo > MissingResourceHUDMessages;

	/* Warning messages to display when you try to spend more of a resource than you have */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	TMap < EHousingResourceType, FGameWarningInfo > MissingHousingResourceHUDMessages;

	/**
	 *	Minimum time between game warning messages being allowed to show on HUD. If player say
	 *	spams a build button without the prereqs met then messages will appear on HUD only if this
	 *  amount of time has passed
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	float HUDGameMessageCooldown;

	/* How to draw the marquee selection box */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Marquee HUD")
	EMarqueeBoxDrawMethod MarqueeSelectionBoxStyle;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditMarqueeBorderValues;

	UPROPERTY()
	bool bCanEditMarqueeFillValues;
#endif

	/* Color of the marquee box filled rectangle if using it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Marquee HUD", meta = (EditCondition = bCanEditMarqueeFillValues))
	FLinearColor MarqueeBoxRectangleFillColor;

	/* Color of the marquee box border. Always drawn opaque - alpha value will be ignored */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Marquee HUD", meta = (EditCondition = bCanEditMarqueeBorderValues))
	FLinearColor MarqueeBoxBorderColor;

	/* How thick to draw the line for the marquee box border */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Marquee HUD", meta = (EditCondition = bCanEditMarqueeBorderValues))
	float MarqueeBoxBorderLineThickness;

	/** 
	 *	The percentage of max health building starts with at the start of its construction (in
	 *	the range 0 to 1). Clamped to 0.01 (1%) because possible replicated float compresssion may
	 *	cause it to be rounded to 0 (if that is even a thing) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float BuildingStartHealthPercentage;

	/**
	 *	Rule for being able to see and change unit rally point for a unit-producing building.
	 *	The rule refers to the state of the building 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingRallyPointDisplayRule BuildingRallyPointDisplayRule;

	/**
	 *	The name of the vector param on each of the ghost building mesh's materials to modify when
	 *	the ghost is in a suitable/non-suitable build location e.g. in most RTSes the building will
	 *	change to a red color when not in a suitable build location 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName GhostBadLocationParamName;

	/* The value to change GhostBadLocationParam to when not in a suitable build location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FLinearColor GhostBadLocationParamValue;

	/* Fog of war material */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Fog of War")
	UMaterialInterface * FogOfWarMaterial;

	//-----------------------------------------------------------------------------------------
	//	Unified HUD button hovered/pressed images/sounds
	//-----------------------------------------------------------------------------------------
	//	Brief description of what these are:
	//	When you hover/press your mouse on a button sometimes their appearance changes. For the 
	//	structs below if you set the bool bUseUnifiedImage to true then the same OnHovered 
	//	or OnPressed effect will be used for each button on that particular part of the HUD.
	//	An image will be drawn over top of the current 
	//	image. So that means you do not have to hard code and manually set all the 
	//	hovered/pressed images for each image. The hovered/pressed image will automatically be 
	//	adjusted to the right size of the button you're hovering/pressing.
	//
	//	If you want the hovered/pressed effects to be different for different buttons or if 
	//	you have all the hovered/pressed image files already then you can ignore these and 
	//	should set the hovered/pressed image files individually.
	//
	//	Oh yeah same deal with sounds too since often OnHovered/OnPressed sounds are the same 
	//	sound no matter the button.
	//
	//	My notes: it's not hard to add more categories: e.g. I have action bar / persistent panel / 
	//	production queue all using the same brush, but you can split them up if you choose. 
	//	Update UsingAtLeastOneUnifiedMouseFocusImage() if you do add more though. 
	//	PostEditChangeChainProperty will need updating too
	//
	//	I just realized: you can change the tint of brushes so you can create a hover style 
	//	effect. But this method still allows you to draw borders and such around the brushes
	//	so it has some use.
	//-----------------------------------------------------------------------------------------

	// Selectable's action bar / Persistent HUD panel / Production queue
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedImageAndSoundAssets UnifiedImagesAndSounds_ActionBarETC;

	// Selectable's inventory / Item shop
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedImageAndSoundAssets UnifiedImagesAndSounds_InventoryItems;

	// Menus. I have only used these for the pause menu but may want to use them for main menu, lobby, etc
	UPROPERTY(EditDefaultsOnly, Category = "RTS|HUD")
	FUnifiedImageAndSoundAssets UnifiedImagesAndSounds_Menus;

	/* These hold data already contained in the structs above. It is here for performance */
	UPROPERTY()
	FUnifiedImageAndSoundFlags UnifiedButtonAssetFlags_ActionBarETC;
	UPROPERTY()
	FUnifiedImageAndSoundFlags UnifiedButtonAssetFlags_InventoryItems;
	UPROPERTY()
	FUnifiedImageAndSoundFlags UnifiedButtonAssetFlags_Shop;
	UPROPERTY()
	FUnifiedImageAndSoundFlags UnifiedButtonAssetFlags_Menus;

	//==========================================================================================
	//	Player controller control settings
	//==========================================================================================
	//	These really belong on the player controller but my current philosiphy is that users 
	//	don't make blueprints of the game.mode/player.controller so these variables are here 
	//	instead
	//------------------------------------------------------------------------------------------

	/** 
	 *	A curve that says how fast the camera moves at different zoom levels 
	 *	
	 *	X axis = Zoom amount e.g. if your range of zoom amounts is 500 to 5000 then that is what 
	 *	should be along this axis 
	 *	Y axis = speed multiplier 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Controls")
	UCurveFloat * CamaraZoomToMoveSpeedCurve;

	/** 
	 *	SHOULD BE LINEAR, NOT CURVED. Actually zooming doesn't really work very well anyway 
	 *	so doesn't matter.
	 *
	 *	Curve used for gradual zoom in/out when scrolling mouse wheel. Can be left null and
	 *	zoom amount will be changed instantly.
	 *	
	 *	X axis = base time to reset camera. Will be multiplied by CameraZoomSpeed
	 *	Y axis = Lerp amount, range should be from 0 to 1 
	 *
	 *	At some point a pointer on the PC is set to this. But are there performance gains from 
	 *	having it as a EditDefaultsOnly variable on the PC to begin with? Unless there is some 
	 *	serious black magic with UE4 then I would think not. If there is though could just 
	 *	remove that variable on PC and go GI->GetCameraMouseWheelZoomCurve() whenever we need it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Controls")
	UCurveFloat * CameraMouseWheelZoomCurve;

	/**
	 *	Curve to use to transition from current camera rotation/zoom to defaults. Can be left
	 *	null and transition will happen instantly.
	 *
	 *	X axis = base time to complete zoom. Will be multiplied by ResetCameraToDefaultRate. Good to
	 *	make it at least from 0 to 1 and adjust ResetCameraToDefaultRate to get desired time taken
	 *	Y axis = Lerp amount, range should be from 0 to 1
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Controls")
	UCurveFloat * CameraResetCurve;

	//	End player controller control settings
	//==========================================================================================

	//==========================================================================================
	//	Key mappings
	//==========================================================================================

	/* Maps a key on the keyboard/mouse to information about it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	TMap<FKey, FKeyInfo> KeyInfo;

	/* The "plus: symbol to put in between modifier keys e.g. CTRL + ALT + P */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	FSlateBrush KeyMappings_PlusSymbol;

	/* The symbol for the CTRL key when it is being used as a modifier */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings", meta = (DisplayName = "CTRL Key Image"))
	FSlateBrush KeyMappings_CTRLModifier;

	/* The symbol for the ALT key when it is being used as a modifier */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings", meta = (DisplayName = "ALT Key Image"))
	FSlateBrush KeyMappings_ALTModifier;

	/* The symbol for the SHIFT key when it is being used as a modifier */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings", meta = (DisplayName = "SHIFT Key Image"))
	FSlateBrush KeyMappings_SHIFTModifier;

	/** 
	 *	If true then you want the image representation of a keyboard/mouse key to have 
	 *	some text drawn overtop of some image and you want this to happen for every key. 
	 *	My notes: could perhaps allow certain FKeyInfo to opt out of having this applied to them 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	bool KeyMappings_bForceUsePartialBrushes;

	/* How to use the image override */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	EPropertyOverrideMode KeyMappings_BrushOverrideMode;

	/* How to use the font override */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	EPropertyOverrideMode KeyMappings_FontOverrideMode;

	/* Partial brush override */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	FSlateBrush KeyMappings_DefaultBrush;

	/* Partial font override */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Key Mappings")
	FSlateFontInfo KeyMappings_DefaultFont;

	//	End key mappings variables
	//==========================================================================================

	/**
	 *	List of selectable classes that when the match starts should be left on the map.
	 *	Good example is resource spots. 
	 *	There is an assumption in game mode that any classes added to this list are neutral 
	 *	selectables
	 */
	// UPROPERTY() // Can maybe get away without this
	TSet < UClass * > LeaveOnMapList;

	/* Most amount of items a shop has on display and/or sells */
	int32 LargestShopCatalogueSize;
	
	/* The largest inventory a selectable has */
	int32 LargestInventoryCapacity;

	//========================================================================================
	//	Constructor functions
	//========================================================================================
	
	/* Sets up some common context commands such as 'attack move' and 'hold position'. Saves
	having to redo them in editor because of data loss bug */
	void SetupCommonContextActions();

	/* Setup context actions like artillery strike. Saves having to redo in editor because of
	data loss bug */
	void SetupCustomContextActions();

	/* Fill context action TMap with default struct for any abilities that have not been 
	defined in SetupCommonContextActions or SetupCustomContextActions */
	void SetupMissingContextActions();

	void SetupCPUInfo();

	void SetupMatchWidgets_BP();

	void SetDefaultHUDMessages();

	void SetupResourceInfo();

	void SetupLeaveOnMapList();

	void SetupSelectableResourceInfo();

	void SetupInventoryItemInfo();

	void SetupKeyInfo();

protected:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

public:

	/* Init() = overridable game instance version of begin play */
	void Initialize();

	/* Spawns faction infos. Initialize() may work too */
	void OnMapChange();

	// Virtual function to allow custom GameInstances an opportunity to do cleanup when shutting down
	virtual void Shutdown() override;

	// Getters and setters

	/* Get array of all faction info */
	const TArray < AFactionInfo * > & GetAllFactionInfo() const;

	/* Get reference to FactionInfo for certain faction
	@param Faction - the faction to get info for
	@return - the faction info */
	AFactionInfo * GetFactionInfo(EFaction Faction) const;

	/* Get faction info for a random faction */
	AFactionInfo * GetRandomFactionInfo() const;

	/* Get a random faction */
	EFaction GetRandomFaction() const;

	/* Get info about a context action given the button type.
	The function that is called by each button type needs to
	be added to a function in MyPlayerController
	@param ButtonType - the button type to get info for
	@return - a struct that holds the info about the button */
	const FContextButtonInfo & GetContextInfo(EContextButton ButtonType) const;

	const FCommanderAbilityInfo & GetCommanderAbilityInfo(ECommanderAbility AbilityType) const;
	const FCommanderAbilityTreeNodeInfo & GetCommanderSkillTreeNodeInfo(ECommanderSkillTreeNodeType NodeType) const;

	const TArray <FCommanderAbilityInfo> & GetAllCommanderAbilities() const;

	const FBuildingTargetingAbilityStaticInfo & GetBuildingTargetingAbilityInfo(EBuildingTargetingAbility AbilityType) const;

	/* Get information about a resource */
	const FResourceInfo & GetResourceInfo(EResourceType ResourceType) const;

	/* Get the specific amounts of resources to start with given an amount enum */
	const FStartingResourceConfig & GetStartingResourceConfig(EStartingResourceAmount Amount) const;

	const TMap < EStartingResourceAmount, FStartingResourceConfig > & GetAllStartingResourceConfigs() const;

	/* Get object pooling manager in charge of projectile pooling */
	AObjectPoolingManager * GetPoolingManager() const;

	UHeavyTaskManager * GetHeavyTaskManager() const;

	/* Get how much base damage (before upgrades) a certain damage type deals
	to a certain armour type
	@param DamageType - damage instigator's damage type
	@param ArmourType - damage recipient's armour type
	@return - base damage multiplier */
	float GetDamageMultiplier(TSubclassOf <UDamageType> DamageType, EArmourType ArmourType) const;

	const FBuildingNetworkAttributes & GetBuildingNetworkInfo(EBuildingNetworkType NetworkType) const;

	float GetExperienceBountyMultiplierPerLevel() const;

	bool IsInitializingFactionInfo() const;

	const TArray < FContextButtonInfo > & GetAllContextInfo() const;

	const FDefeatConditionInfo & GetDefeatConditionInfo(EDefeatCondition Condition) const;

	const TMap < EDefeatCondition, FDefeatConditionInfo > & GetAllDefeatConditions() const;

	/* Get the list of maps matches can be played on. */
	const TMap <FName, FMapInfo> & GetMapPool() const;

	/* Get map info given its name or ID. I think MapName refers to the name given by user
	in editor not folder path */
	const FMapInfo & GetMapInfo(const FName & MapName) const;
	const FMapInfo & GetMapInfo(FMapID MapID) const;

	/* Return whether a world location is inside the bounds of the current map */
	bool IsLocationInsideMapBounds(const FVector & Location) const;

	const FCursorInfo & GetMouseCursorInfo(EMouseCursorType MouseCursorType) const;

	EMouseCursorType GetMatchMouseCursor() const;
	EMouseCursorType GetEdgeScrollingCursor_Top() const;
	EMouseCursorType GetEdgeScrollingCursor_TopRight() const;
	EMouseCursorType GetEdgeScrollingCursor_Right() const;
	EMouseCursorType GetEdgeScrollingCursor_BottomRight() const;
	EMouseCursorType GetEdgeScrollingCursor_Bottom() const;
	EMouseCursorType GetEdgeScrollingCursor_BottomLeft() const;
	EMouseCursorType GetEdgeScrollingCursor_Left() const;
	EMouseCursorType GetEdgeScrollingCursor_TopLeft() const;

	EMouseCursorType GetAbilityDefaultMouseCursor_Default() const;
	EMouseCursorType GetAbilityDefaultMouseCursor_AcceptableTarget() const;
	EMouseCursorType GetAbilityDefaultMouseCursor_UnacceptableTarget() const;

	/* Get sound mix that is passed around by UGameplayStatics::SetSoundMixClassOverride */
	USoundMix * GetSoundMix() const;

	/* Get all sound classes */
	const TMap <FString, USoundClass *> & GetSoundClasses() const;

	/* Given a string get the sound class */
	USoundClass * GetSoundClass(const FString & SoundClassName) const;

	/* Get info about a static buff/debuff */
	const FStaticBuffOrDebuffInfo * GetBuffOrDebuffInfo(EStaticBuffAndDebuffType Type) const;

	/* Get info about a tickable buff/debuff */
	const FTickableBuffOrDebuffInfo * GetBuffOrDebuffInfo(ETickableBuffAndDebuffType Type) const;

	const FBuffOrDebuffSubTypeInfo & GetBuffOrDebuffSubTypeInfo(EBuffAndDebuffSubType SubType) const;

	const FSelectableResourceColorInfo & GetSelectableResourceInfo(ESelectableResourceType ResourceType) const;

	const FInventoryItemInfo * GetInventoryItemInfo(EInventoryItem ItemType) const;

	/* Returns "None" if there are none */
	EInventoryItem GetRandomInventoryItem() const;

	const FSlateBrush & GetEmptyInventorySlotImage_Normal() const;
	const FSlateBrush & GetEmptyInventorySlotImage_Hovered() const;
	const FSlateBrush & GetEmptyInventorySlotImage_Pressed() const;
	USoundBase * GetEmptyInventorySlotSound_Hovered() const;
	USoundBase * GetEmptyInventorySlotSound_PressedByLMB() const;
	USoundBase * GetEmptyInventorySlotSound_PressedByRMB() const;

#if !UE_BUILD_SHIPPING
	/* Functions to query whether a type is a buff or a debuff */
	bool IsBuff(EStaticBuffAndDebuffType Type) const;
	bool IsBuff(ETickableBuffAndDebuffType Type) const;
	bool IsDebuff(EStaticBuffAndDebuffType Type) const;
	bool IsDebuff(ETickableBuffAndDebuffType Type) const;
#endif

	/* Get info on the custom main menu mouse cursor */
	const FCursorInfo & GetMenuCursorInfo() const;

	/* Get selection decal info for observers */
	const FSelectionDecalInfo & GetObserverSelectionDecal() const;

	/* Get widgets to appear on selectables for observers */
	const TSubclassOf<UWorldWidget> & GetObserverSelectableSelectionWorldWidget() const;
	const TSubclassOf<UWorldWidget> & GetObserverSelectablePersistentWorldWidget() const;
	const TSubclassOf<UWorldWidget> & GetObserverResourceSpotSelectionWorldWidget() const;
	const TSubclassOf<UWorldWidget> & GetObserverResourceSpotPersistentWorldWidget() const;

	EWorldWidgetViewMode GetWorldWidgetViewMode() const;

	/* Get CPU difficulty as string */
	const FString & GetCPUDifficulty(ECPUDifficulty AsEnum) const;

	const TMap < ECPUDifficulty, FCPUDifficultyInfo > & GetCPUDifficultyInfo() const;

	/* Return whether an optional match widget has a blueprint set for it */
	bool IsMatchWidgetBlueprintSet(EMatchWidgetType WidgetType) const;

	const TSubclassOf <UInGameWidgetBase> & GetMatchWidgetBP(EMatchWidgetType WidgetType) const;

	const FGameNotificationInfo & GetHUDNotificationMessageInfo(EGameNotification NotificationType) const;

	// For generic common things
	const FGameWarningInfo & GetGameWarningInfo(EGameWarning MessageType) const;
	// For some reason an ability cannot be used
	const FGameWarningInfo & GetGameWarningInfo(EAbilityRequirement ReasonForMessage) const;
	// For missing resources
	const FGameWarningInfo & GetGameWarningInfo(EResourceType ResourceType) const;
	// For missing housing resources
	const FGameWarningInfo & GetGameWarningInfo(EHousingResourceType HousingResourceType) const;

	float GetHUDGameMessageCooldown() const;

	// Marquee settings 
	EMarqueeBoxDrawMethod GetMarqueeSelectionBoxStyle() const;
	const FLinearColor & GetMarqueeBoxRectangleFillColor() const;
	const FLinearColor & GetMarqueeBoxBorderColor() const;
	float GetMarqueeBoxBorderLineThickness() const;

	float GetBuildingStartHealthPercentage() const;

	EBuildingRallyPointDisplayRule GetBuildingRallyPointDisplayRule() const;

	const FName & GetGhostBadLocationParamName() const;

	const FLinearColor & GetGhostBadLocationParamValue() const;

	UMaterialInterface * GetFogOfWarMaterial() const;

	/* Return whether the user wants to use the same mouse hover/press image for at least one 
	type of HUD element */
	bool UsingAtLeastOneUnifiedMouseFocusImage() const;

	const FUnifiedImageAndSoundAssets & GetUnifiedButtonAssets_ActionBar() const;
	const FUnifiedImageAndSoundAssets & GetUnifiedButtonAssets_Inventory() const;
	const FUnifiedImageAndSoundAssets & GetUnifiedButtonAssets_Shop() const;
	const FUnifiedImageAndSoundAssets & GetUnifiedButtonAssets_Menus() const;

	const FUnifiedMouseFocusImage & GetUnifiedHoverBrush_ActionBar() const;
	const FUnifiedMouseFocusImage & GetUnifiedPressedBrush_ActionBar() const;
	const FUnifiedUIButtonSound & GetUnifiedHoverSound_ActionBar() const;
	const FUnifiedUIButtonSound & GetUnifiedLMBPressedSound_ActionBar() const;
	const FUnifiedUIButtonSound & GetUnifiedRMBPressedSound_ActionBar() const;

	const FUnifiedMouseFocusImage & GetUnifiedHoverBrush_Inventory() const;
	const FUnifiedMouseFocusImage & GetUnifiedPressedBrush_Inventory() const;
	const FUnifiedUIButtonSound & GetUnifiedHoverSound_Inventory() const;
	const FUnifiedUIButtonSound & GetUnifiedLMBPressedSound_Inventory() const;
	const FUnifiedUIButtonSound & GetUnifiedRMBPressedSound_Inventory() const;

	// These just return the same as inventory
	const FUnifiedMouseFocusImage & GetUnifiedHoverBrush_Shop() const;
	const FUnifiedMouseFocusImage & GetUnifiedPressedBrush_Shop() const;
	const FUnifiedUIButtonSound & GetUnifiedHoverSound_Shop() const;
	const FUnifiedUIButtonSound & GetUnifiedLMBPressedSound_Shop() const;
	const FUnifiedUIButtonSound & GetUnifiedRMBPressedSound_Shop() const;

	const FUnifiedMouseFocusImage & GetUnifiedHoverBrush_Menus() const;
	const FUnifiedMouseFocusImage & GetUnifiedPressedBrush_Menus() const;
	const FUnifiedUIButtonSound & GetUnifiedHoverSound_Menus() const;
	const FUnifiedUIButtonSound & GetUnifiedLMBPressedSound_Menus() const;
	const FUnifiedUIButtonSound & GetUnifiedRMBPressedSound_Menus() const;

	FUnifiedImageAndSoundFlags GetUnifiedButtonAssetFlags_ActionBar() const;

	FUnifiedImageAndSoundFlags GetUnifiedButtonAssetFlags_InventoryItems() const;

	FUnifiedImageAndSoundFlags GetUnifiedButtonAssetFlags_Shop() const;

	FUnifiedImageAndSoundFlags GetUnifiedButtonAssetFlags_Menus() const;

	UCurveFloat * GetCamaraZoomToMoveSpeedCurve() const;

	UCurveFloat * GetCameraMouseWheelZoomCurve() const;

	UCurveFloat * GetCameraResetCurve() const;

	const FKeyInfo & GetInputKeyInfo(const FKey & Key) const;

	const FSlateBrush & KeyMappings_GetPlusSymbolImage() const;
	const FSlateBrush & KeyMappings_GetCTRLModifierImage() const;
	const FSlateBrush & KeyMappings_GetALTModifierImage() const;
	const FSlateBrush & KeyMappings_GetSHIFTModifierImage() const;

	/* Given a selectable return whether it should stay on the map or be destroyed at the
	start of the match. Resource spots are examples of falses. This is a slowish operation */
	bool ShouldStayOnMap(const AActor * Selectable) const;

	int32 GetLargestShopCatalogueSize() const;
	int32 GetLargestInventoryCapacity() const;


	//==========================================================================================
	//	Audio
	//==========================================================================================

protected:

	/* Setup audio component during Initialize */
	void SetupAudioComponents();

	/* The audio component in charge of playing music, both in the main menu and in match */
	UPROPERTY()
	UAudioComponent * MusicAudioComp;

	/* The audio component in charge of playing the sound when player selects a selectable
	e.g. "Ready to roll out!" */
	UPROPERTY()
	UAudioComponent * PlayerSelectionAudioComp;

	/* Audio component in charge of playing sounds when the player issues a command */
	UPROPERTY()
	UAudioComponent * PlayerCommandAudioComp;

	/* Audio component to play zero health sounds */
	UPROPERTY()
	UAudioComponent * ZeroHealthAudioComp;

	/* Audio component to play various effects that don't fall into a category above for
	example: the sound when player sets a buildings rally point */
	UPROPERTY()
	UAudioComponent * EffectsAudioComp;

	/* The music to play while in main menu/lobby */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * MainMenuMusic;

	/* The sounds to play when a chat message is received */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	TMap <EMessageRecipients, USoundBase *> ChatMessageReceivedSounds;

public:

	/* Play some music from the music audio comp */
	void PlayMusic(USoundBase * InMusic);

	/* Returns whether a sound for player selection is playing or not */
	bool IsPlayingPlayerSelectionSound() const;

	/* Play a 'player selection' sound - the sound when player selects a selectable */
	void PlayPlayerSelectionSound(USoundBase * InSound);

	/* Returns whether a sound for a player command is playing or not */
	bool IsPlayingPlayerCommandSound() const;

	/* Play a sound for when the player issues a command */
	void PlayPlayerCommandSound(USoundBase * SoundToPlay);

	bool IsPlayingZeroHealthSound() const;

	void PlayZeroHealthSound(USoundBase * SoundToPlay);

	/* Play a 2D sound with EffectsAudioComp */
	void PlayEffectSound(USoundBase * SoundToPlay);

	/* Get the sound to play for a certain chat message type, or null if none */
	USoundBase * GetChatMessageReceivedSound(EMessageRecipients MessageRecipients) const;


	//==========================================================================================
	//	Menu Widget Functionality
	//==========================================================================================

	/* These function bodies can be found in RTS/MainMenu/Menus.cpp. Also some of this stuff
	can be made protected */

public:

	/* Main menu widgets blueprints */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (DisplayName = "Main Menu Widgets"))
	TMap < EWidgetType, TSubclassOf <UMainMenuWidgetBase> > Widgets_BP;

	/* Main menu widgets that have already been created */
	UPROPERTY()
	TMap < EWidgetType, UMainMenuWidgetBase *> Widgets;

	/* Show a widget and optionally change visibility of the current widget.
	Use Collapsed to hide, Visible to show and make interactive, HitTestInvisible to show
	but make inactive.
	Z order for widgets is defined as static_cast<uint8>(WidgetType) and can be reordered
	by reordering EWidgetType enum.
	@param WidgetType - type of widget to show
	@param NewCurrentWidgetsVisibility - visibility to change current widget to, either
	HistTestInvisible or collapsed */
	void ShowWidget(EWidgetType WidgetType, ESlateVisibility NewCurrentWidgetsVisibility = ESlateVisibility::HitTestInvisible);

	/* Show previous widget and make it interactable again if it is not. Behavior for returning
	to a previous widget is to play current widgets exit anim and when it finishes to then make
	all widgets in between the target widget to become hidden and then play target widgets enter
	anim. Currently widget does not become interactable until anim fully completes (can be
	overridden by adding it as a keyframe in editor but strongly advise not doing this)
	@param NumToGoBack - how many widgets to go back. Use 0 to return to the first widget in
	WidgetHistory. If NumToGoBack is larger then the number of previous widgets shown then an
	assert will be thrown */
	void ShowPreviousWidget(int32 NumToGoBack = 1);

	/* Remove a widget from viewport. For internal use only. Possibly not safe to call with
	NumWidgetsBack = 0
	@return - true if a widget was removed, false otherwise */
	bool Internal_HideWidget(EWidgetType WidgetType);

	/* Remove all menu widgets from viewport and delete WidgetHistory */
	void HideAllMenuWidgets();

	/* Remove a widget type from WidgetHistory so it can never be navigated to with
	ShowPreviousWidget */
	void RemoveFromWidgetHistory(EWidgetType WidgetType);

	/* When loading a match destroy all the menu widgets since they aren't needed except the
	match loading screen */
	void DestroyMenuWidgetsBeforeMatch();

	void OnWidgetExitAnimFinished(UMainMenuWidgetBase * Widget);
	void OnWidgetEnterAnimFinished(UMainMenuWidgetBase * Widget);

	/* Returns whether an optional widget has had its blueprint set in editor */
	bool IsWidgetBlueprintSet(EWidgetType WidgetType) const;

	/* Returns a reference to the widget for the specified type. Will construct a widget
	if the widget hasn't already been created. For this reason should never return null.
	Should not be called in mutlithreaded situations to avoid possibility of creating two
	of the same widget type */
	UMainMenuWidgetBase * GetWidget(EWidgetType WidgetType);

	/* A templated version of GetWidget which I should use in place of the non-templated 
	version. */
	template <typename T>
	T * GetWidget(EWidgetType WidgetType)
	{
		return CastChecked<T>(GetWidget(WidgetType));
	}

private:

	/* Array to keep track of previous menus visited to implement back behaviour. Index 0 holds
	the first widget viewed, index 1 the next etc. If returning to a previous menu then show
	widget at WidgetHistory.Num() - 2 provided it's a valid index.
	ShowWidget() and ShowPreviousWidget() are the only two functions that change this array */
	UPROPERTY()
	TArray <EWidgetType> WidgetHistory;

	/* Get the widget most recently added to screen */
	UMainMenuWidgetBase * GetCurrentWidget();

	/* Get the ZOrder for a type of widget */
	uint8 GetZOrder(EWidgetType WidgetType);

	/*  Play the exit animation for a widget when it is leaving the screen */
	void PlayWidgetExitAnim(EWidgetType WidgetType);

	typedef void (URTSGameInstance:: *FunctionPtrType)(void);
	/* Optional function to call after widget exit anim finishes. Mainly here to aid in playing
	widget animations and changing maps */
	FunctionPtrType WidgetTransitionFunction;

	void PlayWidgetEnterAnim(EWidgetType WidgetType);

	/* Widget to show after camera fade */
	EWidgetType WidgetToShow;

	/* When calling ShowPreviousWidget how many widgets back to go. Also used as a signal
	to know whether we are showing a new widget or returning to a previous one. 0 means
	showing new widget. > 0 means returning to a previous one */
	int32 NumWidgetsBack;

	/* Call function that returns void after delay
	@param TimerHandle - timer handle to use
	@param Function - function to call
	@param Delay - delay before calling function */
	void Delay(FTimerHandle & TimerHandle, void(URTSGameInstance:: *Function)(), float Delay);

	//---------------------------------------------
	//	Popup Widget
	//---------------------------------------------

	/** Widget to show up when a warning or something wants to be shown to player while in menus */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Popup Widget"))
	TSubclassOf < UPopupWidget > PopupWidget_BP;

	/* Popup widget once it has been created */
	UPROPERTY()
	UPopupWidget * PopupWidget;

public:

	/** 
	 *	Show the popup widget. Widget does not go in widget history and will be removed from 
	 *	screen when changing widget
	 *	@param Message - message to set on popup widget 
	 *	@param Duration - how long to show widget for. @See UPopupWidget::Show for a more 
	 *	thourogh breakdown
	 *	@param bSpecifyTextColor - if true then 3rd param will be color text will be. If false 
	 *	widget will use the color that was set in editor 
	 *	@param TextColor - color of text
	 */
	void ShowPopupWidget(const FText & Message, float Duration = 3.f, bool bSpecifyTextColor = false, 
		const FLinearColor & TextColor = FLinearColor::Black);

protected:

	void HidePopupWidget();


	//==========================================================================================
	//	Main Menu
	//==========================================================================================

public:

	/* Called when entering main menu for the very first time from launching game */
	void OnEnterMainMenuFromStartup();

	void OnEnterMainMenu();

	/* Call when user has confirmed they want to exit to OS */
	void OnQuitGameInitiated();

	/* Static version. Exit to OS as fast as possible */
	static void QuitGame(UWorld * World);

protected:

	/* Exit to OS as fast as possible */
	UFUNCTION()
	void QuitGame();


	//==========================================================================================
	//	Lobby Creation Functionality
	//==========================================================================================

public:

	/* Travel to lobby map and show lobby widget
	@param LobbyType - network model of lobby e.g. LAN, online, offline */
	void CreateSingleplayerLobby();

protected:

	/* Open the blank persistent level and stream in the lobby map */
	void LoadLobbyMapForSingleplayer();

	UFUNCTION()
	void SetupLobbyWidget_Singleplayer();

public:

	/* Try create a networked session
	@param LobbyName -
	@param bIsLAN - true if network model is LAN
	@param Password - password for lobby. Blank implies no password
	@param NumSlots -
	@param MapID - map ID of the map to create lobby with. Map can possibly be changed after 
	lobby has already been created
	@param StartingResources - resources for players to start with when match begins
	@param DefeatCondition - the defeat condition for match */
	void CreateNetworkedSession(const FText & LobbyName, bool bIsLAN, const FText & Password,
		uint32 NumSlots, uint8 MapID, EStartingResourceAmount StartingResources,
		EDefeatCondition DefeatCondition);

	/* Called from game session when it tries to create a networked session
	@param SessionSettings - all the settings for the session such as max players, if LAN etc
	@param bSuccessful - whether creating the session was successful or not */
	void OnNetworkedSessionCreated(const TSharedPtr <FOnlineSessionSettings> InSessionSettings, bool bSuccessful);

protected:

	/* These 3 vars are here to persist across map changes... think it's needed */

	/* Session settings here so they persist across map changes */
	TSharedPtr <FOnlineSessionSettings> SessionSettings;

	/* Only known on host */
	FString LobbyPassword;

	/* Session settings for session we are trying to create. Cached here so delegates can reference
	it when trying to recreate session */
	TSharedPtr <FOnlineSessionSettings> PendingSessionSettings;

	void CreateMultiplayerLobby(const TSharedPtr <FOnlineSessionSettings> InSessionSettings);

	void LoadLobbyMapForMultiplayer();

	UFUNCTION()
	void SetupLobbyWidget_Multiplayer();

public:

	TSharedPtr <FOnlineSessionSettings> & GetSessionSettings();

	const FString & GetLobbyPassword() const;
	void SetLobbyPassword(const FString & InPassword);

	TSharedPtr <FOnlineSessionSettings> & GetPendingSessionSettings();
	// Maybe param needs to be a ref I don't know
	void SetPendingSessionSettings(const TSharedPtr <FOnlineSessionSettings> InSessionSettings);


	//==========================================================================================
	//	Going from lobby or match to main menu
	//==========================================================================================

public:

	void GoFromLobbyToMainMenu();
	void GoFromMatchToMainMenu();

	/* This is called by GM::ReturnToMainMenuHost, which bubbles to game session then to PC impl
	then this. This will be called by both host and client when the host leaves the lobby */
	virtual void ReturnToMainMenu() override;


	//==========================================================================================
	//	Searching for and Joining Lobbies
	//==========================================================================================

public:

	void SearchForNetworkedLobbies();

	/* URTSLocalPlayer::GetGameLoginOptions() passes in password info amoung other things */
	void TryJoinNetworkedSession(const FOnlineSessionSearchResult & SearchResult);


	//==========================================================================================
	//	Transition from lobby to match
	//==========================================================================================

public:

	/* The first function called when lobby is requesting a match to be created */
	virtual void TryCreateMatch(EMatchType MatchNetworkType);

	/* Called by game session when delegate fires */
	virtual void OnStartSessionComplete(bool bWasSuccessful);

	/* Assign a starting spot to players that do not have one. -1 means they were not assigned 
	a spot. By the end of func every entry should be either >= 0 or -2. -2 means could not 
	find spot for player 
	@param PlayerInfo - info about all the players in the match. Important to only change 
	their starting spots and nothing else */
	virtual void AssignOptimalStartingSpots(const FMapInfo & MatchMap, 
		TArray < FMinimalPlayerInfo > & PlayerInfo);

	/* The function called from lobby when a match is going to happen */
	void LoadMatch();

	/* Called my GS when level has loaded */
	void OnMatchLevelLoaded();

	/* Call when all players have connected, have acknowledged they have completed their client
	post login and have streamed in the level */
	void OnLevelLoadedAndPCSetupsDone();

	/* Call when all human players have acknowledged they have received the initial values for
	each player state (including CPU players player states) */
	void OnInitialValuesAcked();

	/* Called when all player states have completed AMyPlayerState::Client_FinalSetup */
	void OnMatchFinalSetupComplete();

	const FMatchInfo & GetMatchInfo() const;
	FMatchInfo & GetMatchInfoModifiable();

	const FText & GetMatchLoadingStatusText(ELoadingStatus LoadingStatus) const;

	bool IsColdBooting() const;
	void SetIsColdBooting(bool bNewValue);

	/* Whether we are in the main menu map or not */
	bool IsInMainMenuMap() const;

protected:

	/* Because same game mode used for all maps. Helps GM know to show intro or not. True while
	main menu has never been seen */
	bool bIsColdBooting;

	/* Because same game mode if used for main menu, lobby and matches this helps game mode
	decide what to do each time it initializes and call BeginPlay etc */
	bool bIsInMainMenuMap;

	/* Info about match for when level has loaded. Saving as a reference/pointer will not
	work after OpenLevel because widgets must get destroyed after map change */
	FMatchInfo MatchInfo;

public:

	/** 
	 *	Setup CPU player 
	 *	
	 *	@param bSetup - whether to call Setup on the AI controller 
	 */
	ACPUPlayerAIController * SpawnCPUPlayer(ECPUDifficulty InCPUDifficulty, ETeam InTeam, 
		EFaction InFaction, int16 StartingSpot, const TArray <int32> & InStartingResources);

protected:

	/* Spawn the buildings/units that each player starts with. 
	@return - a TMap that maps player state to the types of selectables they started match with. 
	This is here for CPU player AI controllers */
	virtual TMap < ARTSPlayerState *, FStartingSelectables > SpawnStartingSelectables();


	//==========================================================================================
	//	Development
	//==========================================================================================

public:

#if WITH_EDITOR
	void OnInitiateSkipMainMenu();
	
	/* Partially setup MatchInfo for when playing in PIE. Assumption about the param array is
	that host is at index 0, client 1 at index 1, etc. Some info is not added such as match map
	or starting resources
	@param NewTeams - maps old team to new team, where new team is the lowest enum value possible */
	void SetupMatchInfoForPIE(int32 InNumTeams, const TArray < ARTSPlayerState * > & PlayerStates,
		const TMap < ETeam, ETeam> & NewTeams);
#endif

protected:

#if WITH_EDITORONLY_DATA
	/** 
	 *	Class that holds settings that can be changed while not in shipping build to speed
	 *	up development 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Development", meta = (DisplayName = "Development Settings"))
	TSubclassOf <ADevelopmentSettings> DevelopmentSettings_BP;

	/* Development settings actor once spawned */
	UPROPERTY()
	ADevelopmentSettings * DevelopmentSettings;

	/* Alternative to DevelopmentSettings */
	UEditorPlaySettingsWidget * EditorPlaySettingsWidget;
#endif

public:

#if WITH_EDITORONLY_DATA
	/* Functions to get the setting to use for a PIE/standalone session. They will likely either 
	query the ADevelopmentSettings actor or the editor play settings editor utility widget */
	bool EditorPlaySession_ShouldSkipOpeningCutscene() const;
	bool EditorPlaySession_ShouldSkipMainMenu() const;
	EDefeatCondition EditorPlaySession_GetDefeatCondition() const;
	const TArray<FPIEPlayerInfo> & EditorPlaySession_GetHumanPlayerInfo() const;
	int32 EditorPlaySession_GetNumCPUPlayers() const;
	const TArray<FPIECPUPlayerInfo> & EditorPlaySession_GetCPUPlayerInfo() const;
	EInvalidOwnerIndexAction EditorPlaySession_GetInvalidHumanOwnerRule() const;
	EInvalidOwnerIndexAction EditorPlaySession_GetInvalidCPUOwnerRule() const;
	bool EditorPlaySession_IsCheatWidgetBPSet() const;
	bool EditorPlaySession_ShouldInitiallyShowCheatWidget() const;
	TSubclassOf<UInMatchDeveloperWidget> EditorPlaySession_GetCheatWidgetBP() const;
	bool EditorPlaySession_ShowInfantryControllerDebugWidgets() const;
	TSubclassOf<UInfantryControllerDebugWidget> EditorPlaySession_GetInfantryControllerDebugWidgetBP() const;
private:
	const FStartingResourceConfig & EditorPlaySession_GetStartingResourceConfig() const;
#endif
};


//==============================================================================================
//	Structs implemented in Menus.cpp
//==============================================================================================

USTRUCT()
struct FMinimalPlayerInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	bool bIsForHumanPlayer;

	UPROPERTY()
	ETeam Team;

	UPROPERTY()
	EFaction Faction;

	// This can change during GI::AssignOptimalStartingSpots
	UPROPERTY()
	int16 StartingSpot;

public:

	FMinimalPlayerInfo();
	FMinimalPlayerInfo(bool bIsForHuman, ETeam InTeam, EFaction InFaction, int16 InStartingSpotID);

	bool IsHumanPlayer() const;
	ETeam GetTeam() const;
	EFaction GetFaction() const;
	int16 GetStartingSpot() const;
	void SetStartingSpot(int16 InStartingSpot);
};
