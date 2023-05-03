// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/Structs_1.h"
#include "Statics/Structs_2.h"
#include "UI/WorldWidgets/WorldWidget.h" // Suddenly one day I needed this for TSubclassOf<UWorldWidget>, gross
#include "Statics/Statics.h"
#include "FactionInfo.generated.h"

class AObjectPoolingManager;
class AStartingGrid;
class UInGameWidgetBase;
class ABuilding;
class AGhostBuilding;
class UMaterialInterface;
class UWorldWidget;
class UParticleSystem;
class UUpgradeEffect;
class USoundBase;
class UCommanderSkillTreeWidget;


/* An integer that is clamped to always be at least 1 */
USTRUCT()
struct FAtLeastOneInt32
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int32 Integer;

public:

	FAtLeastOneInt32() : Integer(1) { }

	int32 GetInteger() const { return Integer; }
};


USTRUCT()
struct FUnitTypeArray
{
	GENERATED_BODY()

protected:

	TArray < EUnitType > Array;

public:

	int32 Emplace(EUnitType UnitType) { return Array.Emplace(UnitType); }

	const TArray < EUnitType > & GetArray() const { return Array; }
};


USTRUCT()
struct FBuildingTypeArray
{
	GENERATED_BODY()

protected:

	TArray < EBuildingType > Array;

public:

	int32 Emplace(EBuildingType BuildingType) { return Array.Emplace(BuildingType); }

	const TArray < EBuildingType > & GetArray() const { return Array; }
};


/* An array of FContextButton */
USTRUCT()
struct FButtonArray
{
	GENERATED_BODY()

protected:

	TArray < FContextButton > Buttons;

public:

	void Sort();

	const TArray < FContextButton > & GetButtons() const;

	/* Add button to Buttons, based on user defined ordering with HUDPersistentTabButtonOrdering */
	void AddButton(const FContextButton & InButton);
};


/* Holds information about things that happen when leveling up */
USTRUCT()
struct FRankInfo
{
	GENERATED_BODY()

private:

	/* Optional. Upgrade effect applied to unit when reaching this level */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf <UUpgradeEffect> Bonus;

	/**
	 *	Optional. Particle effect to play when reaching this level. If multiple levels are
	 *	gained at the same time then only the particle effect for the highest level gained
	 *	will be played
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * Particles;

	/* Sound to play when a unit reaches this level */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound;

	/**
	 *	Image to display on unit for this unit. If you are choosing to display level icons
	 *	on your unit widgets then you will need to give this a value 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Icon;

public:

	FRankInfo() 
		: Bonus(nullptr) 
		, Particles(nullptr) 
		, Sound(nullptr) 
		, Icon(nullptr)
	{
	}

	TSubclassOf <UUpgradeEffect> GetBonus() const;
	UParticleSystem * GetParticles() const;
	USoundBase * GetSound() const;
	UTexture2D * GetIcon() const;
};


/**
 *	A info class that holds all the data about a faction. It is a pure info class. This object 
 *	stores no state.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AFactionInfo final : public AInfo
{
	GENERATED_BODY()

public:

	AFactionInfo();

protected:

	virtual void BeginPlay() override;

private:

	/* Reference to object pooling manager */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	/* The faction this info is for */
	EFaction Faction;

	/* The name to appear in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/* Image to appear in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UTexture2D * Icon;

	/* Buildings that can be built by this faction. */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "Building Roster"))
	TArray< TSubclassOf < ABuilding > > Buildings;
	
	/* Units that can be built by this faction */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "Unit Roster", MustImplement = "Selectable"))
	TArray < TSubclassOf < AActor > > Units;

	/** 
	 *	The starting grid to use for this faction. 
	 *	
	 *	Starting grids define what and where buildings/units this faction starts a match with 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf < AStartingGrid > StartingGrid;

	/* Upgrades this faction can research */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EUpgradeType, FUpgradeInfo> Upgrades;

	/** 
	 *	These are upgrades that can be researched through a building as opposed to being 
	 *	aquired by a commander ability. 
	 *
	 *	Note: I currently don't use this at any other time besides setup. It could be removed 
	 *	as a class variable to save on memory.
	 */
	TSet<EUpgradeType> UpgradesResearchableThroughBuildings;

	/** 
	 *	Maps level to info about what happens to a selectable upon reaching that level. 
	 *	
	 *	The entry for the starting level will ignore the bonus and particles - it is just for 
	 *	the icon
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Leveling Up Info"))
	TMap <uint8, FRankInfo> LevelUpInfo;

	/* What happens when the commander gains a rank. Maps rank to the bonuses */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<uint8, FCommanderLevelUpInfo> CommanderLevelUpInfo;

	/* How many commander skill points players who play this faction should start the game with */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	int32 NumInitialCommanderSkillPoints;

	/* Holds information for all buildings for this faction */
	UPROPERTY()
	TMap< EBuildingType, FBuildingInfo > BuildingInfo;

	/* Holds information for all units for this faction */
	UPROPERTY()
	TMap< EUnitType, FUnitInfo > UnitInfo;

	/* Holds bonuses from level ups for selectables as created UObjects */
	UPROPERTY()
	TMap <uint8, UUpgradeEffect *> LevelUpBonuses;

	/* Initialize */
	void Initialize(URTSGameInstance * GameInst);

	/* Fill HUDPersistentTabButtons with emtpy arrays for all tab types */
	void InitHUDPersistentTabButtons();

	/* Fills the PS array BuildingInfo with info on all the
	buildings this faction can make. The ordering is the
	order they were added in the editor */
	void InitBuildingInfo();

	/* Fills the PS array UnitInfo with info on all the
	units this faction can make. The ordering is the
	order they were added in the editor */
	void InitUnitInfo();

	/** 
	 *	Checks if the ghost building has all the right properties to be
	 *	a ghost for a building. Throws assert if it is not
	 *	
	 *	@param GhostBuilding - ghost building
	 *	@param Building - actual building 
	 */
	void CheckGhost(const AGhostBuilding * GhostBuilding, const ABuilding * Building) const;

	/* Create the FText for each building/units prerequisites */
	void CreatePrerequisitesText();

	/* Do setup type stuff for upgrades */
	void InitUpgradeInfo();

	/* Clears BuildingInfo */
	void EmptyBuildingInfo();

	/* Clears UnitInfo */
	void EmptyUnitInfo();

	void SortHUDPersistentTabButtons();

	/* Sets up TMap from button types to what HUD persistent tab they belong in. This is mainly
	for the HUD, giving it fast lookups to what tab a button resides in when it is added to a
	production queue */
	void InitButtonToPersistentTabMappings();

	/* Sets up LevelUpBonuses */
	void InitLevelUpBonuses();

	void InitMouseCursors(URTSGameInstance * GameInst);

	/** 
	 *	Optional override for mouse cursor to use when playing matches as this faction. 
	 *	
	 *	Leaving Cursor Path blank will default to using the one defined in game instance 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType MouseCursor;

	/* 8 edge scrolling cursors */
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

	FCursorInfo MouseCursor_Info;
	
	/** 
	 *	Array of the 8 mouse cursors that appear when the player has their mouse close 
	 *	enough to the edge of the screen that the camera moves 
	 *	
	 *	Index 0 = left
	 *	Index 1 = right 
	 *	Index 2 = up 
	 *	Index 3 = left + up 
	 *	Index 4 = right + up 
	 *	Index 5 = down 
	 *	Index 6 = left + down 
	 *	Index 7 = right + down 
	 *
	 *	The indices are set out like this so just additions can be used to figure out the index 
	 *	in ARTSPlayerController::MoveIfAtEdgeOfScreen
	 */
	FCursorInfo EdgeScrollingCursor_Infos[8];

	/*********************************************************************************************
	 *	Different mouse cursors for when the mouse is hovered over a selectable. Some of these 
	 *	may be able to be overridden on a per-unit basis 
	**********************************************************************************************/
	
	/** 
	 *	Default mouse cursor to show when hovering the mouse over a hostile unit and it can 
	 *	be attacked by at least one selected selectable 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CanAttackHoveredHostileUnit;
	/**
	 *	Default mouse cursor to show when hovering the mouse over a hostile unit and it cannot
	 *	be attacked by any selected selectables 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CannotAttackHoveredHostileUnit;
	/**
	 *	Default mouse cursor to show when hovering the mouse over a friendly unit and it can 
	 *	be 'attacked' by at least one selected selectable. The attack is hopefully something
	 *	helpful that heals the target or something.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CanAttackHoveredFriendlyUnit;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CannotAttackHoveredFriendlyUnit;
	/* Same deal as the unit cursors except for buildings instead */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CanAttackHoveredHostileBuilding;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CannotAttackHoveredHostileBuilding;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CanAttackHoveredFriendlyBuilding;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding;
	/** 
	 *	Default mouse cursor to show when hovering mouse over an inventory item in the world and at least 
	 *	one selected selectable can pick it up 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CanPickUpHoveredInventoryItem;
	/** 
	 *	Default mouse cursor to show when hovering mouse over an inventory item in the world and no 
	 *	selected selectable can pick it up. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType DefaultMouseCursor_CannotPickUpHoveredInventoryItem;
	/**
	 *	Default cursors to show when hovering mouse over a resource spot and at least one selected 
	 *	selectable can gather from it. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EResourceType, EMouseCursorType> DefaultMouseCursors_CanGatherFromHoveredResourceSpot;
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EResourceType, EMouseCursorType> DefaultMouseCursors_CannotGatherFromHoveredResourceSpot;

	FCursorInfo DefaultMouseCursor_CanAttackHoveredHostileUnit_Info;
	FCursorInfo DefaultMouseCursor_CannotAttackHoveredHostileUnit_Info;
	FCursorInfo DefaultMouseCursor_CanAttackHoveredFriendlyUnit_Info;
	FCursorInfo DefaultMouseCursor_CannotAttackHoveredFriendlyUnit_Info;
	FCursorInfo DefaultMouseCursor_CanAttackHoveredHostileBuilding_Info;
	FCursorInfo DefaultMouseCursor_CannotAttackHoveredHostileBuilding_Info;
	FCursorInfo DefaultMouseCursor_CanAttackHoveredFriendlyBuilding_Info;
	FCursorInfo DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding_Info;
	FCursorInfo DefaultMouseCursor_CanPickUpHoveredInventoryItem_Info;
	FCursorInfo DefaultMouseCursor_CannotPickUpHoveredInventoryItem_Info;
	/* Use Statics::ResourceTypeToArrayIndex to get the right index */
	FCursorInfo DefaultMouseCursor_CanGatherFromHoveredResourceSpot_Info[Statics::NUM_RESOURCE_TYPES];
	FCursorInfo DefaultMouseCursor_CannotGatherFromHoveredResourceSpot_Info[Statics::NUM_RESOURCE_TYPES];
	
	//-------------------------------------------------------------------------------------------

	/* The widget to attach to all selectables when playing this faction excluding resource spots */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<UWorldWidget> SelectablePersistentWidget;

	/* The widget to attach to all selectables when they are selected excluding resource spots */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<UWorldWidget> SelectableSelectionWidget;

	/* The widget to attach to all resource spots. This widget should not have any health 
	displaying widgets bound */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<UWorldWidget> ResourceSpotPersistentWidget;

	/* The widget to attach to a resource spot when it is selected. This widget should not have 
	any health displaying widgets bound */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<UWorldWidget> ResourceSpotSelectionWidget;

	/* Decal to show under a selectable when it is selected. Set size individually on each
	selectable */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap<EAffiliation, FSelectionDecalInfo> SelectionDecals;

	/* Maps affiliation to particles to display on selectable when selecting them. Different
	size templates can be set instead of just scaling the particles which sometimes does not look
	right */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap<EAffiliation, FVaryingSizeParticleArray> SelectionParticles;

	/* Maps affiliation to particles to display on a selectable when right-clicking it */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < EAffiliation, FVaryingSizeParticleArray > RightClickCommandParticles;

	/* Particle system to spawn on world when right mouse click command is issued, if any. Note
	these do not attach to the selectable clicked so the target ones might want to be left blank */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap < ECommandTargetType, FParticleInfo > RightClickCommandStaticParticles;

	/* Decal to show under a selectable when right-clicking it. Setup size in each specific
	selectable's BP. These do attach to the selectable clicked */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS")
	TMap <EAffiliation, FDecalInfo> RightClickCommandDecals;

	void InitSelectionDecals();

	void CreateTechTree();

	/* Setup all the HUD warnings */
	void InitHUDWarningInfo(URTSGameInstance * GameInst);

	/** 
	 *	Optional set of widgets than can override the default in-game widgets.
	 *
	 *	Any widget types without entries will use the widget defined in the game instance. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "In-Game Widget Overrides"))
	TMap < EMatchWidgetType, TSubclassOf < UInGameWidgetBase > > MatchWidgets_BP;

	/** 
	 *	Optional warning message overrides for events such as 'Production queue is full'. 
	 *
	 *	Any warning types without entries will use the message defined in game instance. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EGameWarning, FGameWarningInfo > HUDMessages;

	/** 
	 *	Optional warning message overrides for whatever custom checks are performed by abilities
	 *	e.g. "Target not below 30% HP", "Buff not present on target" 
	 *
	 *	Any warning types without entries will use the message defined in game instance. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EAbilityRequirement, FGameWarningInfo> AbilityHUDMessages;

	/** 
	 *	Optional warning message overrides for times when you try to spend more resources than 
	 *	you have. 
	 *	
	 *	Any resource types without entries will use the message defined in game instance 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EResourceType, FGameWarningInfo> MissingResourceHUDMessages;

	/** 
	 *	Optional warning message overrides for times when you do not have enough housing resources 
	 *
	 *	Any resource types without entries will use the message defined in game instance
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EHousingResourceType, FGameWarningInfo> MissingHousingResourceHUDMessages;

	/* Maps HUD persistent tab type to sorted array of order buttons should appear on that tab */
	UPROPERTY()
	TMap <EHUDPersistentTabType, FButtonArray> HUDPersistentTabButtons;

	/* Maps button type to what HUD persistent tab it should be in */
	UPROPERTY()
	TMap <FTrainingInfo, EHUDPersistentTabType> ButtonTypeToPersistentTabType;
	
#if WITH_EDITORONLY_DATA
	/** 
	 *	How much housing resources are always provided no matter what. 
	 *	
	 *	e.g. in SCII if you have 0 pylons and 0 nexus then you have 0 supply. However if this 
	 *	variable is 5 then you would have 5. After you build a pylon you would have 13.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EHousingResourceType, int32> ConstantHousingResourceAmountProvided;
#endif

	/* Copy of ConstantHousingResourceAmountProvided that is populated on post edit. It exists 
	for performance only */
	UPROPERTY()
	TArray <int32> ConstantHousingResourceAmountProvidedArray;

#if WITH_EDITORONLY_DATA
	/** 
	 *	Limits on the max value a housing resource can provide. Leaving no entry means the 
	 *	resource has no limit.
	 *	
	 *	e.g. in SCII population has a limit of 200 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EHousingResourceType, int32> HousingResourceLimits;
#endif

	/* Copy of HousingResourceLimits that is populated in post edit. It exists for performance only */
	UPROPERTY()
	TArray <int32> HousingResourceLimitsArray;

	/** 
	 *	Maps building type to the maximum amount of that building each player is allowed to 
	 *	have when playing as this faction. e.g. in C&C generals sometimes each player is only 
	 *	allowed to build a maximum of one superweapon.
	 *	
	 *	Maps from building type to the limit. All values should be at least one. If you do not 
	 *	want a limit then just remove its key/value pair from this container.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EBuildingType, FAtLeastOneInt32> BuildingQuantityLimits;

	/** 
	 *	Same as BuildingQuantityLimits except for units instead e.g. in RA2 only one tanya can 
	 *	be built per player at a time 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EUnitType, FAtLeastOneInt32> UnitQuantityLimits;

	/** 
	 *	The default way buildings are built for this faction. This can be overridden on a per
	 *	building basis 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingBuildMethod DefaultBuildingBuildMethod;

	/* The distance a building must be from another owned/allied (see bCanBuildOffAllies below)
	building for it to be built. This is measured from the center of the building trying to be
	placed to the edge of other buildings. 0 = unlimited. This can be overridden and ignored on a
	per building basis
	e.g. In C&C 3 and many other C&Cs there is a rule where the building must be placed close to
	other buildings. In SCII there is no restriction */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float BuildingProximityRange;

	/* Whether BuildingProximityRange can use allied buildings aswell. So if true you can build
	off allies buildings too. An example of true is in Red Alert 2 where you can place buildings
	near friendlies buildings

	Because BuildingProximityRange can be overridden on a per building basis this can still be
	relevant even if BuildingProximityRange = 0 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanBuildOffAllies;

	/* Optional overrides for the default image set in game instance. These are for different
	resource types */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EResourceType, UTexture2D *> ResourceIconOverrides;

	/* Music to play when playing match as this faction */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * MatchMusic;

	/* Sound to play when setting a rally point for a unit producing building */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * ChangeRallyPointSound;

	/* Buildings that have a persistent queue i.e. are construction yard type buildings. This 
	is mainly here for CPU player AI controllers */
	UPROPERTY()
	TArray <EBuildingType> PersistentQueueTypes;

	/* Units that can build at least one building. This is mainly here for CPU player AI 
	controllers */
	UPROPERTY()
	TArray <EUnitType> WorkerTypes;

	/* Unit types that are considered 'attacking' types. Anything not a collector or a worker 
	will be considered an attacking type. This is mainly here for CPU player AI controller */
	UPROPERTY()
	TArray <EUnitType> AttackingUnitTypes;

	/* Maps resource type to unit types that can gather that resource. This is mainly here for 
	CPU player AI controller */
	UPROPERTY()
	TMap <EResourceType, FUnitTypeArray> CollectorTypes;

	/* Maps resource type to the type of buildings that serve as a drop point for it. Mainly 
	here for CPU player AI controllers */
	UPROPERTY()
	TMap < EResourceType, FBuildingTypeArray > ResourceDepotTypes;

	/* Array of all building types that can build at least one unit. Does not distinguish 
	between whether they are a worker/collector/army unit. Mainly here for CPU player AI 
	controllers */
	UPROPERTY()
	TArray < EBuildingType > BarracksTypes;

	/* Array of building types that are considered base defenses. Mainly here for CPU player 
	AI controller */
	UPROPERTY()
	TArray < EBuildingType > BaseDefenseTypes;

	/* Largest context menu on this faction */
	int32 LargestContextMenu;

	/* Largest production queue for all buildings on this faction's building roster. 
	Context queues only */
	int32 LargestProductionQueueCapacity;

	/* Most unit garrison slots a building on this faction has */
	int32 LargestBuildingGarrisonSlotCapacity;

	/* The maximum number of items a shop on this faction has on display and/or sells */
	int32 MaxNumberOfShopItemsOnAShop;

	/* The maximum size inventory a selectable on this faction has */
	int32 MaxUnitInventoryCapacity;

	/* For sorting */
	friend bool operator<(const AFactionInfo & Info1, const AFactionInfo & Info2);

#if WITH_EDITOR
	/* To change visibility of options in upgrades */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

public:

	/* This should not be called by anything other than the game instance when it is initializing
	faction info */
	void SetInitialFaction(EFaction InFaction);

	const FText & GetName() const;

	UTexture2D * GetFactionIcon() const;

	const FBuildingInfo * GetBuildingInfo(EBuildingType Type) const;

	/* This version is slow and is only run during the spawning of starting selectables. 
	It can be made faster by using a TMap that maps from actor blueprint to a FBuildingInfo 
	pointer (or the Tset SelectableBPs could be converted for this purpose, would have to 
	remove the WITH_EDITORONLY_DATA that wraps it though). The reason I do not want to add 
	another data structure is basically we'll just accept it is slowish during starting 
	selectable spawn but we do not polute CPU cache more 
	
	@return - null if faction does not have a the building on its building roster */
	const FBuildingInfo * GetBuildingInfoSlow(TSubclassOf <AActor> BuildingBP) const;

	const FUnitInfo * GetUnitInfo(EUnitType Type) const;

	/* Get info for an upgrade type. Assumes that an entry is in the upgrade roster */
	const FUpgradeInfo & GetUpgradeInfoChecked(EUpgradeType Type) const;
	FUpgradeInfo & GetUpgradeInfoMutable(EUpgradeType Type);

	/* Returns pointer to upgrade info if the upgrade type is on the faction's upgrade roster. 
	Otherwise it will return null */
	const FUpgradeInfo * GetUpgradeInfoNotChecked(EUpgradeType UpgradeType) const;

	/* Return true if an upgrade is on the context menu of at least one building on this 
	faction's building roster */
	bool IsUpgradeResearchableThroughBuilding(EUpgradeType UpgradeType) const;

#if WITH_EDITORONLY_DATA

	/* Stores all the blueprints for each selectable on this faction. Here to speed up the 
	process of removing already placed on map selectables right before a PIE session */
	UPROPERTY()
	TSet <TSubclassOf < AActor >> SelectableBPs;
	
	/* Return whether this faction has this selectable on their roster */
	bool HasSelectable(const TSubclassOf < AActor > & ActorBP) const;

#endif 

	/* Get the time it takes to produce something in a production queue */
	float GetProductionTime(const FTrainingInfo & TrainingInfo) const;

	TSubclassOf < AStartingGrid > GetStartingGrid() const;

	/* Get what faction this info is for */
	EFaction GetFaction() const;

	/* Get reference to upgrades array */
	const TMap <EUpgradeType, FUpgradeInfo> & GetUpgradesMap() const;

	/* Get info for a level/rank */
	const FRankInfo & GetLevelUpInfo(uint8 Level) const;

	UUpgradeEffect * GetLevelUpBonus(uint8 Level) const;

	/* Get info struct for a certain rank of commander */
	const FCommanderLevelUpInfo & GetCommanderLevelUpInfo(uint8 InRank) const;

	/* Get how many skill points the commander should start the game with */
	int32 GetNumInitialCommanderSkillPoints() const;

	/* Get info on this faction's default match mouse cursor */
	const FCursorInfo & GetMouseCursorInfo() const;

	/* Mouse cursors for when the mouse is close enough to the edge of the screen that the 
	camera moves */
	const FCursorInfo & GetEdgeScrollingCursorInfo_Top() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_TopRight() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_Right() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_BottomRight() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_Bottom() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_BottomLeft() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_Left() const;
	const FCursorInfo & GetEdgeScrollingCursorInfo_TopLeft() const;

	/* Gets the mouse cursor given the caller has already calculated the array index */
	const FCursorInfo & GetEdgeScrollingCursorInfo(int32 ArrayIndex) const;

	const FCursorInfo & GetDefaultMouseCursorInfo_CanAttackHoveredHostileUnit() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotAttackHoveredHostileUnit() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyUnit() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyUnit() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CanAttackHoveredHostileBuilding() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotAttackHoveredHostileBuilding() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyBuilding() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyBuilding() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CanPickUpHoveredInventoryItem() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotPickUpHoveredInventoryItem() const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CanGatherFromHoveredResourceSpot(EResourceType ResourceType) const;
	const FCursorInfo & GetDefaultMouseCursorInfo_CannotGatherFromHoveredResourceSpot(EResourceType ResourceType) const;

	/* Get the persistent selectable world user widget */
	const TSubclassOf<UWorldWidget> & GetSelectablePersistentWorldWidget() const;

	/* Get the widget to show only when selected */
	const TSubclassOf <UWorldWidget> & GetSelectableSelectionWorldWidget() const;

	/* Get the persistent widget for resource spots */
	const TSubclassOf <UWorldWidget> & GetResourceSpotPersistentWorldWidget() const;

	/* Get the widget to show on resource spots only when they are selected */
	const TSubclassOf <UWorldWidget> & GetResourceSpotSelectionWorldWidget() const;

	/* Get decal to appear under selectable when selected. */
	const FSelectionDecalInfo & GetSelectionDecalInfo(EAffiliation Affiliation) const;

	/* Get the particle system template to attach to a selectable because it was selected
	@param Affiliation - affiliation of selectable towards local player
	@param SelectablesSize - how large the selectable considers itself */
	UParticleSystem * GetSelectionParticles(EAffiliation Affiliation, int32 SelectablesSize) const;

	/* Get the particle system template to attach to a selectable because it was right-clicked
	@param Affiliation - affiliation of selectable towards local player
	@param SelectablesSize - how large the selectable considers itself */
	UParticleSystem * GetRightClickParticles(EAffiliation Affiliation, int32 SelectablesSize) const;

	/* Get info about a particle system to spawn when issuing right click commands. This particle
	system does not attach to selectables */
	const FParticleInfo & GetRightClickCommandStaticParticles(ECommandTargetType TargetType) const;

	/* Get info about a decal to show when issuing right click commands. Pointer to material
	in struct can be null
	@param TargetAffiliation - affiliation of selectable clicked on */
	const FDecalInfo & GetRightClickCommandDecal(EAffiliation TargetAffiliation) const;

	/* Get blueprint for in-game widget */
	TSubclassOf < UInGameWidgetBase > GetMatchWidgetBP(EMatchWidgetType WidgetType) const;

	/* Get message like "Not enough resources" if one is set, or null otherwise */
	const FText & GetHUDErrorMessage(EGameWarning MessageType) const;
	const FText & GetHUDErrorMessage(EAbilityRequirement ReasonForMessage) const;
	const FText & GetHUDMissingResourceMessage(EResourceType ResourceType) const;
	const FText & GetHUDMissingHousingResourceMessage(EHousingResourceType HousingResourceType) const;

	/* Get sound for the warnings */
	USoundBase * GetWarningSound(EGameWarning MessageType) const;
	USoundBase * GetWarningSound(EAbilityRequirement ReasonForMessage) const;
	USoundBase * GetWarningSound(EResourceType ResourceType) const;
	USoundBase * GetWarningSound(EHousingResourceType HousingResourceType) const;

	/* Given a HUD persistent tab type and index, get the button for that tab at index, or null
	if no button for that combination exists. Tab category None can be used and is possible to
	return valid pointer */
	const FContextButton * GetHUDPersistentTabButton(EHUDPersistentTabType TabType, int32 ButtonIndex) const;

	const TMap < EHUDPersistentTabType, FButtonArray > & GetHUDPersistentTabButtons() const;

	/* Given some training info get the the HUD persistent tab it should belong in */
	EHUDPersistentTabType GetHUDPersistentTab(const FTrainingInfo & InInfo) const;
	EHUDPersistentTabType GetHUDPersistentTab(const FContextButton & InButton) const;

	/* Get how much of housing resources is always provided */
	const TArray < int32 > & GetConstantHousingResourceAmountProvided() const;

	/* Get the limits for each housing resource e.g. in SCII it is 200 */
	const TArray < int32 > & GetHousingResourceLimits() const;

	/** 
	 *	Return whether there is a limit on the number of buildings/units of a particular 
	 *	type the player is allowed 
	 *	
	 *	@return - true if there is some kind of limit 
	 */
	bool IsQuantityLimited(EBuildingType BuildingType) const;
	bool IsQuantityLimited(EUnitType UnitType) const;

	/** 
	 *	Returns what the quantity limit of a selectable is. Will crash if quantity of the passed 
	 *	in type is not limited. Should always return at least 1 
	 */
	int32 GetQuantityLimit(EBuildingType BuildingType) const;
	int32 GetQuantityLimit(EUnitType UnitType) const;

	/* Get build info for a context button. Assumes that it is either for a building or unit
	and not an upgrade */
	const FBuildInfo * GetBuildInfo(const FContextButton & ContextButton) const;

	/* Given a context button, get the display info that corrisponds to it */
	const FDisplayInfoBase * GetDisplayInfo(const FContextButton & ContextButton) const;

	const TMap <EUnitType, FAtLeastOneInt32> & GetUnitQuantityLimits() const;

	/* Get the build method to use for building buildings for this faction. Buildings can override
	this if they choose */
	EBuildingBuildMethod GetDefaultBuildingBuildMethod() const;

	/* Get the default building proximity range */
	float GetBuildingProximityRange() const;

	/* Get whether this faction can build off allied buildings */
	bool CanBuildOffAllies() const;

	/* Whether this faction has overridden a resource image */
	bool HasOverriddenResourceImage(EResourceType ResourceType) const;

	/* Get image that represents a resource. Assuming it has already been checked that faction
	overrides image */
	UTexture2D * GetResourceImage(EResourceType ResourceType) const;

	/* Get the music to play during a match */
	USoundBase * GetMatchMusic() const;

	// Get sound to play when changing rally point
	USoundBase * GetChangeRallyPointSound() const;

	/* Get how many different upgrades this faction has */
	int32 GetNumUpgrades() const;

	/* Was added here so the keys can be iterated over to know what buildings are on this 
	faction's building roster. */
	const TMap <EBuildingType, FBuildingInfo> & GetAllBuildingTypes() const;

	const TArray <EBuildingType> & GetPersistentQueueTypes() const;

	/* Was added to be able to know all the unit types this faction has on it's unit roster */
	const TMap < EUnitType, FUnitInfo > & GetAllUnitTypes() const;

	// Get array of all unit types that are considered worker types
	const TArray <EUnitType> & GetWorkerTypes() const;

	/* Given a resource type get all the units that can gather that resource */
	const TArray <EUnitType> & GetCollectorTypes(EResourceType ResourceType) const;

	/* Given a resource type get all the buildings that can serve as a drop point for it. This 
	is mainly here fore the CPU player AI controller */
	const TArray <EBuildingType> & GetAllDepotsForResource(EResourceType ResourceType) const;

	/* Get all buildings that can train a unit */
	const TArray <EBuildingType> & GetBarracksBuildingTypes() const;

	/* Get a list of all unit types that are considered attacking units */
	const TArray <EUnitType> & GetAttackingUnitTypes() const;

	/* Get all buildings that are considered base defenses */
	const TArray <EBuildingType> & GetBaseDefenseTypes() const;

	/* For a given resource type returns whether the faction has at least one unit on its unit 
	roster that can collect that resource */
	bool HasCollectorType(EResourceType ResourceType) const;

	/* Returns whether this faction has at least one upgrade on its upgrade roster */
	bool HasUpgrades() const;

	/* Returns whether this faction has base defense buildings on its building roster */
	bool HasBaseDefenseTypeBuildings() const;

	/* Return whether this faction has any army units on its unit roster */
	bool HasAttackingUnitTypes() const;

	/* Get a random building that is a construction yard. Returns EBuildingType::NotBuilding 
	if this faction has none */
	EBuildingType Random_GetPersistentQueueSupportingType() const;

	/* Get a random unit that can build at least one building. Returns EUnitType::None if 
	this faction has no units that can build buildings */
	EUnitType Random_GetWorkerType() const;

	/* Get a random unit this is considered an 'attacking' unit. Returns EUnitType::None if 
	no unit on this faction is an attacking unit */
	EUnitType Random_GetAttackingUnitType() const;

	/* Get a random base defense building type. Returns EBuildingType::NotBuilding if faction 
	has no base defense buildings */
	EBuildingType Random_GetBaseDefenseType() const;

	/* Get how many buttons the selectable with the most context buttons on this faction has */
	int32 GetLargestContextMenu() const;

	/* Returns the largest production queue for buildings this faction has */
	int32 GetLargestProductionQueueCapacity() const;

	int32 GetLargestBuildingGarrisonSlotCapacity() const;

	/* Return the maximum number of items a shop on this faction displays and/or sells */
	int32 GetLargestShopCatalogue() const;
	
	/* Return the largest number of slots a selectable on this faction has in their inventory */
	int32 GetLargestInventory() const;
};
