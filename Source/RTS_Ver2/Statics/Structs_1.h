// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "Settings/ProjectSettings.h" // Added for TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
#include "Statics/Structs_4.h"
#include "Statics/CoreDefinitions.h"
#include "Statics/Statics.h" // Added for NUM_BUILDING_GARRISON_NETWORK_TYPES
#include "Structs_1.generated.h"

struct FBuildInfo;
struct FContextCommand;
struct FTrainingInfo;
class AAbilityBase;
class ABuilding;
class ARTSPlayerState;
class AFactionInfo;
class ARTSPlayerController;
class UUpgradeEffect;
class AGhostBuilding;
struct FContextButton;
class ACPUPlayerAIController;
struct FMacroCommandReason;
struct FStaticBuffOrDebuffInfo;
struct FTickableBuffOrDebuffInfo;
class ISelectable;
class ARTSGameState;
class URTSDamageType;
struct FAttackAttributes;
class URTSHUD;
class USelectableWidgetComponent;
struct FItemOnDisplayInShopSlot;
class URTSGameInstance;
class InventoryItemBehavior;
class USkeletalMesh;
class UAnimMontage;
struct FInventory;
struct FInventorySlotState;
struct FSlateBrush;
class AInventoryItem;
enum class ECommanderSkillTreeNodeType : uint8;
class UCommanderAbilityBase;
class UBuildingTargetingAbilityBase;
class AInfantry;
struct FSelectableRootComponent2DShapeInfo;
struct FBuildingGarrisonAttributes;
struct FBuildingSideInfo;


/**--------------------------------------------------------------------------------------------
 *	A file holding a lot of structs	
 ----------------------------------------------------------------------------------------------*/


/* Int16 that must be at least 1. Mainly here because at times I want to impose a min limit 
for container entries */
USTRUCT()
struct FAtLeastOneInt16
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1, ShowOnlyInnerProperties))
	int16 Integer;

public:

	FAtLeastOneInt16() : Integer(1) { }

	int16 GetInteger() const { return Integer; }

	FAtLeastOneInt16 & operator-=(const int16 & Number)
	{
		Integer -= Number;
		return *this;
	}

	FAtLeastOneInt16 & operator+=(const int16 & Number)
	{
		Integer += Number;
		return *this;
	}
};


/* EFaction and EBuildingType */
USTRUCT()
struct FFactionBuildingTypePair
{
	GENERATED_BODY()

public:

	FFactionBuildingTypePair();
	FFactionBuildingTypePair(EFaction InBuildingsFaction, EBuildingType InBuildingsType)
		: Faction(InBuildingsFaction)
		, BuildingType(InBuildingsType)
	{
	}

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EFaction Faction;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingType BuildingType;

public:

	friend uint32 GetTypeHash(const FFactionBuildingTypePair & Elem)
	{
		return GetTypeHash(Elem.Faction) + GetTypeHash(Elem.BuildingType);
	}

	friend bool operator==(const FFactionBuildingTypePair & Elem_1, const FFactionBuildingTypePair & Elem_2)
	{
		return Elem_1.Faction == Elem_2.Faction && Elem_1.BuildingType == Elem_2.BuildingType;
	}
};


/* Holds what brush/sound assets the player uses */
USTRUCT()
struct FUnifiedImageAndSoundFlags
{
	GENERATED_BODY()

	/* Bools for whether or not we're using a single image/sound or not */
	UPROPERTY()
	uint8 bUsingUnifiedHoverImage : 1;
	UPROPERTY()
	uint8 bUsingUnifiedPressedImage : 1;
	UPROPERTY()
	uint8 bUsingUnifiedHoverSound : 1;
	UPROPERTY()
	uint8 bUsingUnifiedLMBPressSound : 1;
	UPROPERTY()
	uint8 bUsingUnifiedRMBPressSound : 1;
};


/* Static mesh and a transform */
USTRUCT()
struct FStaticMeshInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UStaticMesh * Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTransform Transform;

public:

	FStaticMeshInfo()
		: Mesh(nullptr)
	{
	}

	UStaticMesh * GetMesh() const { return Mesh; }
	const FTransform & GetTransform() const { return Transform; }
};


/* This struct allows the user to set either a static mesh or skeletal mesh */
USTRUCT()
struct FMeshInfoBasic
{
	GENERATED_BODY()

protected:

	/**
	 *	Can either be a static mesh or skeletal mesh.
	 *
	 *	My notes:
	 *	Odd. I cannot set SKs in this project, but in another project I can do fine.
	 *	I get this message on loading up editor:
	 *	"Property MeshInfo of MeshInfo has a struct type mismatch (tag HOTRELOADED_MeshInfo_0 != prop MeshInfo)
	 *	in package:  ../../../../Projects/RTS_Ver2/Content/Framework/RTSGameInstance_BP.uasset. If that struct got
	 *	renamed, add an entry to ActiveStructRedirects" - might have something to do with it.
	 *	Update: I no longer get this warning message and it still doesn't work.
	 *
	 *	Named it MeshPtr to try and get around this warning.
	 *	Tried moving around in struct to get it fixed. This sucks.
	 *
	 *	Can't get this working TODO. Once fixed can remove WORKAROUND_MeshPath variable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (AllowedClasses = "StaticMesh, SkeletalMesh"))
	UObject * Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector Location;
	
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FRotator Rotation;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector Scale;

#if WITH_EDITORONLY_DATA
	/** 
	 *	This is a workaround for Mesh not being able to set skeletal meshes (or anything other 
	 *	than static meshes actually). Copy and paste an object path into here to set Mesh to it.
	 *	(get path by right-clicking on asset and choosing 'Copy Reference').
	 *
	 *	The issue probably lies in the fact I used to have a struct 
	 *	called FMeshInfo that only allowed static meshes then I changed it to allow both. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FString WORKAROUND_MeshPath;
#endif

public:

	FMeshInfoBasic();

	/* Return whether Mesh is not null */
	bool IsMeshSet() const;

	/* It's gonna be either a SM or SK. Make sure to null check before calling these */
	bool IsStaticMesh() const;
	bool IsSkeletalMesh() const;

	UStaticMesh * GetMeshAsStaticMesh() const;
	USkeletalMesh * GetMeshAsSkeletalMesh() const;

	FVector GetLocation() const;
	FRotator GetRotation() const;
	FVector GetScale3D() const;

#if WITH_EDITOR
	void OnPostEdit();
#endif
};


 /* Struct for storing data about hardware cursors. Similar to the engine's FHardwareCursor,
 follows similar naming conventions */
USTRUCT()
struct FCursorInfo
{
	GENERATED_BODY()

protected:

	/* File path relative to UStatics::HARDWARE_CURSOR_PATH which I think is Slate/HardwareCursors/
	but you might want to double check that in Statics.cpp. Leave this varaible blank to indicate
	you do not want to use a custom mouse cursor in this case ("None" may show) */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName CursorPath;

	/* Hot spot refers to the part of the cursor image that determines where the click happens.
	This should be normalized to between 0 and 1, so I assume that (0, 0) would be the top left
	of the cursor image while (1, 1) is the bottom right */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector2D HotSpot;

	/* Full path =  UStatics::HARDWARE_CURSOR_PATH + Path. Set during game startup to avoid
	creating FNames while setting hardware cursor in match */
	UPROPERTY()
	FName FullPath;

#if WITH_EDITORONLY_DATA
	/* Whether SetFullPath() has been called */
	UPROPERTY()
	bool bHasSetFullPath;
#endif

public:

	FCursorInfo();

	/* This constructor was added so development widget actions could set custom cursors. */
	FCursorInfo(const FString & InFullPath, FVector2D InHotSpot);

	/* Getters and setters */

	const FName & GetCursorPath() const { return CursorPath; }
	FVector2D GetHotSpot() const { return HotSpot; }
	const FName & GetFullPath() const { return FullPath; }
	
	/* Create the full path to the cursor. This is called during setup to avoid creating FNames 
	on the fly */
	void SetFullPath();

	/* Return whether user decided to use link this to a custom cursor or not */
	bool ContainsCustomCursor() const;
};


//=============================================================================================
//	Next 3 structs are for player commands 
//=============================================================================================


/* Struct to be sent from client to server when a command is issued that doesn't
require a location in world space e.g. a train unit command from a barracks */
USTRUCT()
struct FContextCommand
{
	GENERATED_BODY()

	/* If true then the same group as the last order should be used */
	UPROPERTY()
	uint8 bSameGroup : 1;

	/**
	 *	Just static_cast<uint8>(EContextAction). Don't really need to convert to uint8
	 */
	UPROPERTY()
	uint8 Type;

	UPROPERTY()
	uint8 SubType;

	UPROPERTY()
	TArray <uint8> AffectedSelectables;

	FContextCommand(bool bUseSameGroup, const FContextButton & Button);

	FContextCommand(bool bUseSameGroup, EContextButton AbilityType);

	FContextCommand();

	void AddSelectable(uint8 SelectableID);

private:

	void SetupTypes(EContextButton AbilityType);
	void SetupTypes(const FContextButton & Button);
};


/* A context command that requires a location. */
USTRUCT()
struct FContextCommandWithLocation : public FContextCommand
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	FVector_NetQuantize10 ClickLocation;

public:

	FContextCommandWithLocation();
	FContextCommandWithLocation(EContextButton AbilityType, bool bUseSameGroup, const FVector & ClickLoc);

	FVector_NetQuantize10 GetClickLocation() const { return ClickLocation; }
};


/* A context command that requires a target to be sent over the wire */
USTRUCT()
struct FContextCommandWithTarget : public FContextCommand
{
	GENERATED_BODY()

	/* Keeping this as regular FVector. Does this lose precision by default?
	I want this to be precise since if the player is placing a building and
	their machine says it is ok to place there then a I don't want the server
	saying 'actually you can't place that there' because of precision loss */
	UPROPERTY()
	FVector_NetQuantize10 ClickLocation;

	UPROPERTY()
	AActor * Target;

	FContextCommandWithTarget(EContextButton AbilityType, bool bUseSameGroup, const FVector & ClickLoc, 
		AActor * ClickTarget);

	/* Don't think this is ever called */
	FContextCommandWithTarget();
};


/* Base struct for all right click commands */
USTRUCT() 
struct FRightClickCommandBase
{
	GENERATED_BODY()

protected:

	/* If true then the same group as the last order should be used */
	UPROPERTY()
	uint8 bSameGroup : 1;

	/* Selectable IDs for all selectables given the command */
	UPROPERTY()
	TArray <uint8> AffectedSelectables;

	/* World space coords of the click location */
	UPROPERTY()
	FVector_NetQuantize ClickLocation;

public:

	FRightClickCommandBase();

	FRightClickCommandBase(const FVector & ClickLoc);

	// Call AffectedSelectables.Reserve(Num)
	void ReserveAffectedSelectables(int32 Num);

	// Add selectables as those affected by command
	void AddSelectable(uint8 SelectableID);

	void SetUseSameGroup(bool bValue);

	//-----------------------------------------------------------------------
	//	Simple getters
	//-----------------------------------------------------------------------

	// Return whether the PC should use the last set of selectable IDs when issuing command
	bool UseSameGroup() const;

	// Array of selectable IDs of selectables to be issued with command
	const TArray < uint8 > & GetAffectedSelectables() const;

	// Get the click location of command
	const FVector & GetClickLocation() const;
};


/* Struct to send over wire when issuing a right-click command */
USTRUCT()
struct FRightClickCommandWithTarget : public FRightClickCommandBase
{
	/* TODO: add NetSerialize overload for this */

	GENERATED_BODY()

protected:

	/* Target if there was one clicked on. This gets rid of some of the efficiency
	of using this struct. TODO: make seperate structs: one for no targets and one for
	targets */
	UPROPERTY()
	AActor * Target;

public:

	FRightClickCommandWithTarget(const FVector & ClickLoc, AActor * InTarget);

	FRightClickCommandWithTarget();

	// Get the selectable that was clicked on, or null if none
	AActor * GetTarget() const;
};


/* Right click command on an inventory item */
USTRUCT()
struct FRightClickCommandOnInventoryItem : public FRightClickCommandBase
{
	GENERATED_BODY()

protected:

	// Unique ID of the item that was clicked on
	UPROPERTY()
	FInventoryItemID ItemID;

public:

	FRightClickCommandOnInventoryItem();

	FRightClickCommandOnInventoryItem(const FVector & ClickLoc, AInventoryItem * InClickedItem);

	AInventoryItem * GetClickedInventoryItem(ARTSGameState * GameState) const;

#if WITH_EDITOR
	// Mainly here for debugging. Should never need to be called normally
	FInventoryItemID GetClickedInventoryItemID() const { return ItemID; }
#endif
};


 /* Context buttons for each unit */
USTRUCT()
struct FContextButton
{
	GENERATED_BODY()

protected:

	/* What the button does */
	UPROPERTY(EditDefaultsOnly)
	EContextButton ButtonType;

	/* If this button is to build a building, the building it will build */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCanEditBuildingType))
	EBuildingType BuildingType;

	/* If the button trains a unit what unit it trains. Must be
	set to NotUnit if ButtonType != Train */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bShowUnitType))
	EUnitType UnitType;

	/* Upgrade type if this is for upgrades */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bShowUpgradeType))
	EUpgradeType UpgradeType;

	/* Relevant only when using a persistent panel in HUD like in C&C. The order to appear on
	tab. Lower = appear sooner */
	UPROPERTY()
	int32 HUDPersistentTabButtonOrdering;

	/* Build info this button is related to. Probably never use this. Should delete it */
	const FBuildInfo * BuildInfo;

public:

	FContextButton(EBuildingType InBuildingType);
	FContextButton(EUnitType InUnitType);
	FContextButton(EUpgradeType InUpgradeType);
	explicit FContextButton(EContextButton InButtonType);
	explicit FContextButton(const FContextCommand & Command);
	explicit FContextButton(const FBuildInfo * InBuildInfo);
	explicit FContextButton(const FTrainingInfo & InTrainingInfo);
	FContextButton();

#if !UE_BUILD_SHIPPING
	FString ToString() const;
#endif

#if WITH_EDITORONLY_DATA

protected:
	/* Booleans and function to toggle visibility of variables in editor */

	UPROPERTY()
	bool bCanEditBuildingType;

	UPROPERTY()
	bool bShowUnitType;

	UPROPERTY()
	bool bShowUpgradeType;

public:

	/* Update visibility of fields and their values */
	void UpdateFieldVisibilitys()
	{
		if (ButtonType == EContextButton::Train)
		{
			bShowUnitType = true;
			bShowUpgradeType = false;
			bCanEditBuildingType = false;
			UpgradeType = EUpgradeType::None;
			BuildingType = EBuildingType::NotBuilding;
		}
		else if (ButtonType == EContextButton::Upgrade)
		{
			bShowUnitType = false;
			bShowUpgradeType = true;
			bCanEditBuildingType = false;
			UnitType = EUnitType::NotUnit;
			BuildingType = EBuildingType::NotBuilding;
		}
		else if (ButtonType == EContextButton::BuildBuilding)
		{
			bShowUnitType = false;
			bShowUpgradeType = false;
			bCanEditBuildingType = true;
			UnitType = EUnitType::NotUnit;
			UpgradeType = EUpgradeType::None;
		}
		else
		{
			bShowUnitType = bShowUpgradeType = bCanEditBuildingType = false;
			UnitType = EUnitType::NotUnit;
			UpgradeType = EUpgradeType::None;
			BuildingType = EBuildingType::NotBuilding;
		}
	}
#endif // WITH_EDITORONLY_DATA

	/* For hash function */
	friend bool operator==(const FContextButton & Button1, const FContextButton & Button2)
	{
		return Button1.ButtonType == Button2.ButtonType && Button1.BuildingType == Button2.BuildingType
			&& Button1.UnitType == Button2.UnitType && Button1.UpgradeType == Button2.UpgradeType;
	}

	/* Hash function for sets and maps */
	friend uint32 GetTypeHash(const FContextButton & Button)
	{
		const uint32 Hash = static_cast<uint8>(Button.ButtonType) + static_cast<uint8>(Button.BuildingType)
			+ static_cast<uint8>(Button.UnitType) + static_cast<uint8>(Button.UpgradeType);

		return Hash;
	}

	/* For sorting in FButtonArray */
	friend bool operator<(const FContextButton & Button1, const FContextButton & Button2)
	{
		return Button1.HUDPersistentTabButtonOrdering < Button2.HUDPersistentTabButtonOrdering;
	}

	EContextButton GetButtonType() const { return ButtonType; }
	EBuildingType GetBuildingType() const { return BuildingType; }
	EUnitType GetUnitType() const { return UnitType; }
	EUpgradeType GetUpgradeType() const { return UpgradeType; }
	const FBuildInfo * GetBuildInfo() const { return BuildInfo; }

	int32 GetHUDPersistentTabButtonOrdering() const { return HUDPersistentTabButtonOrdering; }
	void SetHUDPersistentTabButtonOrdering(int32 InValue) { HUDPersistentTabButtonOrdering = InValue; }
	void SetBuildInfo(const FBuildInfo * InInfo) { BuildInfo = InInfo; }

	bool IsForBuildBuilding() const { return ButtonType == EContextButton::BuildBuilding; }
	bool IsForTrainUnit() const { return ButtonType == EContextButton::Train; }
	bool IsForResearchUpgrade() const { return ButtonType == EContextButton::Upgrade; }
};


/* Used in production queues */
USTRUCT()
struct FTrainingInfo
{
	GENERATED_BODY()

protected:

	/* The building this is for or NotBuilding if not for building  */
	UPROPERTY()
	EBuildingType BuildingType;

	/* The unit training or NotUnit if this is for an upgrade */
	UPROPERTY()
	EUnitType UnitType;

	/* Upgrade type, or None if this is for training a unit */
	UPROPERTY()
	EUpgradeType UpgradeType;

public:

	bool IsProductionForBuilding() const;

	bool IsForUnit() const;

	/* Returns true if this is for an upgrade, false if this is for a unit or building */
	bool IsForUpgrade() const;

	FTrainingInfo(const FContextButton & InButton);
	FTrainingInfo(EBuildingType InBuildingType);
	FTrainingInfo(EUnitType InUnitType);
	FTrainingInfo(EUpgradeType InUpgradeType);
	FTrainingInfo();

	/* For TMap */
	friend bool operator==(const FTrainingInfo & Info1, const FTrainingInfo & Info2);
	friend uint32 GetTypeHash(const FTrainingInfo & Info);


#if !UE_BUILD_SHIPPING
	// For debugging 
	FString ToString() const;
#endif

	/* Getters and setters */

	EBuildingType GetBuildingType() const;
	EUnitType GetUnitType() const;
	EUpgradeType GetUpgradeType() const;
};


/* Training info that also holds the reason the item is being produced */
USTRUCT()
struct FCPUPlayerTrainingInfo : public FTrainingInfo
{
	GENERATED_BODY()

protected:

	/* Reason for producing this */
	EMacroCommandSecondaryType ActualCommandType;

	/* More info about the reason for production e.g. if it is for building a resource depot 
	then the resource type it is for. Sometimes this can be ignored. */
	uint8 AuxilleryInfo;

	EMacroCommandType OriginalCommand_ReasonForProduction;
	uint8 OriginalCommand_AuxilleryInfo;

public:

	// Default ctor, never call this 
	FCPUPlayerTrainingInfo();

	/* Ctors that are all basically the same */
	FCPUPlayerTrainingInfo(EBuildingType InBuildingType, const FMacroCommandReason & CommandReason);
	FCPUPlayerTrainingInfo(EUnitType InUnitType, const FMacroCommandReason & CommandReason);
	FCPUPlayerTrainingInfo(EUpgradeType InUpgradeType, const FMacroCommandReason & CommandReason);

	EMacroCommandSecondaryType GetActualCommand() const;
	
	uint8 GetRawAuxilleryInfo() const;

	EMacroCommandType GetOriginalCommandReasonForProduction() const;
	uint8 GetOriginalCommandAuxilleryInfo() const;
};


USTRUCT()
struct FBasicDecalInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UMaterialInterface * Decal; 

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float Radius;

public:

	FBasicDecalInfo();
	FBasicDecalInfo(UMaterialInterface * InDecal, float InRadius);

	UMaterialInterface * GetDecal() const;
	float GetRadius() const;
};


/*  Defines basic command params for a context action. Some of the things defined
in this struct can be ignored by individual units e.g. MaxRange, cooldown and
Effect_BP can be ignored by units */
USTRUCT()
struct FContextButtonInfo
{
	GENERATED_BODY()

protected:

	/* Name of action to appear on HUD. Only applicable if a non BuildBuilding/Train/Upgrade action */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/* Quick note: it *should* be possible to use my own custom struct here instead of FSlateBrush 
	if FSlateBrush has too many options. In SMyButton::OnPaint the FSlateDrawElement::MakeBox 
	can be replaced with a func that does simular but takes my custom struct as a param instead 
	of the FSlateBrush */

	/* Image to appear in HUD. Only applicable if a non BuildBuilding/Train/Upgrade action */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush Image;

	/** 
	 *	The image for when the mouse is over a button for this ability. This will be ignored 
	 *	if you use a unified image for the HUD element this appears on.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HoveredImage;

	/** 
	 *	The image for when the mouse is pressed on a button for this ability. This will be ignored 
	 *	if you use a unified image for the HUD element this appears on.  
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush PressedImage;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HighlightedImage;

	/* The sound to play when the mouse is hovered over a button on a action bar for this ability. 
	This will be ignored if you use a unified sound set in game instance BP */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * HoveredSound;

	/* The sound to play the moment the left mouse button is pressed on an action bar button for 
	this ability. This will be ignored if you use a unified sound set in game instance BP */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * PressedByLMBSound;

	/* The sound to play at the moment the right mouse button is pressed on an action bar button 
	for this ability. This will be ignored if you use a unified sound set in game instance BP */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * PressedByRMBSound;

	/* A description of the ability. Will likely be ignored if this button is for 
	BuildBuilding/Train/Upgrade */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Description;

	/* Type of button this is */
	UPROPERTY()
	EContextButton ButtonType;

	/**
	 *	Does this action happen instantly or is another mouse click required?
	 *
	 *	e.g. true = Zerg burrow command in Starcraft II
	 *	false = High Templar Psionic Storm in SCII. This requires another click to select the location
	 *	before the command is issued. False basically means the action requires targeting
	 *	someone/somewhere
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bIsIssuedInstantly : 1;

	/**
	 *	If true this command will be issued to every unit selected (if they support the action).
	 *	If false then only a single unit will be issued with the command and it will be the unit whose
	 *	context menu is showing.
	 *	e.g. True = Zerg burrow - all selected units will do it
	 *	False = High Templar Psionic Storms - no matter how many High Templar are selected only one
	 *	will do it
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bIsIssuedToAllSelected : 1;

	/** 
	 *	If true then a command for this ability can only be issued if the unit is in range of 
	 *	the target selectable/location. If false then the unit can always be issued the command 
	 *	and if they are not in range at the time of receiving the command then they will move 
	 *	in range and then use it. 
	 *	Strict is a bad name. Something better might be bMustUnitBeInRangeAtTimeOfIssue
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditNonInstantPorperties))
	uint8 bIsRangeRequirementStrict : 1;

	/* If this is true then a unit receiving this
	command will stop movement when they receive the command and after it has executed they will
	go idle where they are. If false unit will not stop moving while they execute the command
	(if bUseAnimation is true then they will play anim as they keep moving) and will continue
	carrying out previous command. If you want your unit to use this ability while they move then
	set this to false. 
	
	Not 100% this has been implemented fully. For now it is not possible to have this as false 
	and use an animation */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bCommandStopsMovement : 1; // TODO actually implement this in AI controller. Important that when anim finishes the walk anim resumes or someting. Update comment here

	/**
	 *	Whether to use an animation for this ability. If false then no animation will be played
	 *	and ability will take effect instantly. 
	 *
	 *	Currently if true then bCommandStopsMovement will be automatically flagged as true
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bUseAnimation : 1;

	/** 
	 *	Whether to use a preparation animation. Currently this is only used by buildings.
	 *	
	 *	Make sure to adjust the variable PreparationAnimPlayPoint to suit when this anim should play 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUseAnimation))
	uint8 bUsePreparationAnimation : 1;

	/**
	 *	The animation type to use for this ability. You should not use animation types that are
	 *	common such as attack or move animations since these will have anim notifies that will cause
	 *	crashes if used here. It is best to stick to animation types you have defined yourself or the
	 *	ContextAction_1/2/3 types
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUseAnimation))
	EAnimation AnimationType;

	/** 
	 *	This animation plays at a certain point in the ability cooling down. 
	 *	
	 *	e.g. in C&C generals: at approx 10 secs before your nuke becomes ready an animation plays 
	 *	that opens the silo and points the nuke upwards. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUsePreparationAnimation))
	EBuildingAnimation PreparationAnimationType_Building;

	/** Animation to use if a building is using the ability */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUseAnimation))
	EBuildingAnimation AnimationType_Building;

	/**
	 *	If true then once the animation for this ability starts you will not be able
	 *	to stop the animation with new commands until anim notify OnContextAnimationFinished is
	 *	executed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUseAnimation))
	bool bAnimationBlocksCommands; // TODO could add as ctor param

	/**	
	 *	Whether to use a custom mouse cursor or a decal when choosing where to use ability. If using
	 *	CustomMouseCursor it is assumed at least DefaultMouseCursor has a valid path to a cursor.
	 *	If using HideAndShowDecal it is assumed Decal is set 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditNonInstantPorperties))
	EAbilityMouseAppearance MouseAppearance;

	/**
	 *	True = ability requires another selectable as a target
	 *	False = any location in the world is good enough.
	 *	E.g. True = Starcraft II Ghosts snipe ability
	 *	False = Starcraft II Ghosts EMP ability
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditNonInstantPorperties))
	uint8 bRequiresSelectableTarget : 1;

	/** 
	 *	This is only relevant if the ability does not require a selectable as a target but requires 
	 *	a location in the world as a target. 
	 *	Whether we are allowed to use this ability on locations that are inside fog of war 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditLocationTargetingProperties))
	uint8 bCanTargetInsideFog : 1;

	/* Whether ability can use an enemy as a target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	uint8 bCanTargetEnemies : 1;

	/**
	 *	Whether ability can use a selectable on your team as a target. Note if this and
	 *	bCanTargetEnemies is false then your ability will not be able to target anything
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	uint8 bCanTargetFriendlies : 1;

	/**
	 *	Whether can target self. bCanTargetFriendlies must be true for this to be relevant. If your
	 *	ability wants to only target self then you can set bIsIssuedInstantly to false
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditTargetSelfProperties))
	uint8 bCanTargetSelf : 1;

	/** 
	 *	Whether to check if in range at time anim notify to use ability happens. Only relevant 
	 *	for abilities that target another selectable. 
	 *	
	 *	True = right before ability is done a range check happens
	 *	False = once anim starts ability will be considered always in range no matter how far away 
	 *	the target moves
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditCheckRangeAtAnimNotify))
	uint8 bCheckRangeAtAnimNotify : 1;

	/**
	 *	The types of selectables this ability can target. E.g. a High Templars feedback can
	 *	only target energy users. This is only for targeting purposes. Once the ability has been
	 *	used it is possible its effect causes an AoE effect which hits types that are not this type.
	 *	That is all up to how the ability has been implemented.
	 *	You must add at least one type for your ability to be usable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	TSet <ETargetingType> AcceptableTargets;

	/* AcceptableTargets as FNames */
	UPROPERTY()
	TSet <FName> AcceptableTargetFNames;

	/* Default mouse cursor for ability to show when not hovering over a selectable */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType DefaultMouseCursor;

	/**
	 *	Mouse cursor to appear when hovering over a selectable that is an acceptable target for
	 *	this ability
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType AcceptableTargetMouseCursor;

	/**
	 *	Mouse cursor to appear when hovering over a selectable that is not an acceptable target
	 *	for this ability
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType UnacceptableTargetMouseCursor;

	FCursorInfo DefaultMouseCursor_Info;
	FCursorInfo AcceptableTargetMouseCursor_Info;
	FCursorInfo UnacceptableTargetMouseCursor_Info;

	/**
	 *	If bIsIssuedInstantly is false, the decal to draw under the mouse for when the 
	 *	location under the mouse is at a location where a command can be issued.
	 *
	 *	Note about Radius: this is for visuals only. To change
	 *	the actual area of effect of a action would need to be done in
	 *	each implementation of the action ability (can also be done on a
	 *	per-unit basis although this is not exposed to editor).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditDecalProperties))
	FBasicDecalInfo AcceptableLocationDecalInfo;

	/** 
	 *	If bIsIssuedInstantly is false, the decal to draw under the mouse when it is at a 
	 *	position where a command cannot be issued because perhaps it is inside fog, out of 
	 *	range etc. 
	 *
	 *	This decal is optional. If not set then the AcceptableLocationDecal will always be 
	 *	shown.
	 *
	 *	UE4 tip: If all you want to do is change the color of this material then it is a good 
	 *	idea to add a parameter to your AcceptableLocationDecal asset, make a material instance 
	 *	from it in editor and adjust that param value.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditDecalProperties))
	FBasicDecalInfo UnusableLocationDecalInfo;

	/** 
	 *	Cooldown 
	 *	
	 *	0 = no cooldown. Could remove this and have each unit set its own cooldown 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float Cooldown;

	/**
	 *	The cooldown this ability starts with. 
	 *	
	 *	e.g. in C&C generals when you build a nuke silo you have to wait 6 minutes before you can 
	 *	use it.
	 *
	 *	0 = starts off cooldown
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float InitialCooldown;

	/** 
	 *	How much time remaining for ability coming off cooldown when we should play the preparation 
	 *	animation. Makes sense for this to be no larger than Cooldown.
	 *
	 *	e.g. if this is 10 then when the ability has 10 secs left before coming off cooldown then 
	 *	the preparation anim will play. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bUsePreparationAnimation))
	float PreparationAnimPlayPoint;

	/** 
	 *	The selectable resource cost of this ability. 
	 *	
	 *	Selectable resources are things like mana or energy. Currently each selectable is only 
	 *	allowed a maximum of 1 selectable resource so you cannot have costs like "50 mana and 
	 *	25 energy". 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	int32 SelectableResourceCost_1;

	/* Max range of action. 0 = unlimited range */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditNonInstantPorperties))
	float MaxRange;

	/* Blueprint for the effect of this button. This is pretty important */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Effect"))
	TSubclassOf <AAbilityBase> Effect_BP;

public:

	FContextButtonInfo(const FText & InName, EContextButton ButtonType, bool bIsIssuedInstantly,
		bool bIsIssuedToAllSelected, bool bInIsRangeRequirementStrict, bool bInCommandStopsMovement, 
		bool bInUseAnimation, EAnimation InAnimationType, EAbilityMouseAppearance InMouseAppearance,
		bool bInRequiresSelectableTarget, bool bInCanTargetEnemies, bool bInCanTargetFriendlies,
		bool bInCanTargetSelf, float Cooldown, float MaxRange, UMaterialInterface  * AcceptableLocDecal,
		float AcceptableDecalRadius, TSubclassOf <AAbilityBase> Effect_BP);
	explicit FContextButtonInfo(EContextButton InType);
	FContextButtonInfo();

	/* Set everything but ButtonType to default value */
	void MostlyDefaultValues();

	/* This should only be called during GI initialization */
	void SetInitialType(EContextButton InType);

	/* Add a FName to the AcceptableTargetFNames container. Will probably be called during 
	setup */
	void AddAcceptableTargetFName(const FName & InName);

	/* Create the full path names for each hardware cursor */
	void SetupHardwareCursors(URTSGameInstance * GameInst);

	/* For sorting */
	friend bool operator<(const FContextButtonInfo & Info1, const FContextButtonInfo & Info2);

	/* Getters and setters */
	EContextButton GetButtonType() const { return ButtonType; }
	const FText & GetName() const;
	const FSlateBrush & GetImage() const { return Image; }
	const FSlateBrush & GetHoveredImage() const { return HoveredImage; }
	const FSlateBrush & GetPressedImage() const { return PressedImage; }
	const FSlateBrush & GetHighlightedImage() const { return HighlightedImage; }
	USoundBase * GetHoveredSound() const { return HoveredSound; }
	USoundBase * GetPressedByLMBSound() const { return PressedByLMBSound; }
	USoundBase * GetPressedByRMBSound() const { return PressedByRMBSound; }
	const FText & GetDescription() const { return Description; }
	bool IsInstant() const { return bIsIssuedInstantly; }
	bool IsIssuedToAllSelected() const { return bIsIssuedToAllSelected; }
	/* Could get rid of the MaxRange > 0.f check by flagging bIsRangeRequirementStrict to true 
	on any post edits when MaxRange is 0 or less */
	bool ShouldCheckRangeAtCommandIssueTime() const { return bIsRangeRequirementStrict && MaxRange > 0.f; }
	/* These two funcs make ShouldCheckRangeAtCommandIssueTime obsolete. Remove it */
	bool PassesCommandTimeRangeCheck(ISelectable * AbilityUser, const FVector & TargetLocation) const;
	bool PassesCommandTimeRangeCheck(ISelectable * AbilityUser, ISelectable * Target) const;
	/* Only relevant if the ability uses an animation and targets another selectable 
	At time anim notify to use ability happens is the target considered visible? */
	bool PassesAnimNotifyTargetVisibilityCheck(AActor * AbilityTarget) const;
	
	/** 
	 *	Check if in range at time of anim notify. Only relevant if ability uses an anim and 
	 *	targets another selectable.
	 *	
	 *	@param Instigator - selectable instigating the ability use.
	 *	@param AIController - Instigator's behavior AI controller. Nothing to do with CPU players.
	 *	@param Target - ability target
	 */
	template <typename TSelectable, typename TSelectableAIController>
	bool PassesAnimNotifyRangeCheck(TSelectable * Instigator, TSelectableAIController * AIController,
		ISelectable * Target) const
	{
		// Need to add this variable
		if (bCheckRangeAtAnimNotify)
		{
			/* May want to take into account some kind of lenience here */
			
			const float DistanceSqr = FMath::Square(AIController->GetDistanceFromAnotherSelectableForAbility(*this, Target));
			const float RequiredDistanceSqr = Statics::GetAbilityRangeSquared(*this);

			return (DistanceSqr <= RequiredDistanceSqr);
		}
		else
		{
			return true;
		}
	}

	bool DoesCommandStopMovement() const { return bCommandStopsMovement; }
	bool UsesAnimation() const { return bUseAnimation; }
	bool UsesPreparationAnimation() const { return bUsePreparationAnimation; }
	EAnimation GetAnimationType() const { return AnimationType; }
	EBuildingAnimation GetBuildingPreparationAnimationType() const { return PreparationAnimationType_Building; }
	EBuildingAnimation GetBuildingAnimationType() const { return AnimationType_Building; }
	bool DoesAnimationBlockCommands() const { return bAnimationBlocksCommands; }
	EAbilityMouseAppearance GetMouseAppearanceOption() const { return MouseAppearance; }
	bool RequiresSelectableTarget() const { return bRequiresSelectableTarget; }
	bool CanLocationBeInsideFog() const { return bCanTargetInsideFog; }
	bool CanTargetEnemies() const { return bCanTargetEnemies; }
	bool CanTargetFriendlies() const { return bCanTargetFriendlies; }
	bool CanTargetSelf() const { return bCanTargetSelf; }
	const TSet <ETargetingType> & GetAcceptableTargets() const { return AcceptableTargets; }
	const TSet <FName> & GetAcceptableTargetFNames() const { return AcceptableTargetFNames; }
	const FCursorInfo & GetDefaultCursorInfo() const { return DefaultMouseCursor_Info; }
	const FCursorInfo & GetAcceptableTargetCursorInfo() const { return AcceptableTargetMouseCursor_Info; }
	const FCursorInfo & GetUnacceptableTargetCursorInfo() const { return UnacceptableTargetMouseCursor_Info; }
	const FBasicDecalInfo & GetAcceptableLocationDecal() const { return AcceptableLocationDecalInfo; }
	const FBasicDecalInfo & GetUnusableLocationDecal() const { return UnusableLocationDecalInfo; }
	float GetCooldown() const { return Cooldown; }
	float GetInitialCooldown() const { return InitialCooldown; }
	float GetPreparationAnimPlayPoint() const { return PreparationAnimPlayPoint; }
	int32 GetSelectableResourceCost_1() const { return SelectableResourceCost_1; }
	float GetMaxRange() const { return MaxRange; }
	TSubclassOf < AAbilityBase > GetEffectBP() const { return Effect_BP; }

	/* Get a pointer to the spawned effect info actor for this ability */
	AAbilityBase * GetEffectActor(ARTSGameState * GameState) const;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditNonInstantPorperties;
	
	UPROPERTY()
	bool bCanEditPreparationAnimProperties;

	UPROPERTY()
	bool bCanEditSingleTargetProperties;

	UPROPERTY()
	bool bCanEditTargetSelfProperties;

	UPROPERTY()
	bool bCanEditMouseCursorProperties;

	UPROPERTY()
	bool bCanEditCheckRangeAtAnimNotify;

	UPROPERTY()
	bool bCanEditLocationTargetingProperties;

	UPROPERTY()
	bool bCanEditDecalProperties;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	void OnPostEdit(EContextButton InTMapKey)
	{
		/* For now if using animation then movement must stop. Can remove this when anims + not
		stopping movement is supported */
		if (bUseAnimation)
		{
			bCommandStopsMovement = true;
		}
		
		ButtonType = InTMapKey;

		bCanEditNonInstantPorperties = !bIsIssuedInstantly;

		bCanEditSingleTargetProperties = bCanEditNonInstantPorperties && bRequiresSelectableTarget;

		bCanEditTargetSelfProperties = bCanEditSingleTargetProperties && bCanTargetFriendlies;

		bCanEditMouseCursorProperties = bCanEditNonInstantPorperties
			&& MouseAppearance == EAbilityMouseAppearance::CustomMouseCursor;

		bCanEditCheckRangeAtAnimNotify = bCanEditNonInstantPorperties
			&& bCanEditSingleTargetProperties && MaxRange > 0.f;

		bCanEditLocationTargetingProperties = bCanEditNonInstantPorperties
			&& !bRequiresSelectableTarget;

		bCanEditDecalProperties = bCanEditNonInstantPorperties
			&& MouseAppearance == EAbilityMouseAppearance::HideAndShowDecal;
	}
#endif // WITH_EDITOR 
};


/* Similar to FContextButtonInfo but for abilities instigated by the commander (commander = player) */
USTRUCT()
struct FCommanderAbilityInfo
{
	GENERATED_BODY()

public:

	// Make sure at some point Type is set to the key of the TMap this info resides in
	FCommanderAbilityInfo();

	void SetType(ECommanderAbility InAbilityType);
	void PopulateAcceptableTargetFNames(); // This func can just return if the ability cannot target selectables 
	void SetupHardwareCursors(URTSGameInstance * GameInst);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditWorldLocationTargetingProperties;

	UPROPERTY()
	bool bCanEditSingleTargetProperties;

	UPROPERTY()
	bool bCanEditSelectableTargetingProperties;

	UPROPERTY()
	bool bCanEditMouseCursorProperties;

	UPROPERTY()
	bool bCanEditDecalProperties;
#endif

#if WITH_EDITOR
	void OnPostEdit();
#endif

	//----------------------------------------------------------
	//	Getters
	//----------------------------------------------------------

	const FText & GetName() const { return Name; }
	const FSlateBrush & GetNormalImage() const { return NormalImage; }
	const FSlateBrush & GetHoveredImage() const { return HoveredImage; }
	const FSlateBrush & GetPressedImage() const { return PressedImage; }
	const FSlateBrush & GetHighlightedImage() const { return HighlightedImage; }
	USoundBase * GetHoveredSound() const { return HoveredSound; }
	USoundBase * GetPressedByLMBSound() const { return PressedByLMBSound; }
	USoundBase * GetPressedByRMBSound() const { return PressedByRMBSound; }
	const FText & GetDescription() const { return Description; }
	ECommanderAbility GetType() const { return Type; }
	EAbilityTargetingMethod GetTargetingMethod() const { return TargetingMethod; }
	bool CanTargetInsideFog() const { return bCanTargetInsideFog; }
	bool CanTargetEnemies() const { return bCanTargetEnemies; }
	bool CanTargetFriendlies() const { return bCanTargetFriendlies; }
	bool CanTargetSelf() const { return bCanTargetSelf; }
	const TSet<FName> & GetAcceptableSelectableTargetFNames() const { return AcceptableSelectableTargetFNames; }
	float GetCooldown() const { return Cooldown; }
	float GetInitialCooldown() const { return InitialCooldown; }
	bool HasUnlimitedUses() const { return NumCharges == -1; }
	int32 GetNumUses() const { return NumCharges; }
	const TSubclassOf<UCommanderAbilityBase> & GetEffectBP() const { return Effect_BP; }
	UCommanderAbilityBase * GetAbilityObject(ARTSGameState * GameState) const;
	EAbilityMouseAppearance GetMouseAppearance() const { return MouseAppearance; }
	const FCursorInfo & GetDefaultMouseCursorInfo() const { return DefaultMouseCursor_Info; }
	const FCursorInfo & GetAcceptableTargetMouseCursorInfo() const { return AcceptableTargetMouseCursor_Info; }
	const FCursorInfo & GetUnacceptableTargetMouseCursorInfo() const { return UnacceptableTargetMouseCursor_Info; }
	const FBasicDecalInfo & GetAcceptableLocationDecalInfo() const { return AcceptableLocationDecalInfo; }
	const FBasicDecalInfo & GetUnusableLocationDecalInfo() const { return UnusableLocationDecalInfo; }

protected:

	//----------------------------------------------------------
	//	Visual/Audio data
	//----------------------------------------------------------

	/** Name of action to appear on HUD */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/** Image to appear in HUD */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush NormalImage;

	/**
	 *	The image for when the mouse is over a button for this ability. This will be ignored
	 *	if you use a unified image for the HUD element this appears on.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HoveredImage;

	/**
	 *	The image for when the mouse is pressed on a button for this ability. This will be ignored
	 *	if you use a unified image for the HUD element this appears on.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush PressedImage;

	/** The image for when button should be highlighted */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HighlightedImage;

	/** 
	 *	The sound to play when the mouse is hovered over a button on a action bar for this ability.
	 *	This will be ignored if you use a unified sound set in game instance BP 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * HoveredSound;

	/**
	 *	The sound to play the moment the left mouse button is pressed on an action bar button for
	 *	this ability. This will be ignored if you use a unified sound set in game instance BP 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * PressedByLMBSound;

	/**
	 *	The sound to play at the moment the right mouse button is pressed on an action bar button
	 *	for this ability. This will be ignored if you use a unified sound set in game instance BP 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * PressedByRMBSound;

	/** A description of the ability */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Description;

	//----------------------------------------------------------
	//	Behavior data
	//----------------------------------------------------------

	/* Type of button this is */
	UPROPERTY()
	ECommanderAbility Type;

	/* What this ability targets */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EAbilityTargetingMethod TargetingMethod;

	/**
	 *	This is only relevant if the ability can target a location in the world.
	 *	Whether we are allowed to use this ability on locations that are inside fog of war.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditWorldLocationTargetingProperties))
	uint8 bCanTargetInsideFog : 1;

	/* Whether ability can use an enemy as a target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	uint8 bCanTargetEnemies : 1;

	/**
	 *	Whether ability can use a selectable on your team as a target. Note if this and
	 *	bCanTargetEnemies is false then your ability will not be able to target anything
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	uint8 bCanTargetFriendlies : 1;

	/**
	 *	Whether can target self. bCanTargetFriendlies must be true for this to be relevant.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSingleTargetProperties))
	uint8 bCanTargetSelf : 1;

	/**
	 *	The types of selectables this ability can target.
	 *	You must add at least one type for your ability to be usable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSelectableTargetingProperties))
	TSet <ETargetingType> AcceptableSelectableTargets;

	UPROPERTY()
	TSet<FName> AcceptableSelectableTargetFNames;

	/**
	 *	Cooldown
	 *
	 *	0 = no cooldown.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float Cooldown;

	/**
	 *	The cooldown when this ability is aquired.
	 *
	 *	e.g. in C&C generals imagine when you aquire fuel air bomb you have to wait 1 min before 
	 *	you can use it. That is what this is.
	 *
	 *	0 = starts off cooldown
	 *
	 *	This may be ignored if this ability is the 2nd/3rd/etc rank of an ability on a single 
	 *	commander skill tree node i.e. only the 1st rank ability matters when it comes to this variable.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float InitialCooldown;

	/** 
	 *	Number of times this ability can be used. 
	 *	-1 = infinite 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "-1"))
	int32 NumCharges;

	/* Blueprint for the effect of this button. This is pretty important */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Effect"))
	TSubclassOf <UCommanderAbilityBase> Effect_BP;

	//----------------------------------------------------------
	//	Appearance related data
	//----------------------------------------------------------

	/**
	 *	Whether to use a custom mouse cursor or a decal when choosing where to use ability. If using
	 *	CustomMouseCursor it is assumed at least DefaultMouseCursor has a valid path to a cursor.
	 *	If using HideAndShowDecal it is assumed Decal is set. 
	 *
	 *	My notes: perhaps if the ability targets a selectable then do not allow decal here?
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EAbilityMouseAppearance MouseAppearance;

	/* Default mouse cursor for ability to show when not hovering over a selectable */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType DefaultMouseCursor;

	/**
	 *	Mouse cursor to appear when hovering over a selectable that is an acceptable target for
	 *	this ability
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType AcceptableTargetMouseCursor;

	/**
	 *	Mouse cursor to appear when hovering over a selectable that is not an acceptable target
	 *	for this ability
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMouseCursorProperties, ShowOnlyInnerProperties))
	EMouseCursorType UnacceptableTargetMouseCursor;

	FCursorInfo DefaultMouseCursor_Info;
	FCursorInfo AcceptableTargetMouseCursor_Info;
	FCursorInfo UnacceptableTargetMouseCursor_Info;

	/**
	 *	The decal to draw at the mouse location when the location is one where it is 
	 *	acceptable to use the ability.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditDecalProperties))
	FBasicDecalInfo AcceptableLocationDecalInfo;

	/**
	 *	The decal to draw at the mouse location when the location is one where it is
	 *	not acceptable to use the ability.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditDecalProperties))
	FBasicDecalInfo UnusableLocationDecalInfo;
};


/* A prerequisite for a commander skill */
USTRUCT()
struct FCommanderSkillTreePrerequisiteArrayEntry
{
	GENERATED_BODY()

public:

	FCommanderSkillTreePrerequisiteArrayEntry()
		: PrerequisiteType(ECommanderSkillTreeNodeType::None) 
		, PrerequisiteRank(1)
	{
	}

	/* This ctor is here for checking if array contains entry, but it's probably better to
	use that FindBYPRedicate func or whatever and do away with this */
	explicit FCommanderSkillTreePrerequisiteArrayEntry(ECommanderSkillTreeNodeType InPrerequisiteType, uint8 InAbilityRank)
		: PrerequisiteType(InPrerequisiteType) 
		, PrerequisiteRank(InAbilityRank)
	{
	}

	friend bool operator==(const FCommanderSkillTreePrerequisiteArrayEntry & Elem_1, const FCommanderSkillTreePrerequisiteArrayEntry & Elem_2) 
	{
		return Elem_1.PrerequisiteType == Elem_2.PrerequisiteType 
			&& Elem_1.PrerequisiteRank == Elem_2.PrerequisiteRank;
	}

	friend uint32 GetTypeHash(const FCommanderSkillTreePrerequisiteArrayEntry & Elem_1)
	{
		return static_cast<uint32>(Elem_1.PrerequisiteType);
	}

	ECommanderSkillTreeNodeType GetPrerequisiteType() const { return PrerequisiteType; }
	uint8 GetPrerequisiteRank() const { return PrerequisiteRank; }

	FString ToString() const;

protected:

	/* The skill that needs to be aquired */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ECommanderSkillTreeNodeType PrerequisiteType;

	/** 
	 *	The rank of PrerequisiteType that needs to be aquired. 
	 *	
	 *	This is not 0 indexed so 1 means the first rank of the ability. Just thought I would say 
	 *	that because under the hood I treat a 0 as rank 1.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	uint8 PrerequisiteRank;
};


USTRUCT()
struct FCommanderAbilityTreeNodeSingleRankInfo
{
	GENERATED_BODY()

public:

	FCommanderAbilityTreeNodeSingleRankInfo();

	ECommanderAbility GetAbilityType() const { return AbilityType; }
	uint8 GetUnlockRank() const { return UnlockRank; }
	int32 GetCost() const { return Cost; }
	const FCommanderAbilityInfo * GetAbilityInfo() const { return AbilityInfo; }
	void SetAbilityInfo(const FCommanderAbilityInfo * InInfoStruct) { AbilityInfo = InInfoStruct; }

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ECommanderAbility AbilityType;

	/* Get the rank this ability unlocks at */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 UnlockRank;

	/* How many skill points it takes to unlock this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int32 Cost;

	/* Pointer to info struct for ability */
	const FCommanderAbilityInfo * AbilityInfo;
};


/* A single node on the commander's (commander == player) skill tree */
USTRUCT()
struct FCommanderAbilityTreeNodeInfo
{
	GENERATED_BODY()

public:

	FCommanderAbilityTreeNodeInfo();

	void SetupInfo(URTSGameInstance * GameInst, ECommanderSkillTreeNodeType InNodeType);

	//------------------------------------------------------------
	//	Getters and setters
	//------------------------------------------------------------

	/* @param AbilityIndex - index in Abilities */
	const FText & GetName(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetName(); }
	const FSlateBrush & GetNormalImage(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetNormalImage(); }
	const FSlateBrush & GetHoveredImage(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetHoveredImage(); }
	const FSlateBrush & GetPressedImage(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetPressedImage(); }
	USoundBase * GetHoveredSound(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetHoveredSound(); }
	USoundBase * GetPressedByLMBSound(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetPressedByLMBSound(); }
	USoundBase * GetPressedByRMBSound(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetPressedByRMBSound(); }
	const FText & GetDescription(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetAbilityInfo()->GetDescription(); }
	ECommanderSkillTreeNodeType GetNodeType() const { return Type; }
	bool OnlyExecuteOnAquired() const { return bOnlyExecuteOnAquired; }

	// 0 indexed
	bool IsRankValid(int32 AbilityRank) const { return Abilities.Num() > AbilityRank; }

	// AbilityIndex is 0 indexed so 0 is the first rank
	const FCommanderAbilityTreeNodeSingleRankInfo & GetAbilityInfo(int32 AbilityIndex) const { return Abilities[AbilityIndex]; }
	const TArray <FCommanderSkillTreePrerequisiteArrayEntry> & GetPrerequisites() const { return Prerequisites; }
	uint8 GetUnlockRank(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetUnlockRank(); }
	int32 GetCost(int32 AbilityIndex) const { return Abilities[AbilityIndex].GetCost(); }
	/* Get how many ranks of abilities are on this node */
	int32 GetNumRanks() const { return Abilities.Num(); }

protected:
	
	//------------------------------------------------------------
	//	Behavior data
	//------------------------------------------------------------

	UPROPERTY()
	ECommanderSkillTreeNodeType Type;

	/** 
	 *	If true then all abilities on this node are executed when they are aquired. Also they 
	 *	aren't added to the global skills panel and it is implied that they only have 1 use 
	 *	and no cooldown. 
	 *	
	 *	A good example of a true here would be an ability that unlocks being able to build something 
	 *	e.g. in C&C generals: pathfinders, nuke cannons
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOnlyExecuteOnAquired;

	/* Prerequisite abilities: abilities that must be aquired before this one can be */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<FCommanderSkillTreePrerequisiteArrayEntry> Prerequisites;

	/** 
	 *	The abilities. Add multiple entries for multiple ranks. 
	 *	Index 0 = 1st rank 
	 *	Index 1 = 2nd rank 
	 *	etc. 
	 *	If you want the ability to have multiple ranks then add multiple entries e.g. C&C generals 
	 *	china artillery strike. 
	 *	If you want each rank of an ability to be on seperate nodes then just add one entry to 
	 *	this array.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<FCommanderAbilityTreeNodeSingleRankInfo> Abilities;
};


/* Info about what happens when the player reaches a rank */
USTRUCT()
struct FCommanderLevelUpInfo
{
	GENERATED_BODY()

#if WITH_EDITOR
	// For a GET_MEMBER_NAME_CHECKED
	friend AFactionInfo; 
#endif

public:

	FCommanderLevelUpInfo()
		: Sound(nullptr) 
		, SkillPointGain(1) 
		, ExperienceRequired(100.f)
	{
	}

	USoundBase * GetSound() const { return Sound; }
	int32 GetSkillPointGain() const { return SkillPointGain; }
	float GetExperienceRequired() const { return ExperienceRequired; }
	float GetCumulativeExperienceRequired() const { return CumulativeExperienceRequired; }
	void SetCumulativeExperienceRequired(float InValue) { CumulativeExperienceRequired = InValue; }

protected:

	/* UI sound to play */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound;

	/* How many skill points the player gains for reaching this level */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	int32 SkillPointGain;

	/* How much experience is required to get to this level from the one below. Not cumulative */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float ExperienceRequired;

	/**
	 *	How much experience is required to reach this level taking into account the amounts 
	 *	required to reach all the previous levels. This can be assigned its value in post edit 
	 */
	UPROPERTY()
	float CumulativeExperienceRequired;
};


USTRUCT()
struct FUnaquiredCommanderAbilityState
{
	GENERATED_BODY()

public:

	// Here so compile. Never use this
	FUnaquiredCommanderAbilityState() { }

	/* Constructor to use */
	FUnaquiredCommanderAbilityState(const FCommanderAbilityTreeNodeInfo * InNodeInfo);

	/* To update prerequisites */
	void OnAnotherAbilityAquired(const FCommanderSkillTreePrerequisiteArrayEntry & AquiredAbilityTuple);

	bool ArePrerequisitesMet() const { return MissingPrerequisites.Num() == 0; }

protected:

	/* Pointer to the node info struct */
	const FCommanderAbilityTreeNodeInfo * NodeInfo;

	/* Missing prerequisites */
	TSet<FCommanderSkillTreePrerequisiteArrayEntry> MissingPrerequisites;
};


USTRUCT()
struct FAquiredCommanderAbilityState
{
	GENERATED_BODY()

public:

	/* Do not use this ctor */
	FAquiredCommanderAbilityState() { }

	/* This is the ctor you should use */
	FAquiredCommanderAbilityState(ARTSPlayerState * AbilityOwner, const FCommanderAbilityInfo * InAbilityInfo);

	const FCommanderAbilityInfo * GetAbilityInfo() const { return AbilityInfo; }
	int32 GetAquiredRank() const { return RankAquired; }
	void OnAnotherRankAquired(const FCommanderAbilityInfo * NextRanksAbilityInfo)
	{ 
		AbilityInfo = NextRanksAbilityInfo;
		
		RankAquired++; 

		NumTimesUsed = 0;
	}

	float GetCooldownRemaining(const FTimerManager & TimerManager) const;

	/* Normalized in range 0 to 1. 0 means off cooldown, 1 means max cooldown remaining */
	float GetCooldownPercent(const FTimerManager & TimerManager) const;

	int32 GetNumTimesUsed() const { return NumTimesUsed; }

	int32 GetGlobalSkillsPanelButtonIndex() const;
	void SetGlobalSkillsPanelButtonIndex(int32 InIndex) { GlobalSkillsPanelButtonIndex = InIndex; }

	/**
	 *	@param AbilityInstigator - the player that used the ability 
	 *	@param ThisAbilityInfo - this will AbilityInfo. It's probably better performance to use 
	 *	this param instead. 
	 */
	void OnAbilityUsed(ARTSPlayerState * AbilityInstigator, const FCommanderAbilityInfo & ThisAbilityInfo, URTSHUD * HUD);

protected:

	/* Pointer to info struct this struct is for */
	const FCommanderAbilityInfo * AbilityInfo;

	FTimerHandle TimerHandle_CooldownRemaining;

	/* To what rank the ability has been aquired. 0 indexed so a value of 0 means rank 1. */
	int32 RankAquired;

	/* Number of times the ability has been used for the current aquired rank of the ability only */
	int32 NumTimesUsed;

	/* Where on the global skills panel widget this ability is */
	int32 GlobalSkillsPanelButtonIndex;
};


USTRUCT()
struct FBuildingTargetingAbilityStaticInfo
{
	GENERATED_BODY()

public:

	FBuildingTargetingAbilityStaticInfo();

	/**
	 *	Do initial setup 
	 *	
	 *	@param InType - the type of ability this struct holds info for 
	 */
	void InitialSetup(EBuildingTargetingAbility InType);
	void SetupMouseCursors(URTSGameInstance * GameInst, EMouseCursorType MaxEnumValue);

	EBuildingTargetingAbility GetAbilityType() const { return AbilityType; }
	UBuildingTargetingAbilityBase * GetEffectObject() const { return EffectObject; }
	const FCursorInfo & GetAcceptableTargetMouseCursor() const { return AcceptableTargetMouseCursor_Info; }
	bool HasUnlimitedRange() const { return GetRange() <= 0.f; }
	float GetRange() const { return Range; }
	bool HasAnimation() const { return bUsesAnimation; }
	EAnimation GetAnimation() const { return Animation; }
	bool ConsumesInstigator() const { return bConsumeInstigator; }

	/* Something to think about/todo: 
	implement the ability 'consuming' the instigator a la engineers or spies. Probably 
	want a special float value like -FLT_MAX that we use to signal this. In AInfantry::OnRep_Health 
	it will check for this special value and if yes then take it that the unit should be destroyed 
	instantly cause it consumed itself from entering a building to use an ability */

protected:

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<UBuildingTargetingAbilityBase> Effect_BP;

	UPROPERTY()
	UBuildingTargetingAbilityBase * EffectObject;

	/**
	 *	Mouse cursor to use when the player hovers the mouse over a building which is an acceptable
	 *	target for the ability.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType AcceptableTargetMouseCursor;

	FCursorInfo AcceptableTargetMouseCursor_Info;

	/** 
	 *	Range of the ability. 
	 *	0 = unlimited 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float Range;

	/* Type of ability this struct holds info for */
	EBuildingTargetingAbility AbilityType;

	/* Whether to use an animation */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bUsesAnimation;

	/* Animation to play */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUsesAnimation))
	EAnimation Animation;

	/**
	 *	If true then the instigator is 'consumed' the moment the ability is used 
	 *	Examples of true: 
	 *	- C&C engineers 
	 *	- C&C spies 
	 *	Examples of false: 
	 *	- C&C commando putting bombs on buildings 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bConsumeInstigator;
};


/* Info struct for an ability that is used against buildings only */
USTRUCT()
struct FBuildingTargetingAbilityPerSelectableInfo
{
	GENERATED_BODY()

public:

	FBuildingTargetingAbilityPerSelectableInfo();

	EBuildingTargetingAbility GetAbilityType() const { return AbilityType; }

	const FCursorInfo & GetAcceptableTargetMouseCursor(URTSGameInstance * GameInst) const;

	/* Return true if the buildings affiliation is an acceptable type for this ability */
	bool IsAffiliationTargetable(EAffiliation Affiliation) const;

protected:

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingTargetingAbility AbilityType;

	/**
	 *	If true the ability is usable against buildings that are owned by the player.
	 *	Note the building's type must also be acceptable too for the ability to be usable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bUsableAgainstOwnedBuildings : 1;

	/**
	 *	If true the ability is usable against buildings that are owned by allies
	 *	Note the building's type must also be acceptable too for the ability to be usable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bUsableAgainstAlliedBuildings : 1;

	/**
	 *	If true the ability is usable against buildings that are owned by enemies.
	 *	Note the building's type must also be acceptable too for the ability to be usable
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bUsableAgainstHostileBuildings : 1;
};


/* Attributes for abilities that target buildings */
USTRUCT()
struct FBuildingTargetingAbilitiesAttributes
{
	GENERATED_BODY()

public:

	/* Will return null if there is no ability that can target the building */
	const FBuildingTargetingAbilityPerSelectableInfo * GetAbilityInfo(EFaction BuildingsFaction, EBuildingType BuildingType, EAffiliation BuildingsAffiliation) const;

protected:

	/**
	 *	Abilities to be used against buildings if the player
	 *	right-clicked on it e.g. in C&C engineers capture buildings, spies let you see what is
	 *	being produced, etc.
	 *
	 *	If you want the ability to be usable against ALL buildings then set building type 
	 *	for the key to NotBuilding, although I think I haven't exposed that enum value to 
	 *	editor so I should probably do that simple as pie TODO
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	TMap<FFactionBuildingTypePair, FBuildingTargetingAbilityPerSelectableInfo> BuildingTargetingAbilities;
};


/**
 *	A queue that produces buildings/units/upgrades. Found on buildings mostly but there is also 
 *	one the player state. A single queue cannot produce in parallel.
 *
 *	My implementation of queues:
 *	Production orders come in 3 types: build building, train unit and research upgrade.
 *	Queues come in 2 types:
 *		- A "persistent" queue which is used only for building buildings when clicking on a button
 *		in the HUD persistent panel (like in C&C). However if clicking a unit/upgrade in the HUD
 *		persistent panel then the production will be put into a "context" queue (described below).
 *		- A "context" queue which handles every other case. This includes producing units/upgrades
 *		from the HUD persistent panel, or producing anything by clicking on the building and pressing
 *		a button on its context menu.
 *
 *	Buildings have one of each queue type. This means you can have a construction yard type building
 *	that will construct buildings in its "persistent" queue while at the same time it is possible to
 *	click on that construction yard building and build units/upgrades/buildings (buildings is kind of
 *	unexpected behavior here but can be done) in its "context" queue. Or you can produce
 *	units/upgrades from it by clicking a unit/upgrade button in the HUD persistent panel. So you can
 *	have a construction yard that constructs a building while at the same time trains a unit or
 *	researches an upgrade.
 */
USTRUCT()
struct FProductionQueue
{
	GENERATED_BODY()

public:

	FProductionQueue();

protected:

	/* Reversed queue of units being built. Last element is the front of queue.
	Could use std::deque.
	Some performance figures:
	- Add unit to queue: O(n)
	- Remove from front of queue when production complete: O(1)
	- Remove from middle when cancel training: O(n) */
	UPROPERTY()
	TArray < FTrainingInfo > Queue;

	/* This is the queue used by CPU player AI controllers */
	UPROPERTY()
	TArray < FCPUPlayerTrainingInfo > AICon_Queue;

	/* Whether this is a HUD persistent queue or a context menu queue. Persistent queues will
	only be used for building buildings from the HUD persistent panel, similar to a construction
	yard in C&C. Persistent panel commands to produce units or upgrades will go to the context
	queue */
	UPROPERTY()
	EProductionQueueType Type;

	/* The max number of items in the queue at once */
	UPROPERTY()
	int32 Capacity;

	/* Timer handle to track progress of front of queue */
	UPROPERTY()
	FTimerHandle TimerHandle_FrontOfQueue;

	/* For remote clients only. If their timer handle completes before the server's completes
	then record this so UI will show 100% while we wait for the server confirmation */
	UPROPERTY()
	bool bHasCompletedEarly;

	/* True if the item at the front of the queue is for a building, its build method is
	BuildsInTab and it has completed production and is therefore ready to be placed in world */
	UPROPERTY()
	bool bHasCompletedBuildsInTab;

	/* If this queue is producing a building using the BuildsItself build method the build 
	it is producing. Currently I only use this for CPU players but it will probably need to 
	be used for human players too. Also this is a persistent queue only variable and is one 
	more reason to make seperate context and persistent queue structs. 
	
	This is possibly only maintained server-side */
	UPROPERTY()
	TWeakObjectPtr < ABuilding > BuildingBeingProduced;

public:

	/* Setup some values critical for the queue to function */
	void SetupForHumanOwner(EProductionQueueType InType, int32 InCapacity);

	/* Constructor for queues that belong to CPU players */
	void SetupForCPUOwner(EProductionQueueType InType, int32 InCapacity);

	/* Return true if the queue has room for more
	@param bShowHUDWarning - if return false, whether to try and show a message on the HUD
	@return - true if the queue has room for another item */
	bool HasRoom(ARTSPlayerState * OwningPlayer, bool bShowHUDWarning = false) const;

	/* Get the max number of items that can be in the queue at once */
	int32 GetCapacity() const;

	/* Get training info at front of queue. Assumes there is at least one item in queue */
	const FTrainingInfo & Peek() const;

	/* To adjust timer handle for visuals only */
	void Client_OnProductionComplete();

	/* Add to back of queue */
	void AddToQueue(const FTrainingInfo & TrainingInfo);

	/* Set BuildingBeingProduced - the building that we are producing using this queue. The 
	building will have the build method BuildsItself */
	void SetBuildingBeingProduced(ABuilding * InBuildingWeAreProducing);

	/* Get timer handle for front of queue */
	const FTimerHandle & GetTimerHandle() const;
	FTimerHandle & GetTimerHandle();

	/* Get the value to display on HUD for progress of production */
	float GetPercentageCompleteForUI(const UWorld * World) const;

	/* Get the unit at the front of the queue. This assumes that it is indeed a unit at the 
	front and not something like a building or upgrade */
	EUnitType GetUnitAtFront(bool bIsOwnedByCPUPlayer) const;

	// Possibly only useful if on server
	TWeakObjectPtr < ABuilding > & GetBuildingBeingProduced();

	/* Get how many items are in the queue */
	int32 Num() const;
	void Pop();
	void Insert(const FTrainingInfo & Item, int32 Index);

	const FTrainingInfo & operator[](int32 Index) const;

	/* Call on remote client only */
	void SetHasCompletedEarly(bool bNewValue);

	void SetHasCompletedBuildsInTab(bool bNewValue);

	/* Whether the queue has finished BuildsInTab production */
	bool HasCompletedBuildsInTab() const;

	/* Call when player places building from front into world that was built using BuildsInTab */
	void OnBuildsInTabBuildingPlaced();

	EProductionQueueType GetType() const;

	/* Not implemented */
	void Server_CancelProductionOfFrontItem();

#if WITH_EDITOR
	/* @param NewMax - the new size limit of queue */
	void OnPostEdit(int32 NewCapacity);
#endif

	//--------------------------------------------------------------------------
	//	CPU player only functions
	//--------------------------------------------------------------------------
	//	When putting stuff in queue CPU players should call these functions

	/* Return whether queue has enough room */
	int32 AICon_Num() const;
	FCPUPlayerTrainingInfo AICon_Pop();
	bool AICon_HasRoom() const;
	void AICon_Insert(const FCPUPlayerTrainingInfo & Item, int32 Index);
	const FCPUPlayerTrainingInfo & AICon_Last() const;
	// Brack operator for AICon_Queue
	const FCPUPlayerTrainingInfo & AICon_BracketOperator(int32 Index) const;
};


/* Holds what buttons a context menu has */
USTRUCT()
struct FContextMenuButtons
{
	GENERATED_BODY()

protected:

	/* Put context buttons in here */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSet < FContextButton > Buttons;

	/* Array of buttons for HUD ordering purposes */
	UPROPERTY()
	TArray < FContextButton > ButtonsArray;

public:

	/* Return true if Buttons contains param */
	bool HasButton(FContextButton Button) const { return Buttons.Contains(Button); }

	/* Trivial getters and setters */

	const TSet <FContextButton> & GetButtons() const { return Buttons; }
	TSet <FContextButton> & GetButtons() { return Buttons; }
	const TArray < FContextButton > & GetButtonsArray() const { return ButtonsArray; }
	TArray < FContextButton > & GetButtonsArray() { return ButtonsArray; }
};


/* Struct that is used for info on buildings, units and upgrades. Contains base data needed to
display info on the HUD. FBuildInfo and FUpgradeInfo derive from it */
USTRUCT()
struct FDisplayInfoBase
{
	GENERATED_BODY()

protected:

	/* Fields left as EditDefaultsOnly only for FUpgradeInfo. FBuildInfo lie in a uneditable array
	so it will have no effect on them */

	/* Name to appear on HUDs */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	FText Name;

	/* Image to appear on HUD */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	FSlateBrush HUDImage;

	/** 
	 *	Image when a button for this is hovered. 
	 *	This will be ignored if you use a unified image as set in GI 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	FSlateBrush HoveredHUDImage;

	/** 
	 *	Image when a button for this is pressed. 
	 *	This will be ignored if you use a unified image as set in GI 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	FSlateBrush PressedHUDImage;

	/** 
	 *	Sound for when a UI button for this is hovered. 
	 *	This will be ignored if you choose to use a unified sound as set in GI 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	USoundBase * HoveredButtonSound;

	/** 
	 *	Sound for when a UI button for this is pressed with left mouse button.
	 *	This will be ignored if you choose to use a unified sound as set in GI 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	USoundBase * PressedByLMBSound;

	/** 
	 *	Sound for when a UI button for this is pressed with right mouse button.
	 *	This will be ignored if you choose to use a unified sound as set in GI 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	USoundBase * PressedByRMBSound;

	/* A description of this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|UI")
	FText Description;

	/* Sound to try and play when just built */
	UPROPERTY()
	USoundBase * JustBuiltSound;

	/* Cost to build/train. Use function ResourceTypeToArrayIndex to get correct index for a 
	resource type */
	UPROPERTY()
	TArray <int32> Costs;

	/* Time to build/train/research */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Production Time"))
	float TrainTime;

	/** 
	 *	Prerequisite buildings that must be built before this can be researched. Adding duplicates
	 *	of the same building will have no additional effect. You do not need to add the building that
	 *	builds/trains/researches this. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray <EBuildingType> Prerequisites_Buildings;

	/** 
	 *	Upgrades that must be researched before this can be built/trained/researched. 
	 *	No point adding duplicates. 
	 *	
	 *	My notes: TODO add duplicate protection for this array in post edit by removing them. 
	 *	Actually I think I need to add protection for the buildings prereqs array too.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<EUpgradeType> Prerequisites_Upgrades;

	/* Text of prerequisites so it can be shown on the HUD */
	UPROPERTY()
	FText PrerequisitesText;

	/* Tab type if using a persistent HUD panel like C&C */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EHUDPersistentTabType HUDPersistentTabType;

	/* Ordering number for tab is using a persistent HUD panel like C&C */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	int32 HUDPersistentTabOrdering;

	/* Context menu if this is for a selectable */
	UPROPERTY()
	FContextMenuButtons ContextMenu;

public:

	FDisplayInfoBase();

	virtual ~FDisplayInfoBase()
	{
	}

	const TArray < int32 > & GetCosts() const { return Costs; }
	float GetTrainTime() const { return TrainTime; }
	const TArray <EBuildingType> & GetPrerequisites() const { return Prerequisites_Buildings; }
	const TArray <EUpgradeType> & GetUpgradePrerequisites() const { return Prerequisites_Upgrades; }
	const FText & GetPrerequisitesText() const { return PrerequisitesText; }
	const FText & GetName() const { return Name; }
	const FSlateBrush & GetHUDImage() const { return HUDImage; }
	const FSlateBrush & GetHoveredHUDImage() const { return HoveredHUDImage; }
	const FSlateBrush & GetPressedHUDImage() const { return PressedHUDImage; }
	USoundBase * GetHoveredButtonSound() const { return HoveredButtonSound; }
	USoundBase * GetPressedByLMBSound() const { return PressedByLMBSound; }
	USoundBase * GetPressedByRMBSound() const { return PressedByRMBSound; }
	const FText & GetDescription() const { return Description; }
	USoundBase * GetJustBuiltSound() const { return JustBuiltSound; }
	EHUDPersistentTabType GetHUDPersistentTabCategory() const { return HUDPersistentTabType; }
	int32 GetHUDPersistentTabOrdering() const { return HUDPersistentTabOrdering; }
	const FContextMenuButtons * GetContextMenu() const { return &ContextMenu; }

	void SetupCostsArray(const TMap < EResourceType, int32 > & InCosts);
	void SetTrainTime(float InTrainTime) { TrainTime = InTrainTime; }
	void SetPrerequisites(const TArray <EBuildingType> & ToClone_Buildings, const TArray<EUpgradeType> & ToClone_Upgrades);
	void SetName(const FText & InName) { Name = InName; }
	void SetHUDImage(const FSlateBrush & InImage) { HUDImage = InImage; }
	void SetHUDHoveredImage(const FSlateBrush & InImage) { HoveredHUDImage = InImage; }
	void SetHUDPressedImage(const FSlateBrush & InImage) { PressedHUDImage = InImage; }
	void SetDescription(const FText & InDescription) { Description = InDescription; }
	void SetJustBuiltSound(USoundBase * InSound) { JustBuiltSound = InSound; }
	void SetHUDPersistentTabType(EHUDPersistentTabType InType) { HUDPersistentTabType = InType; }
	void SetHUDPersistentTabOrdering(int32 InNumber) { HUDPersistentTabOrdering = InNumber; }
	void SetContextMenu(const FContextMenuButtons & InMenu) { ContextMenu = InMenu; }

	/* From the prerequisites array creates a text version of it */
	void CreatePrerequisitesText(const AFactionInfo * FactionInfo);

#if WITH_EDITOR
	virtual void OnPostEdit();
#endif
};


/* Info for upgrades. They only affect selectables you control. You can affect
types of units/buildings and/or damage and armour types */
USTRUCT()
struct FUpgradeInfo : public FDisplayInfoBase
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* How much this upgrade costs. Any resource types without entries implies 0 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Costs"))
	TMap <EResourceType, int32> UpgradeCosts;
#endif

	/** 
	 *	If true this upgrade is applied to all units under your control. If false you can 
	 *	specify which types are affected 
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bAffectsAllUnitTypes;

	/** 
	 *	If true this upgrade is applied to all buildings under your control. If false you can 
	 *	specify which types are affected 
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bAffectsAllBuildingTypes;

	/* If bAffectsAllUnitTypes is false, the unit types affected by this upgrade */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bIsUnitSetEditable))
	TSet < EUnitType > UnitsAffected;

	/* If bAffectsAllBuildingTypes is false, the building types affected by this upgrade */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bIsBuildingSetEditable))
	TSet < EBuildingType > BuildingsAffected;

	/** 
	 *	Effect of upgrade. This decides what the upgrade does to those affected by it.
	 *	You can leave this null to let the upgrade have no effect (but perhaps you just had 
	 *	the upgrade as a prerequisite for something so there was still a point in obtaining it).
	 *
	 *	See UpgradeEffect.h for info on how to implement these 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Effect"))
	TSubclassOf < UUpgradeEffect > Effect_BP;

	/* Array of which buildings can research this upgrade */
	UPROPERTY()
	TArray < EBuildingType > TechTreeParents;

public:

	FUpgradeInfo();

	virtual ~FUpgradeInfo()
	{
	}

	/* Does this upgrade affect a certain unit type? */
	bool AffectsUnitType(EUnitType UnitType) const;

	/* Does this upgrade affect a certain building type? */
	bool AffectsBuildingType(EBuildingType BuildingType) const;

	/** 
	 *	Add an entry to TechTreeParents 
	 *
	 *	@param BuildingType - the type of building that can research this upgrade 
	 */
	void AddTechTreeParent(EBuildingType BuildingType);

#if WITH_EDITORONLY_DATA
	/* Field for toggling visibility of other fields in editor */

	UPROPERTY()
	bool bIsUnitSetEditable;

	UPROPERTY()
	bool bIsBuildingSetEditable;
#endif

#if WITH_EDITOR
	virtual void OnPostEdit() override;
#endif

	/* Getters and setters */

	bool AffectsAllUnits() const { return bAffectsAllUnitTypes; }
	bool AffectsAllBuildings() const { return bAffectsAllBuildingTypes; }
	const TSet <EUnitType> & GetUnitTypesAffected() const { return UnitsAffected; }
	const TSet <EBuildingType> & GetBuildingTypesAffected() const { return BuildingsAffected; }
	TSubclassOf <UUpgradeEffect> GetEffectBP() const { return Effect_BP; }

	/** 
	 *	Return a random building that can build this. Returns EBuildingType::NotBuilding if 
	 *	no building can research this upgrade 
	 */
	EBuildingType Random_GetTechTreeParent() const;
};


/* Base for selectables info */
USTRUCT()
struct FBuildInfo : public FDisplayInfoBase
{
	GENERATED_BODY()

protected:

	/* Reference to blueprint to spawn selectable this is for */
	UPROPERTY()
	TSubclassOf < AActor > Selectable_BP;

	/* Type of selectable this struct holds data for */
	UPROPERTY()
	ESelectableType SelectableType;

	/* The type of building this struct is for. Should be set to NotBuilding if not for a building */
	UPROPERTY()
	EBuildingType BuildingType;

	/* The type of unit this struct is for. Should be set to NotUnit if not for a unit */
	UPROPERTY()
	EUnitType UnitType;

	/* Whether to announce to all players when built */
	UPROPERTY()
	bool bAnnounceToAllWhenBuilt;

public:

	/* For sorting. Just sorts on EBuildingType or EUnitType but does some
	checks to make sure the selectable type is the same */
	friend bool operator<(const FBuildInfo & Info1, const FBuildInfo & Info2);

	const TSubclassOf < AActor > & GetSelectableBP() const { return Selectable_BP; }
	ESelectableType GetSelectableType() const { return SelectableType; }
	EBuildingType GetBuildingType() const { return BuildingType; }
	EUnitType GetUnitType() const { return UnitType; }
	bool AnnounceToAllWhenBuilt() const { return bAnnounceToAllWhenBuilt; }
	/* Get the button that would build this. Should be called after everything has been setup */
	FContextButton ConstructBuildButton() const;

	void SetSelectableBP(const TSubclassOf < AActor > & InBP) { Selectable_BP = InBP; }
	void SetBuildingType(EBuildingType InBuildingType) { BuildingType = InBuildingType; }
	void SetUnitType(EUnitType InUnitType) { UnitType = InUnitType; }
	void SetSelectableType(ESelectableType InType) { SelectableType = InType; }
	void SetAnnounceToAllWhenBuilt(bool bNewValue) { bAnnounceToAllWhenBuilt = bNewValue; }
};


/* Struct to store info necessary to spawn buildings. */
USTRUCT()
struct FBuildingInfo : public FBuildInfo
{
	GENERATED_BODY()

protected:

	/* Out of buildings/units/upgrades currently only buildings might need the highlighted image 
	so here it is */
	UPROPERTY()
	FSlateBrush HighlightedImage;

	/* Reference to ghost building BP. Can leave null for units */
	UPROPERTY()
	TSubclassOf <AGhostBuilding> GhostBuilding_BP;

	/* Half height of bounds. */
	UPROPERTY()
	float BoundsHeight;

	/* The scaled box extent of the root component that is a box component. This isn't affected
	by rotation and because it is scaled it will be the actual size */
	UPROPERTY()
	FVector ScaledBoxExtent;

	/* Building build method */
	UPROPERTY()
	EBuildingBuildMethod BuildingBuildMethod;

	/* Whether building should rise from the ground when it is being built */
	UPROPERTY()
	bool bBuildingRisesFromGround;

	/* The how close to other buildings this building must be to be built
	@See AFactionInfo::BuildingProximityRange */
	UPROPERTY()
	float BuildProximityDistance;

	/* @See FBuildingAttributes::FoundationRadius */
	UPROPERTY()
	float FoundationRadius;

	/* How far into the Destroyed animation the anim notify OnZeroHealthAnimationFinished is. 
	This can be 0 if the anim notify is not present */
	UPROPERTY()
	float TimeIntoZeroHealthAnimThatAnimNotifyIs;

	/* How many persistent queues this building has (currently limited to 1.) If greater 
	than 0 then this building is a construction yard type building */
	UPROPERTY()
	int32 NumPersistentQueues;

	/* Buildings that can build this building. Does not take into account prereqs */
	UPROPERTY()
	TArray <EBuildingType> TechTreeParentBuildings;

	/* Units that can build this building. Does not take into account prereqs */
	UPROPERTY()
	TArray <EUnitType> TechTreeParentUnits;

	/* If true then this building can train at least one unit that is an army/attacking type 
	unit. If this building only trains collectors/workers then it is not considered a barracks */
	uint8 bIsBarracksType : 1;

	uint8 bIsBaseDefenseType : 1;

public:

	FBuildingInfo();

	void AddTechTreeParent(EBuildingType BuildingThatCanBuildThis);
	void AddTechTreeParent(EUnitType UnitThatCanBuildThis);

	/* Some queries */

	/* True if persistent queue's capacity is at least 1 */
	bool IsAConstructionYardType() const;
	
	/* True if can train at least one unit that is an army type unit */
	bool IsABarracksType() const;

	bool IsABaseDefenseType() const;

	/* Getters and setters */

	const FSlateBrush & GetHighlightedHUDImage() const { return HighlightedImage; }

	TSubclassOf<AGhostBuilding> GetGhostBuildingBP() const { return GhostBuilding_BP; }

	float GetBoundsHeight() const { return BoundsHeight; }
	/* This is for figuring out what collides with ghost when placing it */
	const FVector & GetScaledBoundsExtent() const { return ScaledBoxExtent; }

	EBuildingBuildMethod GetBuildingBuildMethod() const { return BuildingBuildMethod; }
	bool ShouldBuildingRiseFromGround() const { return bBuildingRisesFromGround; }
	float GetBuildProximityRange() const { return BuildProximityDistance; }
	float GetFoundationRadius() const { return FoundationRadius; }
	int32 GetNumPersistentQueues() const { return NumPersistentQueues; }
	float GetTimeIntoZeroHealthAnimThatAnimNotifyIs() const { return TimeIntoZeroHealthAnimThatAnimNotifyIs; }

	void SetHUDHighlightedImage(const FSlateBrush & InBrush) { HighlightedImage = InBrush; }
	void SetGhostBuildingBP(const TSubclassOf < AGhostBuilding > & InBP) { GhostBuilding_BP = InBP; }
	void SetBoundsHeight(float InHeight) { BoundsHeight = InHeight; }
	void SetScaledBoxExtent(const FVector & InExtent) { ScaledBoxExtent = InExtent; }
	void SetBuildingBuildMethod(EBuildingBuildMethod InBuildMethod) { BuildingBuildMethod = InBuildMethod; }
	void SetBuildingRisesFromGround(bool bInValue) { bBuildingRisesFromGround = bInValue; }
	void SetBuildProximityDistance(float InDistance) { BuildProximityDistance = InDistance; }
	void SetFoundationRadius(float InValue) { FoundationRadius = InValue; }
	void SetNumPersistentQueues(int32 InValue) { NumPersistentQueues = InValue; }
	void SetIsBarracksType(bool bInValue) { bIsBarracksType = bInValue; }
	void SetIsBaseDefenseType(bool bInValue) { bIsBaseDefenseType = bInValue; }
	void SetTimeIntoZeroHealthAnimThatAnimNotifyIs(float InValue) { TimeIntoZeroHealthAnimThatAnimNotifyIs = InValue; }

	// Tech tree getters. Maybe these could be moved off here and onto faction info

	/* Return whether any construction yard buildings can build this (or if any are on 
	faction's building roster). Also checks that the build method is an acceptable construction 
	yard build method (either BuildsInTab or BuildsItself) */
	bool CanBeBuiltByConstructionYard() const;

	bool IsBuildMethodSupportedByConstructionYards() const;

	/* Return whether at least one unit on the faction's unit roster can build this. Also checks 
	that the build method is supported for unit building */
	bool CanBeBuiltByWorkers() const;

	bool IsBuildMethodSupportedByUnits() const;

	/* Returns a list of all buildings that can build this building i.e. construction yard 
	type buildings */
	const TArray < EBuildingType > & GetTechTreeParentBuildings() const;

	/* Returns a list of all the units can can build this building */
	const TArray < EUnitType > & GetTechTreeParentUnits() const;

	/* Returns a random building that can build this i.e. a construction yard. Returns 
	EBuildingType::NotBuilding if no construction yards for faction */
	EBuildingType Random_GetTechTreeParentBuilding() const;

	/* Returns a random unit that can build this. Returns None if no unit can build this */
	EUnitType Random_GetTechTreeParentUnit() const;
};


/* Info about a unit */
USTRUCT()
struct FUnitInfo : public FBuildInfo
{
	GENERATED_BODY()

protected:

	/* Should definently be moved closer to the regular costs array for better CPU cache locality */
	UPROPERTY()
	TArray <int16> HousingCosts;

	//==========================================================================================
	//	Damage Info
	//==========================================================================================
	//	These values were added so that we can easily query FAttackAttributes and get the damage 
	//	without having to look at a projectile and so upgrades can upgrade damage without 
	//	having to use an extra DamageMultiplier. They are not in a state right now where they 
	//	can be used to display values on the HUD e.g. cannot show a "Damage" value for a tooltip 
	//	when choosing whether to produce this unit or not because it will not be updated when 
	//	upgrades happen. I guess you could show it if you don't want it to be modified by 
	//	upgrades.
	//
	//	To get these values in a state where they can be shown on the HUD upgrades would need
	//	to also update this struct. Then when ISelectable::Setup() runs instead of applying 
	//	all completed upgrades the way we are currently doing it we would instead query this 
	//	struct for all the damage, defense etc values we need and apply them from here. 
	//	Kind of tough though because some upgrades may have cosmetic effects, and we can't 
	//	really store them here on this struct... well we could but I think having a pointer 
	//	directly to the actual infantry is better 
	//------------------------------------------------------------------------------------------

	// Damage dealt to what the projectile impacts
	float ImpactDamage;
	TSubclassOf<URTSDamageType> ImpactDamageType;
	float ImpactRandomDamageFactor;

	// AoE damage the projectile deals
	float AoEDamage;
	TSubclassOf<URTSDamageType> AoEDamageType;
	float AoERandomDamageFactor;

	//==========================================================================================
	//	Other Info
	//==========================================================================================

	/* Buildings that can train this unit. Does not consider prereqs */
	UPROPERTY()
	TArray<EBuildingType> TechTreeParents;
 
	uint8 bIsCollectorType : 1;
	uint8 bIsWorkerType : 1;
	uint8 bIsArmyType : 1;

public:

	void SetupHousingCostsArray(const TMap < EHousingResourceType, int16 > & InCostMap);

	const TArray <int16> & GetHousingCosts() const;

	float GetImpactDamage() const;
	TSubclassOf <URTSDamageType> GetImpactDamageType() const;
	float GetImpactRandomDamageFactor() const;
	float GetAoEDamage() const;
	TSubclassOf <URTSDamageType> GetAoEDamageType() const;
	float GetAoERandomDamageFactor() const;

	/* Set what the damage values are */
	void SetDamageValues(UWorld * World, const FAttackAttributes & AttackAttributes);

	void AddTechTreeParent(EBuildingType Building);

	void SetIsACollectorType(bool bInValue);
	void SetIsAWorkerType(bool bInValue);
	void SetIsAAttackingType(bool bInValue);

	/* Return whether the faction can actually train this unit. A false could mean the unit 
	is only available as a starting selectable */
	bool IsTrainable() const;

	/* Returns true if unit can gather at least one type of resource */
	bool IsACollectorType() const;
	
	/* Returns true if unit can build at least one building */
	bool IsAWorkerType() const;
	
	/* Returns true if unit is neither a collector type or a worker type */
	bool IsAArmyUnitType() const;

	/* Get the buildings that are capable of building this (given prereqs are fulfilled too) */
	const TArray<EBuildingType> & GetTechTreeParents() const;

	/* Get a random building that can build this unit. Returns EBuildingType::NotBuilding if 
	no building can build this unit. */
	EBuildingType Random_GetTechTreeParent() const;
};


/* Struct that holds the state of a buff/debuff that does not tick, so just the instigator */
USTRUCT()
struct FStaticBuffOrDebuffInstanceInfo
{
	GENERATED_BODY()

protected:

	/* Pointer to the info struct this buff/debuff is for */
	const FStaticBuffOrDebuffInfo * Info;

	/* Type of buff/debuff this is for. Should always be a copy of Info->GetSpecificType(). 
	Could move this variable off of here and just query the info struct instead */
	EStaticBuffAndDebuffType SpecificType;

	/* The actor that applied the buff/debuff to us. Depending on how buff/deuff logic goes
	this could be anything but is usually a selectable. Can also be null */
	TWeakObjectPtr < AActor > Instigator;

	/* Instigator as an ISelectable. Can be null */
	ISelectable * InstigatorAsSelectable;

public:

	/* Only here for compile. Never call this */
	FStaticBuffOrDebuffInstanceInfo();

	explicit FStaticBuffOrDebuffInstanceInfo(const FStaticBuffOrDebuffInfo * InInfo,
		AActor * InInstigator, ISelectable * InInstigatorAsSelectable);

	/* Get the actor that instigated the buff/debuff */
	AActor * GetInstigator() const;

	/* Shouldn't call this without first checking if Instigator is valid */
	ISelectable * GetInstigatorAsSelectable() const;

	/* Get the info struct that this buff/debuff instance is for */
	const FStaticBuffOrDebuffInfo * GetInfoStruct() const;

	/* Get the type of buff/debuff this struct is for */
	EStaticBuffAndDebuffType GetSpecificType() const;

	/* For TSet */
	friend bool operator==(const FStaticBuffOrDebuffInstanceInfo & Info1, const FStaticBuffOrDebuffInstanceInfo & Info2)
	{
		return Info1.GetInfoStruct() == Info2.GetInfoStruct();
	}

	/* Hash function for TSet */
	friend uint32 GetTypeHash(const FStaticBuffOrDebuffInstanceInfo & Info)
	{
		return GetTypeHash(Info.GetInfoStruct());
	}
};


/* Struct that holds the state of a buff/debuff that ticks */
USTRUCT()
struct FTickableBuffOrDebuffInstanceInfo
{
	GENERATED_BODY()

protected:

	/* Pointer to the info struct this buff/debuff is for */
	const FTickableBuffOrDebuffInfo * Info;

	/* Type of buff/debuff this is for. Should always be a copy of Info->GetSpecificType().
	Could move this variable off of here and just query the info struct instead */
	ETickableBuffAndDebuffType SpecificType;

	/* How many ticks have happened. If this is a buff/debuff that never expires then this 
	isn't reliable due to overflow (255 max) */
	uint8 TickCount;

	float TimeRemainingTillNextTick;

	/* The actor that applied the buff/debuff to us. Depending on how buff/deuff logic goes
	this could be anything but is usually a selectable. Can also be null */
	TWeakObjectPtr < AActor > Instigator;

	/* Instigator as an ISelectable. Can be null */
	ISelectable * InstigatorAsSelectable;

public:

	/* Only here for compile. Never call this */
	FTickableBuffOrDebuffInstanceInfo();

	/* Ctor will act as if buff/debuff is being freshly applied. Should set NumTicks to 0 and
	set TimeRemainingTillNextTick to proper value */
	explicit FTickableBuffOrDebuffInstanceInfo(const FTickableBuffOrDebuffInfo * InInfo,
		AActor * InInstigator, ISelectable * InInstigatorAsSelectable);

	/* Reset the duration back to full */
	void ResetDuration();

	/* Get how many ticks have happened. Calling this during the DoTick function will return
	1 more than how many tick effects have actually been applied i.e. it will be at least 1 */
	uint8 GetTickCount() const;

	AActor * GetInstigator() const;

	/* Shouldn't call this without first checking if Instigator is valid */
	ISelectable * GetInstigatorAsSelectable() const;

	/* Get the info struct that this buff/debuff instance is for */
	const FTickableBuffOrDebuffInfo * GetInfoStruct() const;

	ETickableBuffAndDebuffType GetSpecificType() const;

	//=======================================================================================
	//	Functions for tick

	void DecrementDeltaTime(float DeltaTime);
	float GetTimeRemainingTillNextTick() const;
	void IncreaseTimeRemainingTillNextTick(float Amount);
	void IncrementTickCount();

	/* Get how long the buff/debuff has left before it will fall off */
	float CalculateDurationRemaining() const;
};


/* Work around for TArrays not being able to be 2 dimensional */
USTRUCT()
struct FPlayerStateArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray <ARTSPlayerState *> PlayerStates;

	void AddToArray(ARTSPlayerState * PlayerState);

	/* Getters and setters */

	const TArray <ARTSPlayerState *> & GetPlayerStates() const;
};


/* Array of different information about particle systems to spawn on selectables */
USTRUCT()
struct FVaryingSizeParticleArray
{
	GENERATED_BODY()

protected:

	/* Array of different templates to use for different size selectables.
	@See FSelectableAttributes::ParticleSize to set which template a selectable will use */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray < UParticleSystem * > Particles;

public:

	FVaryingSizeParticleArray()
	{
		/* Add on entry so default index 0 will be valid */
		Particles.Emplace(nullptr);
	}

	/* Given a selectable's size get the right template for it */
	UParticleSystem * GetTemplate(int32 SelectablesSize) const { return Particles[SelectablesSize]; }
};


/* Info required to spawn a particle system */
USTRUCT()
struct FParticleInfo
{
	GENERATED_BODY()

protected:

	/* Template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * Template;

	/* Optional transform adjustment TODO make it a FVector, FRotator and a float to reduce 
	byte requirement. The float is the scale */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTransform Transform;

public:

	FParticleInfo();

	UParticleSystem * GetTemplate() const;
	const FTransform & GetTransform() const;
};


/* Info for a particle system that attaches to a bone */
USTRUCT()
struct FParticleInfo_Attach
{
	GENERATED_BODY()

protected:

	/* Template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * Template;

	/* Where on the selectable the particles should try attach to */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ESelectableBodySocket AttachPoint;

public:

	FParticleInfo_Attach()
		: Template(nullptr)
		, AttachPoint(ESelectableBodySocket::None)
	{
	}

	UParticleSystem * GetParticles() const { return Template; }
	ESelectableBodySocket GetAttachPoint() const { return AttachPoint; }
};


/* Entry in array of particles that are attached to an actor */
USTRUCT()
struct FAttachedParticleInfo
{
	GENERATED_BODY()

public:

	// Default constructor. Never call this
	FAttachedParticleInfo();

	explicit FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
	explicit FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);
	explicit FAttachedParticleInfo(UParticleSystemComponent * InPSC, const FContextButtonInfo & AbilityInfo, uint32 Index);

	/* These constructors are here for when we need to find the entry in the array. This is bad 
	though. I should use perhaps TArray::FindByPredicate(uint32) or something instead of needlessly 
	creating a pointer that will never be used (compiler might optimize it out anyway) */
	explicit FAttachedParticleInfo(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
	explicit FAttachedParticleInfo(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);
	explicit FAttachedParticleInfo(const FContextButtonInfo & AbilityInfo, uint32 Index);

	UParticleSystemComponent * GetPSC() const { return PSC.Get(); }

protected:

	TWeakObjectPtr<UParticleSystemComponent> PSC;
	uint32 UniqueID;

	/* Generate a ID so when the buff/debuff/ability wants to remove its particles from the
	array we know which entry it is */
	static uint32 GenerateUniqueID(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
	static uint32 GenerateUniqueID(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);
	/* @param Index - some abilities might have more than 1 particle effect that attaches 
	to the target. Pass in different values for each one e.g. 0 for the "flash" particle 
	effect, 1 for something different. But there are a maximum of 8 particles that can 
	attach to target, so range: [0, 7] */
	static uint32 GenerateUniqueID(const FContextButtonInfo & AbilityInfo, uint32 Index);

	friend bool operator==(const FAttachedParticleInfo & Info_1, const FAttachedParticleInfo & Info_2)
	{
		return Info_1.UniqueID == Info_2.UniqueID;
	}
};


/* Info required to spawn a decal that fades out over time */
USTRUCT()
struct FDecalInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UMaterialInterface * Decal;

	/* How long decal should appear for. This should probably not be specific to each decal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.01"))
	float InitialDuration;

	/* How long fade lasts for */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float FadeDuration;

public:

	FDecalInfo();

	UMaterialInterface * GetDecal() const;
	float GetInitialDuration() const;
	float GetFadeDuration() const;
};


/* Information about scene components to attach to when using abilities. The references to the
components are set in post edit */
USTRUCT()
struct FAttachInfo
{
	GENERATED_BODY()

protected:

	/**	Contains a field to type in the FName of the component to use. From my experience
	 *	testing this the FName is usually the name you see in the editor in the components tab minus
	 *	the (inherited) part. If the name does not match then the root component will be used.
	 *	If effects should not rotate when the target rotates then the component should have
	 *	bAbsoluteRotation = true. Since rotation is more specific to the ability being used you
	 *	may need to define extra ESelectableBodySocket like "FloorNoRotate" and set different
	 *	components. The selection decals components are good to use for an absolute rotation
	 *	component 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName ComponentName;

	/* Component as a UPROPERTY pointer and not a TWeakObjectPtr because the latter will be
	null in PIE */
	UPROPERTY()
	USceneComponent * SceneComponent;

	/* Optional socket/bone name on component to attach to */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName SocketName;

	/* Attach transform if required */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTransform AttachTransform;

public:

	FAttachInfo();

	USceneComponent * GetComponent() const;
	const FName & GetSocketName() const;
	const FTransform & GetAttachTransform() const;

#if WITH_EDITOR
	void OnPostEdit(AActor * InOwningActor);
#endif
};


/* Struct sent over wire for updating animation state */
USTRUCT()
struct FAnimationRepVariable
{
	GENERATED_BODY()

	/* Type of animation */
	UPROPERTY()
	EAnimation AnimationType;

	/* World time when the animation started */
	UPROPERTY()
	float StartTime;
};


/** 
 *	Information about an item that goes in an inventory. 
 *	
 *	My notes: we cannot allow usable items to stack. I *think* I have taken care of that in post 
 *	edit.
 *	Ok I'm going to relax that a bit and see how it goes. We'll allow stackable items to be usable 
 *	as long as they only have 1 use AND they are destroyed when they are used, just like clarity 
 *	potions.
 *
 *	I'm not too sure if how I have setup EditConditions gets what I want, but the goal is 
 *	- if an item is usable and it can stack then it should only have 1 charge
 *
 *	I'm doing a lot of uint8, int16 and int32 conversions in this struct. Try and change this
 */
USTRUCT()
struct FInventoryItemInfo
{
	GENERATED_BODY()

protected:

	//==========================================================================================
	//	Stuff displayed on HUD and sounds
	//==========================================================================================

	/* The name to display for this item */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText DisplayName;

	/* The image to display for this item */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush DisplayImage;

	/* Image to display when a button for this item is hovered */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush DisplayImage_Hovered;

	/* Image to display when a button for this when item is pressed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush DisplayImage_Pressed;

	/** 
	 *	Image to display when a button for this when item is highlighted. An example of 
	 *	when this might happen is if the item has a use ability that requires selecting a target. 
	 *	While selecting the target the button will be highlighted 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush DisplayImage_Highlighted;

	/* Sound to play when a button for this item is hovered */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound_Hovered;

	/* Sound to play when a button for this item is pressed by LMB */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound_PressedByLMB;

	/* Sound to play when a button for this item is pressed by RMB */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * Sound_PressedByRMB;

	/* A description of this item */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Description;

	/** 
	 *	Some more text that can be used to say what the stat effects of the item are. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (MultiLine = true))
	FText StatChangeText;

	/** 
	 *	Flavour text. Kept seperate from the description because some people like this text to 
	 *	be in italics, different color, etc. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText FlavorText;

	/* The sound to play when this item enters an inventory */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * AquireSound;

	/** 
	 *	Particles to play when item is aquired. They will most likely be attached to the unit
	 *	that aquires the item. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FParticleInfo_Attach AquireParticles;

	/** 
	 *	Optional animation to play when the item is dropped. Montage should have a section at
	 *	the end that loops idle, and blend in time should be set to 0.
	 *
	 *	Quick note: the gun in the first person shooter example doesn't have any animations and it 
	 *	doesn't have a T-pose from the looks of it. Perhaps looping idle at the end isn't required.
	 *	This is only relevant if MeshInfo's mesh is a skeletal mesh. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UAnimMontage * OnDroppedAnim;

	/* The mesh to use to display this item when it has a presence in the world. 
	
	I *think* setting location to != ZeroVector will actually change the item actor's 
	GetActorLocation(), which is not really what I want. Solution would be to use a shape comp 
	as AInventoryItem's root comp just like I have done with infantry/buildings. This is all 
	assuming though that moving the root component implies moving the actor. If there's a way 
	to not do that then I do not need the shape comp as root. But I do not think there is. 
	
	So bottom line if users use an offset here then the actor location will not be on the ground. 
	For small offsets this doesn't matter too much I guess, just IsInRangeOfItem calcs will be 
	slightly off */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FMeshInfoBasic MeshInfo;

	//==========================================================================================
	//	Behavior stuff
	//==========================================================================================

#if WITH_EDITORONLY_DATA
	/** 
	 *	How much this item costs. This can be overridden on a per shop basis... if I actually 
	 *	implement it 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EResourceType, int32 > DefaultCost;
#endif

	/* Elements of TMap transfered to an array for performance */
	UPROPERTY()
	TArray < int32 > DefaultCostArray;

	/** 
	 *	The maximum number of stacks of this item we can have in our inventory.
	 *	
	 *	0 = unlimited. 
	 *
	 *	If you would like a unique type of item then you would set this to 1.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	int32 StackQuantityLimit;

	/** 
	 *	The maximum amount of this item there can be in a stack e.g. in dota clarity potions can 
	 *	stack like I don't know perhaps 99 times, but a claymore you can only have 1 of in a stack. 
	 *	So if you pickup another claymore it will be in its own inventory slot.
	 *	
	 *	0 = unlimited.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditMaxNumPerStack))
	int32 MaxNumPerStack;

	// Some function pointer typedefs 
	typedef void (*InventoryItem_OnAquired)(ISelectable *, EItemAquireReason);
	typedef void (*InventoryItem_OnRemoved)(ISelectable *, EItemRemovalReason);
	// @param int16 - how many charges the slot changed by. Can be positive or negative
	typedef void (*InventoryItem_OnNumChargesChanged)(ISelectable *, FInventory *, FInventorySlotState &, int16, EItemChangesNumChargesReason);

	/** 
	 *	Function pointer for behavior when item is aquired. This can be left null in which 
	 *	case it is assumed "do nothing" is the desired behavior. 
	 *	
	 *	A good place to assign this pointer is in URTSGameInstance::InitInventoryItemInfo() 
	 */
	InventoryItem_OnAquired OnAquiredFunctionPtr;

	/** 
	 *	Function pointer for behavior when item is removed. Just like OnAquired this can be left 
	 *	null and it will be assumed "do nothing" is the desired behavior 
	 *	
	 *	A good place to assign this pointer is in URTSGameInstance::InitInventoryItemInfo() 
	 */
	InventoryItem_OnRemoved OnRemovedFunctionPtr;

	/** 
	 *	The item type this info struct is for. 
	 *	For a long time I kept this off this struct, so there may be parts of code getting the 
	 *	type in a less efficient way than just querying this struct. Lots of functions have 
	 *	params that are both the item type + this struct so they can get rid of the item type 
	 *	param 
	 */
	EInventoryItem ItemType;

	/**
	 *	If true then this item drops onto the world when the unit carrying it reaches zero health. 
	 *	
	 *	True = divine rapier, gem of true sight 
	 *	False = most other items in dota
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bDropsOnDeath : 1;

	/** 
	 *	Whether the item can be explicity dropped. 
	 *	
	 *	True = most items in dota
	 *	False = divine rapier I think 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bCanBeExplicitlyDropped : 1;

	/** 
	 *	Whether the item can be sold back to a shop or something. 
	 *	
	 *	True = most items in dota
	 *	False = divine rapier I think 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bCanBeSold : 1;

	/** 
	 *	If true then only a certain list of unit types are allowed to:
	 *	- pick up this item off the ground 
	 *	- purchase this item 
	 *
	 *	It may still be possible for the item to enter their inventory in other ways. 
	 *	Also this will not be checked when creating combination items.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bRestrictedToCertainTypes : 1;

	/** 
	 *	What unit types are allowed to pick up this item. Ignored if bRestrictedToCertainTypes 
	 *	is false. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bRestrictedToCertainTypes))
	TSet <EUnitType> TypesAllowedToPickUp;

	//===========================================================================================
	//	Creation using multiple other items
	//===========================================================================================

	/**
	 *	If all these items in this array are present in an inventory then this item will be created.
	 *	Key = ingredient, value = quantity of it required.
	 *
	 *	Note that it takes at least two items in this TMap for it to be valid. You could either 
	 *	have two of the same item or 2 different items but if there is only one key/value 
	 *	pair and its value is 1 then this TMap will be cleared during setup. 
	 *	@See URTSGameInstance::InitInventoryItemInfo
	 *
	 *	e.g. if this struct is for divine rapier then claymore, demon edge and sacred relic would
	 *	go in this map, and their values would each be 1.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Ingredients"))
	TMap < EInventoryItem, FAtLeastOneInt16 > IngredientsTMap;

	/* The oppisite of Ingredients. This array holds what this item can be used to make. */
	UPROPERTY()
	TArray < EInventoryItem > WhatThisCanMake;

	//===========================================================================================
	//	Item Use
	//===========================================================================================

	/** 
	 *	Whether this item is usable.
	 *	
	 *	True = Diffusal blade, clarity potion 
	 *	False = claymore 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditbIsUsable))
	bool bIsUsable;

	/* The ability to use */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bIsUsable))
	EContextButton UseAbilityType;

	/* Pointer to the use ability info struct. If this item has no use ability then it will 
	be null */
	const FContextButtonInfo * UseAbilityInfo;

	/** 
	 *	How many times this item can be used. 
	 *	
	 *	-1 = unlimited. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditNumCharges))
	int32 NumCharges;

	/** 
	 *	What happens when this item changes its number of charges. 
	 *	
	 *	If you choose CustomBehavior then make sure to set the function pointer, which is done in
	 *	GI::InitInventoryItemInfo. Otherwise behavior will default to Remove. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditZeroChargeBehavior))
	EInventoryItemNumChargesChangedBehavior NumChargesChangedBehavior;

	/* Optional behavior when number of charges in a slot with this item changes. Will only be 
	called if ZeroChargeBehavior == CustomBehavior */
	InventoryItem_OnNumChargesChanged OnNumChargesChangedFunctionPtr;

	//===========================================================================================

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditMaxNumPerStack;

	UPROPERTY()
	bool bCanEditNumCharges;

	UPROPERTY()
	bool bCanEditZeroChargeBehavior;
#endif

public:

	//=========================================================================================
	//	Constructors and Setup Functions
	//=========================================================================================

	FInventoryItemInfo();

	/** 
	 *	This ctor sets the display name. It is intended to be called only during the setup 
	 *	process. 
	 *	Because of how the EditConditions are set up it is no advisable to change values in this 
	 *	ctor. 
	 */
	explicit FInventoryItemInfo(const FString & InDisplayName);

	void SetItemType(EInventoryItem InItemType) { ItemType = InItemType; }

	/* Called by GI during setup if the Ingredients TMap is not usable. Removes all entries 
	from Ingredients */
	void ClearIngredients();

	/* Say what this item is an ingredient of */
	void AddCombinationResult(EInventoryItem InItemType);

	/** 
	 *	Sets what the behavior is for when the item is aquired, removed and optionally when 
	 *	it reaches zero charges. 
	 *
	 *	@param OnAquiredPtr - function that is called when the item is aquired.
	 *	@param OnRemovedPtr - function that is called when the item is removed.
	 *	@param OnZeroChargesReachedPtr - function that is called when the item reaches zero charges 
	 *	if it is an item that has charges and the user has specified that they want custom behavior 
	 *	for when it reaches zero charges by setting FInventoryItemInfo::ZeroChargeBehavior to 
	 *	"CustomBehavior".
	 */
	void SetBehaviorFunctionPointers(InventoryItem_OnAquired OnAquiredPtr, InventoryItem_OnRemoved OnRemovedPtr,
		InventoryItem_OnNumChargesChanged OnNumChargesChangedPtr = nullptr);

	void SetUseAbilityInfo(const FContextButtonInfo * AbilityInfo);

	//====================================================
	//	Behavior Functions
	//====================================================

	/**
	 *	Run the logic for when the item is aquired. Often this will adjust stats and stuff.
	 *	
	 *	@param Owner - the selectable that is aquiring this item 
	 *	@param Quantity - number of times to run logic
	 *	@param AquireReason - the way this item was aquired 
	 */
	void RunOnAquiredLogic(ISelectable * Owner, int32 Quantity, EItemAquireReason AquireReason) const;
	
	/** 
	 *	Run the logic for when the item is removed  from inventory. Often this will reverse the 
	 *	changes that were made when it was added to inventory.
	 *
	 *	@param Owner - selectable that is losing the item 
	 *	@param Quantity - how many of the item the selectable is losing 
	 *	@param RemovalReason - how the item(s) is being removed
	 */
	void RunOnRemovedLogic(ISelectable * Owner, int32 Quantity, EItemRemovalReason RemovalReason) const;

	/** 
	 *	Run the logic for what should happen when the item's number of charges changes.
	 *
	 *	@param Owner - selectable that owns the item
	 *	@param Inventory - inventory of Owner as a convenience
	 *	@param InvSlot - inventory slot that changed
	 *	@param ChangeReason - why the number of charges changed
	 *
	 *	My notes: have implemented this but currently do not call it anywhere
	 */
	void RunCustomOnNumChargesChangedLogic(ISelectable * Owner, FInventory * Inventory, FInventorySlotState & InvSlot, 
		int16 ChangeAmount, EItemChangesNumChargesReason ChangeReason) const;

	//====================================================
	//	Trivial Getters
	//====================================================

	/* Get the item type this struct holds data for */
	EInventoryItem GetItemType() const { return ItemType; }

	const FText & GetDisplayName() const;
	const FText & GetDescription() const;
	const FText & GetStatChangeText() const;
	const FText & GetFlavorText() const;
	const FSlateBrush & GetDisplayImage() const;
	const FSlateBrush & GetDisplayImage_Hovered() const;
	const FSlateBrush & GetDisplayImage_Pressed() const;
	const FSlateBrush & GetDisplayImage_Highlighted() const;
	USoundBase * GetUISound_Hovered() const;
	USoundBase * GetUISound_PressedByLMB() const;
	USoundBase * GetUISound_PressedByRMB() const;
	USoundBase * GetAquireSound() const;
	const FParticleInfo_Attach & GetAquireParticles() const;
	UAnimMontage * GetOnDroppedAnim() const;
	const FMeshInfoBasic & GetMeshInfo() const;

	const TArray < int32 > & GetDefaultCost() const;

	/* Return "None" if the selectable's type if allowed to pick up the item */
	EGameWarning IsSelectablesTypeAcceptable(ISelectable * Selectable) const;

	/* Return whether multiple of this item can be stacked on top of each other in inventory */
	bool CanStack() const;

	/* Get how many stacks of this item we are allowed */
	int32 GetNumStacksLimit() const;

	/* Get how many of this item can be in a stack */
	int32 GetStackSize() const;

	bool HasNumberOfStacksLimit() const;
	bool HasNumberInStackLimit() const;

	bool DropsOnDeath() const { return bDropsOnDeath; }
	bool CanBeSold() const { return bCanBeSold; }

	EInventoryItemNumChargesChangedBehavior GetNumChargesChangedBehavior() const { return NumChargesChangedBehavior; }

	/* Whether this item can be used */
	bool IsUsable() const;

	/* This func is sometimes used at times to get the FContextButtonInfo ptr. Now that I have 
	added the UseAbilityInfo variable we can just call that instead */
	EContextButton GetUseAbilityType() const { return UseAbilityType; }
	
	const FContextButtonInfo * GetUseAbilityInfo() const { return UseAbilityInfo; }

	/** 
	 *	How many charges this item starts with e.g. diffusal blade = 10.  
	 */
	int32 GetNumCharges() const;

	/* Return whether the item can be used as many times as wanted. Assumes that you have checked 
	that the item is usable in the first place */
	bool HasUnlimitedNumberOfUsesChecked() const;
	bool HasUnlimitedNumberOfUses() const;

	/* Return reference to what is required to make this item. */
	const TMap < EInventoryItem, FAtLeastOneInt16 > & GetIngredients() const;

	/* Return what is required to make this item, but return by value */
	TMap < EInventoryItem, FAtLeastOneInt16 > GetIngredientsByValue() const;

	/** 
	 *	Get all the items that can be created from this. 
	 *	
	 *	e.g. if this is a claymore then something like, lothar's edge, divine rapier, etc 
	 *	would be what this returns. 
	 */
	const TArray < EInventoryItem > & GetItemsMadeFromThis() const;

#if WITH_EDITOR
	/* @param ThisItemsType - what this items type is. It is the key in the GI TMap */
	void OnPostEdit(EInventoryItem ThisItemsType);
#endif
};


/* Struct of a slot in a shop */
USTRUCT()
struct FItemOnDisplayInShopSlot
{
	GENERATED_BODY()

protected:

	/* The item this slot is selling */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EInventoryItem ItemOnDisplay;

	/* Whether the item is for sale. If not for sale then users cannot buy it but they can look */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bIsForSale;

	/**
	 *	How many purchases of this item are remaining in the store. 
	 *
	 *	-1 means unlimited quantity. 
	 *	Set this to != -1 to only allow a finite number of purchases of this item.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "-1", DisplayName = "Quantity"))
	int32 PurchasesRemaining;

	/** 
	 *	How many of this item are bought everytime you purchase this. e.g. if you want this shop 
	 *	to sell clarity potions in lots of 3 then this would be 3. 
	 *	
	 *	My notes: currently limited to being only 1, but can probably change this in future. 
	 *	I have actually written the TryPutItemInInventory code to be ok if quantity is 
	 *	greater than 1, so it should be all good to remove that ClampMax of 1
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int32 QuantityBoughtPerPurchase;

public:

	FItemOnDisplayInShopSlot();

	EInventoryItem GetItemType() const;
	
	/** 
	 *	Return whether this item is for sale or just for browsing 
	 *	
	 *	@return - true if it is possible to purchase the item, false if it is not 
	 */
	bool IsForSale() const;

	int32 GetQuantityBoughtPerPurchase() const;
	int32 GetPurchasesRemaining() const;
	
	/* If true then there is no limit on how many purchases from this slot can happen */
	bool HasUnlimitedPurchases() const;

	void OnPurchase();
};


/* Info about what inventory items a selectable can display and sell */
USTRUCT()
struct FShopInfo
{
	GENERATED_BODY()

protected:

	/* The items this selectable displays and optionally sells. Limit 256 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray < FItemOnDisplayInShopSlot > ItemsOnDisplay;

	/** 
	 *	The worst affiliation of who can see the items and shop here
	 *	
	 *	e.g. 
	 *	Owned = only we can shop here 
	 *	Allied = ourselves and people on our team can shop here
	 *	Hostile = ourselves and people on our team and enemies can shop here
	 *	
	 *	Things like Observer or Neutral may cause undesirable behavior. Who knows.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMostProperties))
	EAffiliation WhoCanShopHere;

	/** 
	 *	Whether items can be sold to this shop. Does not matter whether they were bought from
	 *	here or not.
	 *	This is irrelevant if the shop does not sell anything in the first place, but can be 
	 *	easily changed. @See FShopInfo::AcceptsRefunds() 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditMostProperties))
	bool bAcceptsRefunds;

	/** 
	 *	Whether this shop actually sells at least one of the items it has on display. Here as 
	 *	an optimization but can be derived from iterating ItemsOnDisplay and checking if at least one 
	 *	item is for sale.
	 *
	 *	I think there were some "shops" in dota that only had items for show but you could not buy 
	 *	from them. 
	 */
	UPROPERTY()
	bool bAtLeastOneItemOnDisplayIsSold;

	/** 
	 *	How close a selectable has to be to this shop to be able to shop from it. Measured by 
	 *	doing a capsule sweep from GetActorLocation of shop.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditMostProperties))
	float ShoppingRange;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditMostProperties;
#endif // WITH_EDITORONLY_DATA

public:

	FShopInfo();

	/* Get the items we display and optionally sell */
	const TArray < FItemOnDisplayInShopSlot > & GetItems() const;

	/* Whether at least one item that is for display is also sold */
	bool SellsAtLeastOneItem() const;

	/* Whether items can be sold to this shop. Doesn't matter whether they were bought from 
	here or not */
	bool AcceptsRefunds() const;

	/* Get item on display and/or for sale at a particular index */
	const FItemOnDisplayInShopSlot & GetSlotInfo(int32 Index) const;

	/* Given an affiliation return whether they are allowed to shop here */
	bool CanShopHere(EAffiliation Affiliation) const;

	/**
	 *	Get all selectables in range of shop on a particular team. 
	 */
	void GetSelectablesInRangeOfShopOnTeam(UWorld * World, ARTSGameState * GameState, 
		TArray < FHitResult > & OutHits, const FVector & ShopLocation, ETeam Team) const;

	/** 
	 *	Get the location we consider the shop at. Use when checking what is in range to shop from here. 
	 *	
	 *	@param Owner - the selectable that this shop info belongs to 
	 */
	static FVector GetShopLocation(AActor * Owner)
	{
		return Owner->GetActorLocation();
	}

	/** 
	 *	Called when an item is purchased from this shop 
	 *	
	 *	@param SlotIndex - index of slot that purchase was made 
	 */
	void OnPurchase(int32 SlotIndex);

#if WITH_EDITOR
	void OnPostEdit(ISelectable * OwningSelectable);
#endif
};


/* Information about a single slot in a selectable's inventory */
USTRUCT()
struct FInventorySlotState
{
	GENERATED_BODY()

protected:

	/* The item in this slot. If "None" then there is no item in this slot */
	EInventoryItem ItemType;

	/* Flags about the item. Horay we crammed this whole struct into 4 bytes... well until I 
	needed to add a timer handle 
	In future I may want to add more bools here */
	bool bCanItemStack : 1;
	bool bItemDropsOnDeath : 1;
	bool bIsItemUsable : 1;
	bool bHasUnlimitedCharges : 1;

	/* This is the number of this item in this stack. Or the number of charges of this item. 
	Because we do not allow an item that has more than 1 charge to stack we can do this. 
	Probably should make sure that if effects reduce the number of charges on the item in 
	this slot then we make sure to never let this go lower than 0 if it was previously equal 
	to or greater than 0, since we use -1 to mean 'unlimited number of charges' */
	int16 NumInStackOrNumCharges;

	/* Timer that keeps track of the cooldown of the item's use ability */
	FTimerHandle TimerHandle_ItemUseCooldown;

public:

	// Never call this ctor
	FInventorySlotState();

	// DEPRECIATED. Param is useless right now. Can use paramless version
	FInventorySlotState(uint8 InStartingIndex);

	//=========================================================================================
	//	Trivial Getters
	//=========================================================================================

	/* Get the item that is in this slot, or None if there isn't an item in this slot */
	EInventoryItem GetItemType() const;

	/* Return true if there is an item in this slot */
	bool HasItem() const;

	bool CanItemStack() const;
	bool DropsOnZeroHealth() const;
	bool IsItemUsable() const;
	bool HasUnlimitedCharges() const;

	/* Get how many of the item is in the stack */
	int16 GetNumInStack() const;

	/**
	 *	Get how many charges the item has. 
	 *
	 *	I don't know if this is what it actually does, but I want this to return the total number 
	 *	of charges the stack has e.g. if it's a stack of 10 clarity potions then it will return 10.
	 */
	int16 GetNumCharges() const;

	/* This version also sets the flags and is what you should call most of the time */
	void PutItemIn(EInventoryItem InItem, int32 Quantity, const FInventoryItemInfo & ItemsInfo);

	/* Put Quantity of InItem into slot. This assumes that the slot was empty previously. 
	Does not modify any of the item flags */
	void PutItemInForReversal(EInventoryItem InItem, int32 Quantity);

	/* Assumes the stack is not emptied as a result of this */
	void AdjustAmount(int16 AmountToAdjustBy);

	/* Assumes that it is possible stack will be emptied as a result of this */
	void RemoveAmount(int16 AmountToRemove);

	/* Remove a charge from the item in this slot */
	void RemoveCharge();

	void SetItemTypeToNone();

	void ReduceQuantityToZero();

	FTimerHandle & GetItemUseCooldownTimerHandle() { return TimerHandle_ItemUseCooldown; }

	/* Whether the 'use' for the item in this slot is on cooldown. Probably need to add a 
	FTimerHandle to this struct to accomidate this */
	bool IsOnCooldown(const FTimerManager & WorldTimerManager) const;

	/* Get the cooldown remaining on the item use ability */
	float GetUseCooldownRemaining(FTimerManager & WorldTimerManager) const;

	/* Get cooldown remaining. Assumes we know the timer handle is pending */
	float GetUseCooldownRemainingChecked(const FTimerManager & WorldTimerManager) const;

	FString ToString() const;
};


/* Simple struct that records how much of an item type is in inventory */
USTRUCT()
struct FInventoryItemQuantity
{
	GENERATED_BODY()

protected:

	/* How many stacks, both partial and full, of this item are in inventory */
	uint8 NumStacks;

	/* How many of this item are in inventory */
	int16 Num;

public:

	// Here for code generation
	FInventoryItemQuantity();

	FInventoryItemQuantity(int16 InStackSize);

	/* Get how many stacks of item are in inventory. Stacks do not have to be full */
	uint8 GetNumStacks() const;
	
	/* Get how many of the item are in inventory */
	int16 GetTotalNum() const;

	void IncrementNumStacksAndAdjustNum(int16 AdjustAmount);

	/* Function assumes a stack was not created/deleted */
	void AdjustAmount(int16 AmountToAdjustBy);
	
	/** 
	 *	Called when a certain amount is removed and a stack is lost. 
	 *	
	 *	@param AdjustAmount - amount lost i.e. the amount that was in the stack 
	 */
	void DecrementNumStacksAndAdjustNum(int16 AmountLost);
};


/* Simple struct that contains item type and how many of it */
USTRUCT()
struct FInventoryItemQuantityPair
{
	GENERATED_BODY()

	EInventoryItem ItemType;
	int16 Quantity;

	// Never call this ctor
	FInventoryItemQuantityPair();

	FInventoryItemQuantityPair(EInventoryItem InItemType, int16 InQuantity)
		: ItemType(InItemType)
		, Quantity(InQuantity)
	{
	}
};


/** 
 *	Holds information about a selectable's inventory. 
 *	
 *	ServerIndex == SlotsArrayIndex
 *	LocalIndex == DisplayIndex
 *
 *------------------------------------------------------------------------------------------------
 *	Some info about how inventories behave: 
 *	
 *	Short explanation: Very similar to dota I think 
 *	
 *	Longer explanation: 
 *	- when a stackable item is added to inventory it will try and be placed on an already existing 
 *	stack of the same type if one exists. Otherwise it will go into an empty slot. 
 *
 *	- in regards to items combining to create other items: 
 *	The initial item must be added to a slot first before combination item creation can happen. 
 *	So if your inventory is full and you know that aquiring an item will cause items to combine 
 *	and your inventory to stay at/below capacity then you cannot aqurie that item, unless it can
 *	go in an empty slot or on top of another.
 *
 *	- Usable items with more than 1 charge are not allowed to stack. e.g. you could 
 *	not let diffusal blades stack. If you want to do similar behavour to tengu's you could 
 *	just allow the shop that sells them to sell them in lots of 3.
 *------------------------------------------------------------------------------------------------
 */
USTRUCT()
struct FInventory
{
	GENERATED_BODY()

protected:

#if WITH_EDITORONLY_DATA
	/* The max number of slots in the inventory e.g. in dota this is 6 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 Capacity;
#endif

	/* How many items this inventory is holding */
	UPROPERTY()
	int32 NumSlotsOccupied;

	/** 
	 *	Array of all the items in the inventory. The server keeps a copy of this and so do 
	 *	clients. The server and client versions are always exactly the same (after RPCs arrive 
	 *	of course). Reordering your inventory is achieved using the two arrays 
	 *	ServerIndexToLocalIndex and LocalIndexToServerIndex.
	 *	
	 *	My notes:
	 *	It may be possible to expose this to editor to allow users to have units start with items. 
	 *	We would need to make sure that the bonuses are applied on spawn but there's lots of other 
	 *	stuff that needs checking: 
	 *	- checking that we didn't already start with a recipe item e.g. if the user puts a
	 *	claymore, demon edge and scared relic in our inventory then we've basically started with a 
	 *	divine rapier and we would need to make sure the 3 items are turned into it. 
	 *	- checking uniquess is obeyed 
	 *	- checking stack size limit is obeyed 
	 */
	UPROPERTY()
	TArray < FInventorySlotState > SlotsArray;
	
	/* What are these two arrays? When we choose to modify the ordering of our items in our 
	inventory these arrays get used. By using these arrays we do not need to notify the server 
	we are changing item slots. This means no server RPC and makes switching item positions lag 
	free which is a good feel. 
	A downside to doing it like this is that no other player sees our inventory in the way 
	we have ordered it but I don't think this is a major issue */

	// Key = index in SlotsArray, value = server index. I think this is the array to query when showing stuff in HUD 
	UPROPERTY()
	TArray < uint8 > LocalIndexToServerIndex;
	
	UPROPERTY()
	TArray < uint8 > ServerIndexToLocalIndex;

	/** 
	 *	Maps item type to how many stacks of it there are in the inventory. Possibly no key/value 
	 *	pair means there are 0 stacks of that item. 
	 */
	UPROPERTY()
	TMap < EInventoryItem, FInventoryItemQuantity > Quantities;

	/** 
	 *	Returns true if this inventory cannot hold anymore stacks of an item type. Note 
	 *	though if the item can stack then we may be able to put it onto an already existing 
	 *	stack of its type.
	 *	
	 *	My notes: this was previously named AtQuantityLimit for a long time.
	 *
	 *	@param ItemType - item to check whether we are at its quantity limit or not 
	 *	@param Item - ItemType's info struct for convenience 
	 */
	bool AtNumStacksLimit(EInventoryItem ItemType, const FInventoryItemInfo & Item) const;

	/** 
	 *	For a item type check whether we cannot put anymore of it into inventory. Works for 
	 *	both stackable and non-stackable item types, although it is more efficient to call 
	 *	AtNumStacksLimit if we know the item is not stackable.
	 *	
	 *	@param ItemType - the item we're trying to add to inventory 
	 *	@param Quantity - how many of the item we're trying to add to inventory 
	 *	@param Item - ItemType's info struct for convenience
	 *	@return -  true if we cannot put anymore of Item into this inventory. May be possible 
	 *	for this to return true only to find later we could not add item to inventory.
	 */
	bool AtTotalNumLimit(EInventoryItem ItemType, int32 Quantity, const FInventoryItemInfo & Item) const;

	/** 
	 *	Run the logic for aquiring an item. This will include doing stuff like adjusting 
	 *	attributes if that's what the item does.
	 *
	 *	@param ItemAquired - the item aquired. 
	 *	@param Quantity - how many of the item was aquired. 
	 *	@param ItemsInfo - info struct of ItemAquired for convenience.
	 */
	void RunOnItemAquiredLogic(ISelectable * InventoryOwner, EInventoryItem ItemAquired, 
		int32 Quantity, EItemAquireReason AquireReason, const FInventoryItemInfo & ItemsInfo);

	/** 
	 *	Same as above but for when the item is removed. 
	 *	
	 *	@param Quantity - how many of the item was removed. 
	 *	@param RemovalReason - reason for why the item is being removed. 
	 *	@param ItemsInfo - info struct for ItemRemoved as a convenience.
	 */
	void RunOnItemRemovedLogic(ISelectable * InventoryOwner, EInventoryItem ItemRemoved, 
		int32 Quantity, EItemRemovalReason RemovalReason, const FInventoryItemInfo & ItemsInfo);

	/* Run the logic for when the number of charges of an item changes 
	
	@param ChargeChangeAmount - how much charges changed by. Can be positive or negative */
	void RunOnNumChargesChangedLogic(ISelectable * InventoryOwner, EInventoryItem ItemType,
		FInventorySlotState & Slot, int16 ChargeChangeAmount, EItemChangesNumChargesReason ReasonForChangingNumCharges,
		const FInventoryItemInfo & ItemsInfo, URTSHUD * HUDWidget);

public:

	FInventory();

	bool IsSlotIndexValid(uint8 Index) const { return SlotsArray.Num() > Index; }

	uint8 GetLocalIndexFromServerIndex(int32 ServerIndex) const;

	const FInventorySlotState & GetSlotGivenServerIndex(int32 Index) const { return SlotsArray[Index]; }

	/* Get the slot at a raw index e.g. if RawIndex = 1 but the player has moved around what 
	positions their buttons are at then this could return an entry in SlotsArray that is not 
	index 1 */
	const FInventorySlotState & GetSlotForDisplayAtIndex(int32 RawIndex) const;

	/* Get the maximum number of items we can hold */
	int32 GetCapacity() const;

	/* Get how many slots have an item in them */
	int32 GetNumSlotsOccupied() const;

	/* Return whether every slot in the inventory is occupied. It may still be possible to 
	receive more items though if they stack */
	bool AreAllSlotsOccupied() const;

	/* Struct returned by TryPutItemInInventory */
	struct TryAddItemResult
	{
		// "None" if successful, something else otherwise
		EGameWarning Result;

		/* Index in SlotsArray item was added at. Irrelevant if not successful */
		uint8 RawIndex;

		/* The last item added. Irrelevant if not successful. 
		If adding the item causes combination items to be made also then 
		this will return the last one made. Otherwise if no combination items are made as a result 
		of adding the item then this will just equal whatever the item was we added */
		EInventoryItem LastItemAdded;

		TryAddItemResult()
		{
		}

		TryAddItemResult(EGameWarning InResult, uint8 InRawIndex, EInventoryItem InLastItemAdded)
			: Result(InResult)
			, RawIndex(InRawIndex)
			, LastItemAdded(InLastItemAdded)
		{
		}
	};

	/** 
	 *	This function essentially does everything CanEnterInventory, 
	 *	PutNonUsableItemInSlotChecked/PutUsableItemInSlotChecked and CreateCombinationItems does. 
	 *	You should just use this if you're the server and you're trying to put an item into the 
	 *	inventory. If the full quantity of the item cannot be added then this will return as 
	 *	unsuccessful.
	 *
	 *	My notes: because this function exists the extra param version of CanEnterInventory and 
	 *	CreateCombinationItems can probably both be removed.
	 *	
	 *	@param InventoryOwner - the selectable this inventory belongs to
	 *	@param Item - item we're trying to add to inventory
	 *	@param Quantity - how many of Item we're trying to add
	 *	@param ItemsInfo - info struct of item for convenience
	 *	@param ReasonForAquiring - reason for trying to aquire item
	 *	@param GameInst - reference to game instance
	 */
	TryAddItemResult TryPutItemInInventory(ISelectable * InventoryOwner, bool bIsOwnerSelected, 
		bool bIsOwnerCurrentSelected, EInventoryItem Item, int32 Quantity, 
		const FInventoryItemInfo & ItemsInfo, EItemAquireReason ReasonForAquiring, 
		const URTSGameInstance * GameInst, URTSHUD * HUDWidget);

protected:

	/**
	 *	Will try to create a combination item. If successful then recusively calls itself to 
	 *	try and create more. Returns the last item that was created as a result of adding ItemJustAdded. 
	 *	If no combination items are created as a result of adding ItemJustAdded then 
	 *	ItemJustAdded will be returned. 
	 *	
	 *	@param InventoryOwner - the selectable this inventory belongs to
	 *	@param ItemJustAdded - the item we just added to inventory 
	 *	@param ItemsInfo - info struct of ItemJustAdded for convenience 
	 *	@param GameInst - game instance
	 */
	TryAddItemResult TryCreateCombinationItem(ISelectable * InventoryOwner, bool bIsOwnerSelected,
		bool bIsOwnerCurrentSelected, TryAddItemResult ItemJustAdded, const FInventoryItemInfo & ItemsInfo, 
		const URTSGameInstance * GameInst, URTSHUD * HUDWidget);

	/** 
	 *	Return whether all the ingredients for an item are present in the inventory, which means 
	 *	we can create it provided we will have enough space for it. 
	 *
	 *	@param Item - item we want to know whether we have all the ingredients for 
	 *	@param ItemsInfo - info struct for Item as a convenience
	 *	@return - true if all the ingredients are present in the inventory 
	 */
	bool AreAllIngredientsPresent(EInventoryItem Item, const FInventoryItemInfo & ItemsInfo) const;

	/* This only exists because whenever we change item position in inventory we only do it
	locally. Basically when an item is created it is put in the lowest array index of SlotsArray,
	but if we've swapped positions around then it's actually possible it will be not in the lowest
	display index. So we swap display indices so that it does appear at the lowest display index */
	void SwapJustCreatedCombinationItemIntoLowestDisplayIndex(int32 SlotsArrayIndexForCombinationItem, 
		ISelectable * InventoryOwner, bool bIsOwnerSelected, bool bIsOwnerCurrentSelected, URTSHUD * HUDWidget);

	/**
	 *	Swap the positions of two slots in inventory. Does it in a way that does not require 
	 *	sending an RPC to the server. Does not update HUD.
	 *	
	 *	This should be good to go for swapping positions with mouse. 
	 */
	void SwapSlotPositions_ServerIndicies(int32 ServerIndex_1, int32 ServerIndex_2);

	/** 
	 *	Version that takes local/display indices. The ServerIndices version and this both do 
	 *	the same thing. 
	 */
	void SwapSlotPositions_LocalIndicies(int32 LocalIndex_1, int32 LocalIndex_2);

public:

	/**
	 *	Get whether we can add a quantity of an item to inventory. Usually this is called by 
	 *	client before deciding whether to send RPC to server. If you want to actually add the 
	 *	item to inventory then use TryPutItemInInventory.
	 *	
	 *	@param Item - item we want to add
	 *	@param Qunatity - how many of the item we want to add 
	 *	@param ItemsInfo - info struct about item for convenience
	 *	@param InventoryOwner - selectable that owns this inventory
	 *	@return - struct with info about whether we can add the item or not. None means we can
	 */
	EGameWarning CanItemEnterInventory(EInventoryItem Item, int32 Quantity,
		const FInventoryItemInfo & ItemsInfo, ISelectable * InventoryOwner) const;

	/* Version that takes a pointer to a AInventoryItem */
	EGameWarning CanItemEnterInventory(AInventoryItem * InventoryItem, ISelectable * InventoryOwner) const;

protected:

	/* Put an item into a slot. Assumes it is valid to put item there. Updates HUD if requested */
	void PutItemInSlotChecked(ISelectable * InventoryOwner, bool bUpdateHUD, bool bIsOwnerSelected,
		bool bIsOwnerCurrentSelected, EInventoryItem ItemType, int32 Quantity,
		int32 SlotIndex, const FInventoryItemInfo & ItemsInfo, EItemAquireReason AquireReason, 
		URTSHUD * HUDWidget);

public:

	/** 
	 *	Called when a slot is used. Puts it on cooldown, decrements a charge and updates HUD 
	 *	
	 *	@param Owner - selectable that owns this inventory
	 *	@param ServerSlotIndex - index in SlotsArray that was used
	 *	@param AbilityInfo - ability info struct for the slot that was used 
	 */
	void OnItemUsed(UWorld * World, ISelectable * Owner, EAffiliation OwnersAffiliation, 
		uint8 ServerSlotIndex, const FContextButtonInfo & AbilityInfo, 
		URTSGameInstance * GameInstance, URTSHUD * HUDWidget);

	/** 
	 *	Called when an item is sold to a shop or something 
	 *
	 *	@param Owner - selectable that owns this inventory
	 *	@param AmountSold - how many of the items in the stack were sold
	 */
	void OnItemSold(ISelectable * Owner, uint8 ServerSlotIndex,
		int16 AmountSold, const FInventoryItemInfo & ItemsInfo, URTSHUD * HUDWidget);

	/** 
	 *	Called when the owner of this inventory reaches zero health. Drop items that are ment 
	 *	to be dropped when reaching zero health e.g. divine rapier
	 *	
	 *	@return - number of items dropped 
	 */
	uint32 OnOwnerZeroHealth(UWorld * World, URTSGameInstance * GameInst, ARTSGameState * GameState, 
		ISelectable * Owner, const FVector & OwnerLocation, float OwnerYawRotation);

protected:

	/* Return whether a location is considered to be on a cliff. Intended to be called during 
	OnOwnerZeroHealth */
	bool OnOwnerZeroHealth_IsLocationConsideredACliff(const FVector & SelectableLocation, const FVector & Location) const;

public:

	//==========================================================================================
	//	Post Edit Function
	//==========================================================================================

#if WITH_EDITOR
	void OnPostEdit(AActor * InOwningActor);
#endif 
};


/** 
 *	A struct that holds information about a selectable resource. A selectable resource is 
 *	something like mana or energy.
 *	
 *	My notes: we could ditch using NumTicksAhead and instead just use CurrentNumTicks which would 
 *	be the game tick that it is up to date with currently. Using NumTicksAhead is actually more 
 *	cumbersome. But the only reason I use it is because I want to avoid a mass branch prediction 
 *	miss in a single frame when the game tick count is incremented to 255 which then means it has 
 *	to be changed to 0.
 */
USTRUCT()
struct FSelectableResourceInfo
{
	GENERATED_BODY()

	/** 
	 *	A multiplier that determines how accurate Amount is. 
	 *
	 *	Larger means more accurate. But means we should be careful about not allowing MaxAmount 
	 *	to be too high i.e. INT32_MAX / MULTIPLIER is actually roughly the max amount.
	 *	
	 *	Because we divide and multiply by this often you will get best performance if this is 
	 *	a power of two. In fact I static_assert it.
	 */
	static constexpr int32 MULTIPLIER = 8192;

protected:

	/** 
	 *	The type of resource this is. Use None if you do not want a resource 
	 *	
	 *	This doesn't affect much. One thing it will affect is the color of the progress bar 
	 *	on the UI. Really minor. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ESelectableResourceType Type;

	/* [Client] How many ticks ahead of GS::TickCounter this resource is in regards to its 
	regen. Negative means this resource is behind */
	UPROPERTY()
	int8 NumTicksAhead;

	/** 
	 *	Amount * MULTIPLIER. 
	 *	
	 *	Range: [0, MaxAmount * MULTIPLIER] 
	 */
	UPROPERTY()
	int32 AmountUndivided;

	/* Range: [0, MaxAmount]. This is the value you want to show in the UI. 
	This is the value to query whenever we want to know whether we have enough of this resource */
	UPROPERTY()
	int32 Amount;

#if WITH_EDITORONLY_DATA
	/**
	 *	How much of this resource is regenerated every minute.
	 *
	 *	Can be negative.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditProperties))
	int32 RegenRatePerMinute;

	UPROPERTY()
	bool bCanEditProperties;
#endif

	/* How much of this resource we should regenerate each custom RTS game tick except it's 
	MULTIPLIER times too big. */
	UPROPERTY()
	int32 RegenRatePerGameTickUndivided;

	/** 
	 *	The maximum amount of this resource we are allowed e.g. for high templars it is 200. 
	 *	
	 *	The max value of this is dependent on FSelectableResourceInfo::NUM_DECIMAL_PLACES. By 
	 *	default this is 4. INT32_MAX / 10^4 = INT32_MAX / 10000 = ~214 000. 
	 *	By default this number should stay below 214 000. If you decide to use 5 decimal 
	 *	places then you will need to then stay below 21 400, and so on. It's probably good to 
	 *	stay a little more below that even.
	 *
	 *	Good way to remove this restriction: just make AmountUndivided an int64
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1, EditCondition = bCanEditProperties))
	int32 MaxAmount;

	/** 
	 *	How much we start with. 
	 *	
	 *	My notes: couldn't this possibly be an editor only variable? 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditProperties))
	int32 StartingAmount;

	/* Given AmountUndivided return the value to use for Amount */
	int32 AmountUndividedToAmount() const;

#if WITH_EDITOR
	/* Convert regen rate from per minute to per custom RTS game tick */
	static int32 CalculateRegenRatePerGameTick(int32 InRegenRatePerMinute);
#endif

public:

	// Set some sensible defaults
	FSelectableResourceInfo();

	/* Get what type of resource this struct is for. Returns None if this struct is not being 
	used */
	ESelectableResourceType GetType() const;

	/* Will only be used by clients. Set how many ticks ahead of GS::TickCounter this selectable's
	resource regen is. Negative values mean behind */
	void SetNumTicksAhead(int8 NumTicks);

	/** 
	 *	Set the value of NumTicksAhead given some stuff. 
	 *	
	 *	@param GameTickCount - local value of ARTSGameState::TickCounter 
	 *	@param InstigatingEffectTickCount - the tick count on the server when the effect was 
	 *	instigated 
	 */
	void SetNumTicksAheadGivenStuff(uint8 GameTickCount, uint8 InstigatingEffectTickCount);

	//===========================================================================================
	//	Adjustment functions
	//===========================================================================================

protected:

	/* By using this function we get a little more accuracy when users want to multiply Amount */
	void SetAmountInternal(int32 NewAmountUndivided, uint8 GameStatesTickCount, 
		uint8 InstigatingEffectsTickCount, bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, 
		USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget,
		bool bIsSelected, bool bIsCurrentSelected);

public:

	/* Set how much of this resource we have. Stays within limits */
	void SetAmount(int32 NewAmount, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount,
		bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected);
	/* Multiply how much of this resource we have. Stays within limits */
	void SetAmountViaMultiplier(float Multiplier, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount,
		bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected);
	
	/**
	 *	Adjust the current amount. Stays within limits.
	 *
	 *	@param AdjustAmount - how much to add to Amount
	 *	@param HUDWidget - reference to the local player's HUD widget
	 *	@param bIsSelected - whether the selectable these attributes belong to is selected by the
	 *	player
	 *	@param bIsCurrentSelected - whether the selectable is the player's CurrentSelected
	 */
	void AdjustAmount(int32 AdjustAmount, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount, 
		bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget, 
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected);

	/* Version that takes a lot less params */
	void AdjustAmount(int32 AdjustAmount, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount,
		ISelectable * Owner, bool bUpdateUI);

	//-------------------------------------------------------------------------------------------
	//	Permanent adjustment functions. Use these for permanent changes.

	/** 
	 *	Functions to set what the new max amount should be. These are for permanent changes 
	 *	such as upgrades and level up bonuses. Will not allow max amount to go lower than 1.
	 *
	 *	@return - new max amount
	 */
	int32 SetMaxAmount(int32 NewMaxAmount, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount, 
		bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner,
		USelectableWidgetComponent * PersistentWorldWidget, USelectableWidgetComponent * SelectedWorldWidget, 
		bool bIsSelected, bool bIsCurrentSelected, 
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Percentage);
	int32 SetMaxAmountViaMultiplier(float Multiplier, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount, 
		bool bUpdateUI,  URTSHUD * HUDWidget, ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget, 
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Percentage);
	/** 
	 *	If AdjustAmount = 50 and we currently have 200 max mana then after this function we will 
	 *	have 250 
	 */
	int32 AdjustMaxAmount(int32 AdjustAmount, uint8 GameStatesTickCount, uint8 InstigatingEffectsTickCount, 
		bool bUpdateUI, URTSHUD * HUDWidget, ISelectable * Owner, USelectableWidgetComponent * PersistentWorldWidget, 
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected,
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Percentage);

	/**
	 *	Add a value to the regen rate per second. This is for permanent changes such as upgrades
	 *	and level up bonuses.
	 *
	 *	@param AdjustAmount - how much to adjust regeneration rate by. This is in units per second.
	 *	@return - the regen rate per second after the change is applied
	 */
	float AdjustRegenRate(float AdjustAmount, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		UWorld * World, ARTSGameState * GameState);
	/** 
	 *	Set what the regeneration rate per minute should now be. This is for permanent changes 
	 *	such as upgrades and level up bonuses. 
	 *
	 *	@return - the regen rate per second after the change is applied
	 */
	float SetRegenRate(float NewRegenRatePerSecond, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		UWorld * World, ARTSGameState * GameState);
	/** 
	 *	Multiply the current regeneration rate. This is for permanent changes such as upgrades 
	 *	and level up bonuses
	 *
	 *	@param Multiplier - what to multiply the current regeneration rate by e.g. 1.2f means we're 
	 *	increasing the regeneration rate by 20%
	 *	@return - the regen rate per second after the change is applied
	 */
	float SetRegenRateViaMultiplier(float Multiplier, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		UWorld * World, ARTSGameState * GameState);

	/** 
	 *	Functions to change the starting amount. 
	 *	
	 *	@param CurrentAmountAdjustmentRule - how to adjust current amount. If we are applying this 
	 *	upgrade as one of the initial on spawn upgrades then this will likely want to be 
	 *	Absolute. Haven't really tested this though 
	 */
	void SetStartingAmount(int32 NewStartingAmount, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Absolute);
	void SetStartingAmountViaMultiplier(float Multiplier, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected, 
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Absolute);
	void AdjustStartingAmount(int32 AdjustAmount, uint8 GameStateTickCount, uint8 InstigatingEffectTickCount,
		ISelectable * Owner, bool bUpdateUI, URTSHUD * HUDWidget, USelectableWidgetComponent * PersistentWorldWidget,
		USelectableWidgetComponent * SelectedWorldWidget, bool bIsSelected, bool bIsCurrentSelected,
		EAttributeAdjustmentRule CurrentAmountAdjustmentRule = EAttributeAdjustmentRule::Absolute);

	//-------------------------------------------------------------------------------------------
	//	Temporary adjustment functions. Use these for temporary changes. TODO


	//=============================================================================================
	//	Other functions
	//=============================================================================================

	/* Regenerate some of this resource based on how many game ticks have passed */
	void RegenFromGameTicks(int8 NumGameTicks, URTSHUD * HUDWidget, ISelectable * Owner, 
		ARTSPlayerController * LocalPlayerController, USelectableWidgetComponent * PersistentWorldWidget, 
		USelectableWidgetComponent * SelectedWorldWidget);

	/* Get how much of this resource we have as an integer */
	int32 GetAmount() const;

	/* Get how much of this resource we have as a float for display purposes only */
	float GetAmountAsFloatForDisplay() const;

	/* Get how much of this resource regenerates each second */
	float GetRegenRatePerSecond() const;

	/* Returns the regen rate per second and as a float. This is for display purposes only */
	float GetRegenRatePerSecondForDisplay() const;

	/* Get the maximum amount of this resource we are allowed */
	int32 GetMaxAmount() const;

	/* Return whether this resource regenerates over time. */
	bool RegensOverTime() const;

#if WITH_EDITOR
	void OnPostEdit();
#endif

	//a;
	
	// Member after all this: gotta look at getting buffs/debuffs/abilities replicating

	// For the selectable resources we gotta: 
	// Add avariable to AInfatry and ABuilding that is the tickCount at time it is spawned.
	// Variable should be replicated and we will need to wait for it before calling Setup()
	// When a building/unit is built add it to an array on GS that registers it as a 
	// selectable resource user . Do like if (SelectableResource != None && RegenRate != 0) then add
	// When the selectable reaches zero health make sure to remove it from this array. 
	// Remember when deciding if removing do the same if (SelectableResource != None && RegenRate != 0) check first 

	// Can do inventory and items too if feel like it

	/* Some thoughts about inventory: 
	- A rule bIsInventoryVisibleToAllies and bIsInventoryVisibleToEnemies that can go in 
	projectsettings.h */
};


/* Information about the mesh to show when a unit gathers a resource */
USTRUCT()
struct FGatheredResourceMeshInfo
{
	GENERATED_BODY()

protected:

	/* The mesh to show */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UStaticMesh * Mesh;

	/* Name of bone on main body mesh to attach to */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName SocketName;

	/* Attachment rule */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EAttachmentRule AttachmentRule;

	/* Location/rotation offset + scale of mesh */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTransform AttachTransform;

public:

	FGatheredResourceMeshInfo();

	UStaticMesh * GetMesh() const { return Mesh; }
	const FName & GetSocketName() const { return SocketName; }
	EAttachmentRule GetAttachmentRule() const { return AttachmentRule; }
	const FTransform & GetAttachTransform() const { return AttachTransform; }
};


/* Info about about gathering resources */
USTRUCT()
struct FResourceCollectionAttribute
{
	GENERATED_BODY()

protected:

	/* Time it takes to gather resource. Lower = gather faster. 0 = instant */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float GatherTime;

	/* Amount of resources this unit can hold */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	uint32 Capacity;

	/* The info about the mesh to show when this resource is gathered */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	FGatheredResourceMeshInfo MeshInfo;

public:

	FResourceCollectionAttribute();

	float GetGatherTime() const { return GatherTime; }
	uint32 GetCapacity() const { return Capacity; }
	void SetCapacity(uint32 NewCapacity) { Capacity = NewCapacity; }
	const FGatheredResourceMeshInfo & GetMeshInfo() const { return MeshInfo; }
};


/* Struct for holding all the resource collecting properties of a selectable */
USTRUCT()
struct FResourceGatheringProperties
{
	GENERATED_BODY()

protected:

	/* How much movement speed is multiplied by for carrying resources. Range of between 0 and
	1 seems sensible. This could probably be made resource type specific */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.001"))
	float MovementSpeedPenaltyForHoldingResources;

	/* An entry in here inplies this unit can gather this type of resource */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EResourceType, FResourceCollectionAttribute > ResourceGatheringProperties;

public:

	FResourceGatheringProperties();

	/* Get how much movement speed should be multiplied by when holding resources */
	float GetMoveSpeedPenaltyForHoldingResources() const;

	/* Return whether this selectable can gather at least one resource */
	bool IsCollector() const;

	/* Return whether selecable can harvest a resource */
	bool CanGatherResource(EResourceType ResourceType) const;

	/* Get the time it takes to gather resource */
	float GetGatherTime(EResourceType ResourceType) const;

	/* Get how much of a resource this selectable can carry */
	uint32 GetCapacity(EResourceType ResourceType) const;

	/* Get the properties for the mesh to show when a resource is gathered */
	const FGatheredResourceMeshInfo & GetGatheredMeshProperties(EResourceType ResourceType) const;

	/* Just get the whole TMap */
	const TMap < EResourceType, FResourceCollectionAttribute > & GetTMap() const;

	/* Just get the whole TMap */
	TMap<EResourceType, FResourceCollectionAttribute> & GetAllAttributesModifiable();
};


/* Attributes common to ALL selectables */
USTRUCT()
struct FSelectableAttributesBase
{
	GENERATED_BODY()

protected:

	/* The display name of the selectable to optionally appear in UI */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/* Description of this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Description;

	/* Sound to try play when owning player selects this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * SelectionSound;

	/** The context menu buttons for this selectable */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	FContextMenuButtons ContextMenu;

	/* Whether selectable is selected by the local player */
	UPROPERTY()
	uint8 bIsSelected : 1;

	/* Primary selected means this is the selectable whose context menu is showing in the HUD. 
	If this is true it implies that the selectable is selected i.e. bIsSelected will be true 
	aswell. 
	For a long time I called this "CurrentSelected" instead */
	UPROPERTY()
	uint8 bIsPrimarySelected : 1;

	UPROPERTY()
	ETeam Team;

	UPROPERTY()
	EAffiliation Affiliation;

	/* ID of player who is controlling this */
	UPROPERTY()
	FName PlayerID;

	UPROPERTY()
	ESelectableType SelectableType;

	/* Can be something like NotBuilding if the selectable is not a building. I think we double 
	up on this variable because I have it in ABuilding. Solution is to make the ABuilding::Type 
	variable editor only and on post edit set this variable to it. Do same with UnitType and 
	AInfatry */
	UPROPERTY()
	EBuildingType BuildingType;

	/* Can be something like NotUnit if the selectable is not a unit */
	UPROPERTY()
	EUnitType UnitType;

	UPROPERTY()
	EFogStatus VisionState;

	/* How the selection decal has been setup */
	UPROPERTY()
	ESelectionDecalSetup SelectionDecalSetup;

	/* Material instance dynamic for selection decal. Can be null. Can tell from SelectionDecalSetup */
	UPROPERTY()
	UMaterialInstanceDynamic * SelectionDecalMID;

	/* Image to appear on HUDs when selecting or producing this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HUDImage_Normal;

	/* Image to appear on HUDs when mouse is hovered over a button for this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HUDImage_Hovered;

	/* Image to appear on HUDs when mouse is pressed on a button for this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HUDImage_Pressed;

	/**
	 *	The index from owning faction info this selectable gets its selection and right-click
	 *	particles from. This is here so different particle system templates can be used for different
	 *	size selectables instead of scaling particle system which can sometimes not turn out right
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	int32 ParticleSize;

public: 

	//===============================================================================
	//	Constructor
	//===============================================================================

	FSelectableAttributesBase();

	/* If this isn't declared virtual then project will crash on load at approx 91% */
	virtual ~FSelectableAttributesBase() { }

	//===============================================================================
	//	Trivial Getters and Setters
	//===============================================================================

	const FText & GetName() const { return Name; }
	void SetName(const FText & InName) { Name = InName; }
	const FText & GetDescription() const { return Description; }
	USoundBase * GetSelectionSound() const { return SelectionSound; }
	const FContextMenuButtons & GetContextMenu() const { return ContextMenu; }
	bool IsSelected() const { return bIsSelected; }
	void SetIsSelected(bool bInIsSelected) { bIsSelected = bInIsSelected; }
	bool IsPrimarySelected() const { return bIsPrimarySelected; }
	void SetIsPrimarySelected(bool bInValue) { bIsPrimarySelected = bInValue; }
	ETeam GetTeam() const { return Team; }
	void SetTeam(ETeam InTeam) { Team = InTeam; }
	EAffiliation GetAffiliation() const { return Affiliation; }
	void SetAffiliation(EAffiliation InAffiliation) { Affiliation = InAffiliation; }
	const FName & GetOwnerID() const;
	ESelectableType GetSelectableType() const { return SelectableType; }
	EBuildingType GetBuildingType() const { return BuildingType; }
	EUnitType GetUnitType() const { return UnitType; }
	EFogStatus GetVisionState() const { return VisionState; }
	void SetupBasicTypeInfo(ESelectableType InSelectableType, EBuildingType InBuildingType, EUnitType InUnitType);
	void SetupSelectionInfo(const FName & InOwnerID, ETeam InTeam);
	void SetVisionState(EFogStatus InVisionState) { VisionState = InVisionState; }
	ESelectionDecalSetup GetSelectionDecalSetup() const { return SelectionDecalSetup; }
	void SetSelectionDecalSetup(ESelectionDecalSetup InValue) { SelectionDecalSetup = InValue; }
	UMaterialInstanceDynamic * GetSelectionDecalMID() const { return SelectionDecalMID; }
	void SetSelectionDecalMaterial(UMaterialInstanceDynamic * InMID) { SelectionDecalMID = InMID; }
	const FSlateBrush & GetHUDImage_Normal() const { return HUDImage_Normal; }
	const FSlateBrush & GetHUDImage_Hovered() const { return HUDImage_Hovered; }
	const FSlateBrush & GetHUDImage_Pressed() const { return HUDImage_Pressed; }
	void SetHUDImage_Normal(const FSlateBrush & InImage) { HUDImage_Normal = InImage; }
	void SetHUDImage_Hovered(const FSlateBrush & InImage) { HUDImage_Hovered = InImage; }
	void SetHUDImage_Pressed(const FSlateBrush & InImage) { HUDImage_Pressed = InImage; }
	int32 GetParticleSize() const { return ParticleSize; }
};


/* Not 100% sure what this struct represents, but I derive building attributes and infantry 
attributes from it, so it's stuff common to both of them */
USTRUCT()
struct FSelectableAttributesBasic : public FSelectableAttributesBase
{
	GENERATED_BODY()

protected:

	/* Sound to try play when built. For buildings this is when construction has completed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * JustBuiltSound;

	/* Sound to try play when this selectable reaches zero health */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * ZeroHealthSound;

	/* Cost to build/train. Maps resource type to cost */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EResourceType, int32 > Costs;

	/**
	 *	For a building this is the time it takes to be constructed.
	 *	For a unit this is the time from when production starts to when the 'open door' animation
	 *	of the barracks is triggered if using open door anim
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.001"))
	float BuildTime; 

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.001"))
	float MaxHealth;

	/* For targeting purposes. The type of selectable this is */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ETargetingType TargetingType;

	/**
	 *	The selectables armour. Armour affects how much damage is taken from different kinds
	 *	of damage sources
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EArmourType ArmourType;

	/* How much incoming damage is multiplied by */
	UPROPERTY()
	float DefenseMultiplier;

	/** Fog of war reveal radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float SightRange;

	/**
	 *	Radius stealth enemies will be revealed (given you can also see that far).
	 *
	 *	0 = cannot reveal stealth. Can be larger than SightRange but still requires vision 
	 *	from another friendly in order to see the stealth unit. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float StealthRevealRange;

	/** 
	 *	How much experience is awarded to an enemy when it destroys this unit. 
	 *
	 *	Can be modified as the match progresses. One time it can increase is when this selectable 
	 *	levels up
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0"))
	float ExperienceBounty;

	/* A resource like mana, energy, etc. Set this to "None" to not have one */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSelectableResourceInfo SelectableResource_1;

	/**
	 *	Buildings that must be built before building this.
	 *
	 *	Adding the building that trains this unit is not required. Duplicates will have
	 *	no additional effect and will be removed in post edit
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray <EBuildingType> BuildingPrerequisites;

	/** 
	 *	Upgrades that must be researched before this selectable can be built. 
	 *	Do not add duplicates. 
	 *	
	 *	My notes: TODO remove duplicates in post edit 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<EUpgradeType> UpgradePrerequisites;

	/**
	 *	Maps body location to the component + bone/socket name on the main mesh. Any blank entries
	 *	will default to NAME_None and the root component
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < ESelectableBodySocket, FAttachInfo > BodyAttachPoints;

	/* Default attach info when no info about a particular location is specified */
	UPROPERTY()
	FAttachInfo DefaultAttachInfo;

	/**
	 *	If using a persistent panel like in C&C in HUD, the tab to appear in. Use "None" to
	 *	exclude this from appearing on the HUD persistent panel
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "HUD Persistent Tab Category"))
	EHUDPersistentTabType HUDPersistentTabCategory;

	/**
	 *	If using a persistent panel like in C&C in HUD, the ordering of button.
	 *	Lower = appears as one of the first buttons in tab (In C&C weaker buildings are
	 *	usually first like power plants)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "HUD Persistent Tab Button Ordering"))
	int32 HUDPersistentTabButtonOrdering;

	/**
	 *	If true then every player (ally and enemy) will be aware that this selectable is built.
	 *	Currently only used for deciding whether to play 'just built' sound.
	 *
	 *	e.g. true = Kirovs in RA2
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bAnnounceToAllWhenBuilt;

	/**
	 *	Sounds to play when given a context command. If any entries are left blank or null then
	 *	the move command sound will be used if infantry
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EContextButton, USoundBase * > ContextCommandSounds;

	//==========================================================================================
	//				------- Temporary modifiers -------
	//------------------------------------------------------------------------------------------
	//	These only exist because of floating point drift.
	//	Could eliminate these if switch to fixed point numbers 
	//------------------------------------------------------------------------------------------

	int16 NumTempMaxHealthModifiers;

	/* What our max health is without any temporary buffs/debuffs */
	UPROPERTY()
	float DefaultMaxHealth;

	/* Number of temporary defense multipliers applied (one reason would be because of a buff 
	or debuff) */
	int16 NumTempDefenseMultiplierModifiers;

	/* What our defense multiplier is without any temporary buffs/debuffs */
	UPROPERTY()
	float DefaultDefenseMultiplier;

	//==========================================================================================
	//					------- Setup ------- 

	/* Number of variables that need to be repped in order for client to call Setup(). Default
	is 3: a unique ID for commands, bIsStartingSelectable and AActor::Owner */
	UPROPERTY()
	int32 NumRequiredReppedVariablesForSetup;

	/* Current tally towards NumRequiredReppedVariablesForSetup */
	UPROPERTY()
	int32 NumReppedVariablesReceived;

	/* How the selectable was spawned */
	UPROPERTY()
	ESelectableCreationMethod CreationMethod;

public:

	FSelectableAttributesBasic();
	virtual ~FSelectableAttributesBasic();

	const TMap <EResourceType, int32> & GetCosts() const { return Costs; }
	float GetBuildTime() const { return BuildTime; }
	float GetMaxHealth() const { return MaxHealth; }
	ETargetingType GetTargetingType() const { return TargetingType; }
	EArmourType GetArmourType() const { return ArmourType; }
	float GetSightRange() const { return SightRange; }
	float GetStealthRevealRange() const { return StealthRevealRange; }
	float GetExperienceBounty() const { return ExperienceBounty; }
	void SetExperienceBounty(float NewValue) { ExperienceBounty = NewValue; }
	const FSelectableResourceInfo & GetSelectableResource_1() const { return SelectableResource_1; }
	FSelectableResourceInfo & GetSelectableResource_1() { return SelectableResource_1; }
	bool AnnounceToAllWhenBuilt() const { return bAnnounceToAllWhenBuilt; }
	USoundBase * GetJustBuiltSound() const { return JustBuiltSound; }
	USoundBase * GetZeroHealthSound() const { return ZeroHealthSound; }
	virtual USoundBase * GetCommandSound(EContextButton Command) const;
	const TArray <EBuildingType> & GetPrerequisites() const { return BuildingPrerequisites; }
	const TArray<EUpgradeType> & GetUpgradePrerequisites() const { return UpgradePrerequisites; }
	const FAttachInfo & GetBodyAttachPointInfo(ESelectableBodySocket AttachPoint) const;
	EHUDPersistentTabType GetHUDPersistentTabCategory() const { return HUDPersistentTabCategory; }
	int32 GetHUDPersistentTabButtonOrdering() const { return HUDPersistentTabButtonOrdering; }

	/* Return whether this is a selectable we started the match with */
	bool IsAStartingSelectable() const;
	ESelectableCreationMethod GetCreationMethod() const;
	void SetCreationMethod(ESelectableCreationMethod InMethod);

	/* Set how far ahead of GS::TickCounter this selectable is. 
	This is only really used by the selectable resource so the variable lies on that struct. 
	But if more things need to use it then it would be a good idea to move it off there onto 
	this struct */
	void SetNumCustomGameTicksAhead(int8 NumAhead);

	/* Return whether this selectable has a selectable resource. Selectable resources are things 
	like mana or energy */
	bool HasASelectableResource() const;

	/* Returns whether this selectable has a selectable resource that also regenerates over 
	time */
	bool HasASelectableResourceThatRegens() const;

	/* Get the type of selectable resource the selectable uses. Will return None if they do 
	not use one */
	ESelectableResourceType GetSelectableResourceType() const;

	//==========================================================================================
	//	Functions for modifying variables that would commonly be modified by upgrades and/or 
	//	buffs/debuffs.

protected:

	/** 
	 *	Possibly change current health in response to a max health change. Will also try update 
	 *	the HUD
	 *	
	 *	@param Owner - the selectable this struct resides on
	 *	@param PreviousMaxHealth - max health (not default max health) before the change happened 
	 *	@param CurrentHealthAdjustmentRule - rule that decides how to change current health
	 */
	void AdjustCurrentHealthAfterMaxHealthChange(ISelectable * Owner, float PreviousMaxHealth, 
		EAttributeAdjustmentRule CurrentHealthAdjustmentRule);

	//=========================================================================================
	//	Functions to notify HUD about changes
	//=========================================================================================

	void TellHUDAboutChange_MaxHealth(ISelectable * Owner, float CurrentHealth);
	void TellHUDAboutChange_MaxHealthAndCurrentHealth(ISelectable * Owner, float CurrentHealth);
	void TellHUDAboutChange_DefenseMultiplier(ISelectable * Owner);

public:

	float GetDefaultMaxHealth() const;

	/**
	 *	These functions will also optionally adjust the current health too 
	 *	
	 *	@param CurrentHealth - reference to the current health for whatever selectable we are 
	 *	adjusting this max health for 
	 *	@param CurrentHealthAdjustmentRule - rule that decides how to adjust current health 
	 */
	void SetMaxHealth(float NewMaxHealth, ISelectable * Owner, EAttributeAdjustmentRule CurrentHealthAdjustmentRule);
	void SetMaxHealthViaMultiplier(float Multiplier, ISelectable * Owner, EAttributeAdjustmentRule CurrentHealthAdjustmentRule);
	float ApplyTempMaxHealthModifierViaMultiplier(float Multiplier, ISelectable * Owner, EAttributeAdjustmentRule CurrentHealthAdjustmentRule);
	float RemoveTempMaxHealthModifierViaMultiplier(float Multiplier, ISelectable * Owner, EAttributeAdjustmentRule CurrentHealthAdjustmentRule);

	/* Get the defense multiplier right now */
	float GetDefenseMultiplier() const { return DefenseMultiplier; }

	/** 
	 *	Get the default defense multiplier. This is what our defense multiplier would be without 
	 *	any temporary effects like buffs/debuffs, but does include permanent changes such as  
	 *	upgrades and level up bonuses
	 */
	float GetDefaultDefenseMultiplier() const;

	/** 
	 *	Set what the defense multiplier is. The current multiplier will also be changed based 
	 *	on percentage. Use this function for any permanent changes such as for upgrades or level 
	 *	up bonuses 
	 */
	void SetDefenseMultiplier(float NewDefenseMultiplier, ISelectable * Owner);

	/* Same as SetDefenseMultiplier but instead multiplies the the multiplier by another 
	multiplier */
	void SetDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);

	/* @return - defense multiplier after the change has been applied */
	float ApplyTempDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);
	/* @return - defense multiplier after the change has been applied */
	float RemoveTempDefenseMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);

	// Add 1 to the tally of replicated variables needed to call Setup()
	void IncrementNumReppedVariablesReceived();

	// Return whether we can call ISelectable::Setup()
	bool CanSetup() const;

#if WITH_EDITOR
	virtual void OnPostEdit(AActor * InOwningActor);
#endif
};


/**
 *	Contains a record of where units have been successfully evacced to and locations where
 *	their overlap tests failed. This is for unloading all units at once using the grid method.
 */
struct FGarrisonEvacHistory_UnloadAllAtOnce_Grid
{
public:

	static const FVector LOCATION_INVALID;

	/* @param GarrisonedUnits - array of garrisoned units in building */
	FGarrisonEvacHistory_UnloadAllAtOnce_Grid(URTSGameInstance * GameInst, ABuilding * Building, 
		const FBuildingGarrisonAttributes * BuildingsGarrisonAttributes);

	/** 
	 *	Return the next location to try. The idea here is to return locations that hopefully
	 *	aren't colliding with anything in the first place so minimal overlap tests fail 
	 *	
	 *	@param UnitsCollisionInfo - some collision info for EvacingSelectable as a convenience 
	 */
	FTransform GetInitialTransformToTry(const UWorld * World, ABuilding * BuildingBeingEvaced, 
		const FBuildingGarrisonAttributes * BuildingsGarrisonAttributes, ISelectable * EvacingSelectable, 
		const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo);

	/* @return - the param Location but it's Z axis is on the ground */
	static FVector PutLocationOnGround(const UWorld * World, const FVector & Location);

	enum class ENextTest : uint8
	{
		InitialTest,
		JustZ,
		XY,
		XYZ,
		FullAdjustment,
		GiveUp
	};

	enum class EDirection : uint8
	{
		Left,
		Right
	};

	/** Copy of FSelectableRootComponent2DShapeInfo */
	struct FUnitBasic2DCollisionInfo
	{
		FUnitBasic2DCollisionInfo(const FSelectableRootComponent2DShapeInfo & InUnitsCollisionInfo);

		float X;
		float Y;
	};

	struct FOverlapTestResult
	{
		FOverlapTestResult(const FVector & InLocation, const FSelectableRootComponent2DShapeInfo & InUnitsCollisionInfo)
			: Location(InLocation)
			, UnitsCollisionInfo(InUnitsCollisionInfo)
		{}

		FVector2D Location;
		FUnitBasic2DCollisionInfo UnitsCollisionInfo;
	};

	struct FBuildingSideInfo
	{
		FBuildingSideInfo() { }

		explicit FBuildingSideInfo(FVector InFacingAwayUnitVector)
			: FacingAwayUnitVector(InFacingAwayUnitVector)
			, NumLeftSideFails(0)
			, NumRightSideFails(0) 
			, NumOnCurrentRow(0) 
			, NumRows(0)
			, CurrentRowShiftLeftAmount(0.f) 
			, CurrentRowShiftRightAmount(0.f) 
			, CurrentRowDirection(EDirection::Left)
		{
			PreviousOverlapTests.Reserve(32);
		}

		bool NeedToStartOnNewRow(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, EDirection InDirection) const;
		float GetRecommendedShiftAwayDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const;
		float GetRecommendedShiftLeftDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const;
		float GetRecommendedShiftRightDistance(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo) const;

		//---------------------------------------------------------------------
		//	Data
		//---------------------------------------------------------------------

		/** History of overlap tests both encroaching and unencroaching  */
		TArray<FOverlapTestResult> PreviousOverlapTests;
		/** Vector facing away from building. GetActorForwardVector() would be an example of one */
		FVector FacingAwayUnitVector;
		/** Number of times a unit has failed to be placed on that side */
		int16 NumLeftSideFails, NumRightSideFails;
		int16 NumOnCurrentRow;
		/* How many rows have been filled */
		int16 NumRows;
		float CurrentRowShiftLeftAmount;
		float CurrentRowShiftRightAmount;
		EDirection CurrentRowDirection;
	};

	/**
	 *	Get notified about trying a location rotation and the overlap test failing. This
	 *	function will then inform the caller whether to try again or not
	 *
	 *	@param InOutState - where along the process the caller is. Also this is used to tell
	 *	the caller what to do next
	 *	@param InOutLocation - location overlap test failed at. Also an out param - this func will
	 *	put another location into it to try if this returns true
	 *	@param InOutRotation - same deal as Location except for rotation
	 */
	void NotifyOfFailedOverlapTest(ENextTest & InOutState, const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, 
		FVector & InOutLocation, FQuat & InOutRotation, const FVector & ProposedAdjustment, const FVector & OriginalLocation);

	void NotifyOfSuccessfulOverlapTest(const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, 
		const FVector & Location, const FQuat & Rotation);

protected:

	/**
	 *	Call when all the adjustments from an overlap test have been tried and none of them have 
	 *	worked. Either try another location or just give up trying to find loc for the unit. 
	 */
	void OnAllAdjustmentsTriedForLocation(ENextTest & InOutState, const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo,
		FVector & InOutLocation, FQuat & InOutRotation, const FVector & OriginalLocation);

	/** 
	 *	Will return LOCATION_INVALID to if it cannot find any suitable location 
	 *	
	 *	@return - a location that has a decent chance at not colliding with anything else 
	 *	previously placed, or LOCATION_INVALID if could not find one 
	 */
	FVector TryResolveOverlapsWithPreviousLocations(FGarrisonEvacHistory_UnloadAllAtOnce_Grid::FBuildingSideInfo & CurrentSideInfo,
		FVector Location, const FSelectableRootComponent2DShapeInfo & UnitsCollisionInfo, 
		EDirection PrefferedDirection);

	//--------------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------------

	/** Holds information about the 4 sides of a building */
	FBuildingSideInfo FoursSidesInfo[4];
	int32 CurrentFoursSidesInfoIndex;
	int16 NumUnitsEvacced;
	bool bHasGivenOutFirstLocation;
};


/** Static attributes for building garrison networks */
USTRUCT()
struct FBuildingNetworkAttributes
{
	GENERATED_BODY()

public:

	FBuildingNetworkAttributes()
		: NumGarrisonSlots(10)  
	{}

	int16 GetNumGarrisonSlots() const { return NumGarrisonSlots; }

protected:

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	/**
	 *	How many 'slots' the network has for units to be garrisoned in it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1, DisplayName = "Slot Capacity"))
	int16 NumGarrisonSlots;
};


/* State for a single building garrison network for a single player */
struct FBuildingNetworkState
{
public:

	FBuildingNetworkState()
		: SlotUsage(0)
	{}

	/* Call a function "Setup" and intellisense takes forever to find it */
	void SetupStruct(const FBuildingNetworkAttributes & ThisNetworksAttributes)
	{
		SlotCapacity = ThisNetworksAttributes.GetNumGarrisonSlots();
	}

	const TArray<TRawPtr(AActor)> & GetGarrisonedUnits() const { return GarrisonedUnits; }
	TArray<TRawPtr(AActor)> & GetGarrisonedUnits() { return GarrisonedUnits; }

	bool HasEnoughCapacityToAcceptUnit(AInfantry * UnitThatWantsToEnter) const;
	/* @return - index inside the garrisoned units container that Unit was placed at */
	int32 ServerOnUnitEntered(AInfantry * Unit);
	/* @return - index inside the garrisoned units container that Unit was placed at */
	int32 ClientOnUnitEntered(AInfantry * Unit);
	/* @return - index inside the garrisoned units container that Unit was removed from */
	int32 ServerOnUnitExited(AInfantry * Unit);
	/* @return - index inside the garrisoned units container that Unit was removed from */
	int32 ClientOnUnitExited(AInfantry * Unit);
	/* @return - index inside the garrisoned units container that Unit was removed from */
	int32 OnGarrisonedUnitZeroHealth(AInfantry * Unit);

protected:

	void OnUnitAdded_UpdateSlotUsage(const FEnteringGarrisonAttributes & AddedUnitsAttributes);
	void OnUnitRemoved_UpdateSlotUsage(const FEnteringGarrisonAttributes & RemovedUnitsAttributes);

	//---------------------------------------------------------------
	//	Data
	//---------------------------------------------------------------

	/**
	 *	TArray of all the units inside this garrison.
	 *	Not auto memory managed i.e. not UPROPERTY or TWeakObjectPtr - unit's are responsible to update
	 *	the container when they reach zero health so invalids should not be an issue.
	 */
	TArray<TRawPtr(AActor)> GarrisonedUnits;

	/* Number of slots this garrison has */
	int16 SlotCapacity;

	/** Number of slots that are being used. Will change as selectables enter and exit */
	int16 SlotUsage;
};


/**
 *	For a single player this struct contains all the information about all building garrison
 *	networks for that player
 */
USTRUCT()
struct FPlayersBuildingNetworksState
{
	GENERATED_BODY()

public:

	FPlayersBuildingNetworksState()
	{
		/* Default initialize NetworksContainer */
		for (uint8 i = 0; i < Statics::NUM_BUILDING_GARRISON_NETWORK_TYPES; ++i)
		{
			NetworksContainer[i] = FBuildingNetworkState();
		}
	}

	/* Just do Statics::BuildingNetworkTypeToArrayIndex and that's the index in NetworksContainer */
	FBuildingNetworkState * GetNetworkInfo(EBuildingNetworkType NetworkType);

	/**
	 *	This array contains state for each network type.
	 *	Use Statics::BuildingNetworkTypeToArrayIndex(EBuildingNetworkType) to get the right index
	 *	for your network type.
	 *
	 *	Even if a faction does not use a network type (e.g. I'm playing terran so I will never
	 *	have my units enter the 'nydus' network) an entry is still there in case I make
	 *	building/unit possession a thing (e.g. I gain control of a nydus worm - now I DO need
	 *	to keep track of what is in that network). /End verbose comment
	 *
	 *	C++ does not like zero sized arrays, so if the player has not defined 
	 *	any custom networks then to get around this it is given a size of 1
	 */
	FBuildingNetworkState NetworksContainer[FMath::Max<int32>(Statics::NUM_BUILDING_GARRISON_NETWORK_TYPES, 1)];
};


/** Attributes that say how many units can be garrisoned inside a building */
USTRUCT()
struct FBuildingGarrisonAttributes
{
	GENERATED_BODY()

public:

	FBuildingGarrisonAttributes()
		: GarrisonNetworkInfo(nullptr)
		, NumGarrisonSlots(0)
		, SlotUsage(0)
		, HightestSightRadiusContainerContributor(nullptr)
		, HighestStealthRevealRadiusContainerContributor(nullptr)
		, MouseCursor_CanEnter(EMouseCursorType::None) 
		, EnterRange(300.f) 
		, EnterSound(nullptr) 
		, EvacuateSound(nullptr)
		, UnloadAllMethod(EGarrisonUnloadAllMethod::AllAtOnce_Grid)
		, bCanSeeOutOf(true) 
		, NetworkType(EBuildingNetworkType::None)
	{
#if WITH_EDITOR
		OnPostEdit();
#endif
	}

	/** Called when the building that owns this does its setup */
	void OnOwningBuildingSetup(ARTSPlayerState * OwningBuildingsOwner);

	/**
	 *	[Server] Call when a infantry enters this garrison
	 *
	 *	@param ThisBuilding - the building these attributes belong to
	 *	@param Infantry - infantry that entered this garrison
	 */
	void ServerOnUnitEntered(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * Infantry, URTSHUD * HUD);

	/** [Client] Call when an infantry enters this garrison */
	void ClientOnUnitEntered(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * Infantry, URTSHUD * HUD);

	/* [Server] Unload asingle unit from garrison */
	void ServerUnloadSingleUnit(ARTSGameState * GameState, ABuilding * ThisBuilding, AActor * UnitToUnload, URTSHUD * HUD);

	/* [Server] Unload all units from the garrison */
	void ServerUnloadAll(ARTSGameState * GameState, ABuilding * ThisBuilding, URTSHUD * HUD);

	/* [Client] Call when the unit has exited the garrison */
	void ClientOnUnitExited(ARTSGameState * GameState, ABuilding * ThisBuilding, AInfantry * ExitingInfantry, URTSHUD * HUD);

	/**
	 *	Call when a unit inside the garrison reaches zero health.
	 *
	 *	@return - the index inside GarrisonedUnits that Unit was at. Will probably never
	 *	be INDEX_NONE
	 */
	int32 OnGarrisonedUnitZeroHealth(ABuilding * ThisBuilding, AInfantry * Unit);

protected:

	/** 
	 *	Get where to place a unit that is leaving the garrison
	 *	
	 *	@param EvacingUnit - unit that is leaving the garrison
	 *	@param EvacHistory - info about evacuations previously completed possibly this frame 
	 *	or maybe over the last few frames. I idea is to query this for the next possible location 
	 *	to put the next unit, cause you can't just put them all in the same spot. Make sure 
	 *	that whoever calls this func adds info to this array if it does indeed evac the unit there
	 *	@param OutLocation - world location where to put evacing unit
	 *	@param OutRotation - rotation to set for evacing unit
	 *	@return - true if a location and rotation were found 
	 */
	bool GetSuitableEvacuationTransform(const UWorld * World, ABuilding * ThisBuilding, AActor * EvacingUnit,
		FGarrisonEvacHistory_UnloadAllAtOnce_Grid & EvacHistory, FVector & OutLocation, FQuat & OutRotation) const;

	/** 
	 *	Basically I'm going to make this similar to the UWorld version but I will 
	 *	remove some stuff I don't need. TBH this function belongs more in Statics class 
	 *	or something 
	 *
	 *	MY GUESS AT PARAMS:
	 *	@param OutProposedAdjustment - a vector that says which way to adjust the TestWorldTransform
	 *	if the test fails
	 *	@return - true if the component is encroaching something so it's colliding with something
	 */
	static bool ComponentEncroachesBlockingGeometry_WithAdjustment(const UWorld * World, 
		const AActor * TestActor, const UPrimitiveComponent * PrimComp, 
		const FTransform & TestWorldTransform, FVector & OutProposedAdjustment);

	static bool ComponentEncroachesBlockingGeometry_NoAdjustment(const UWorld * World,
		const AActor * TestActor, const UPrimitiveComponent * PrimComp,
		const FTransform & TestWorldTransform);

	void OnUnitAdded_UpdateSlotUsage(const FEnteringGarrisonAttributes & AddedUnitsAttributes);
	void OnUnitRemoved_UpdateSlotUsage(const FEnteringGarrisonAttributes & RemovedUnitsAttributes);

	/* This will update the building's sight and stealth reveal radiuses when a unit is removed from 
	the garrison. 2 reasons for removing unit: player ordered it to leave, it died. */
	void OnUnitRemoved_UpdateSightAndStealthRadius(ABuilding * ThisBuilding, AInfantry * RemovedUnit);

public:

	/* Play sound for when a unit or possibly units leave */
	void PlayEvacSound(ARTSGameState * GameState, ABuilding * ThisBuilding);

	bool IsGarrison() const { return GetTotalNumGarrisonSlots() > 0; }
	/* This will likely return null if not part of a network */
	FBuildingNetworkState * GetGarrisonNetworkInfo() const { return GarrisonNetworkInfo; }
	bool IsPartOfGarrisonNetwork() const { return GarrisonNetworkInfo != nullptr; }
	/* Get the total number of slots this garrison has */
	int16 GetTotalNumGarrisonSlots() const { return NumGarrisonSlots; }
	/* Get how many slots are currently being used. Will likely be not helpful if this garrison 
	is part of a garrison network */
	int16 GetSlotUsage() const;
	/* Get how many units are in the garrison */
	int16 GetNumUnitsInGarrison() const;
	/* Container of all units garrisoned inside building */
	const TArray<TRawPtr(AActor)> & GetGarrisonedUnitsContainerTakingIntoAccountNetworkType() const;
	const FCursorInfo & GetCanEnterMouseCursor(URTSGameInstance * GameInst) const;
	float GetEnterRange() const { return EnterRange; }
	bool HasEnoughCapacityToAcceptUnit(AInfantry * UnitThatWantsToEnter) const;
	EGarrisonUnloadAllMethod GetUnloadAllMethod() const { return UnloadAllMethod; }
	EBuildingNetworkType GetGarrisonNetworkType() const { return NetworkType; }

protected:

	/** 
	 *	Pointer to the garrison network info struct on the player state that owns the building. 
	 *	If this garrison is not part of a garrison network then this will be null 
	 */
	FBuildingNetworkState * GarrisonNetworkInfo;

	/**
	 *	TArray of all the units inside this garrison. 
	 *	Not auto memory managed i.e. not UPROPERTY or TWeakObjectPtr - unit's are responsible to update 
	 *	the container when they reach zero health so invalids should not be an issue. 
	 */
	TArray<TRawPtr(AActor)> GarrisonedUnits;

	/** 
	 *	How many 'slots' the building has for units to be garrisoned in it.
	 *
	 *	If this building is part of a garrison network then this is not used. 
	 *	If you're wondering where to edit the capacity of network it's 
	 *	URTSGameInstance::BuildingNetworkInfo
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = "!bIsPartOfGarrisonNetwork"))
	int16 NumGarrisonSlots;

	/** 
	 *	Number of slots that are being used. Will change as selectables enter and exit. 
	 *	Not kept up to date if this building is part of a garrison network. 
	 */
	int16 SlotUsage;

public:

	/* The unit that is contributing to the sight radius i.e. the unit with a sight radius 
	larger than any other unit garrisoned in the building AND larger than the building's sight 
	radius too If null then the building is the contributor */
	ISelectable * HightestSightRadiusContainerContributor;
	ISelectable * HighestStealthRevealRadiusContainerContributor;

protected:

	/** The mouse cursor to show for this garrison to say a unit can enter it */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType MouseCursor_CanEnter;

	/** Range when units can enter garrison */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float EnterRange;

	/** A sound that can be played when a unit enters the garrison */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * EnterSound;

	/* A sound that can be played when a unit leaves the garrison */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * EvacuateSound;

	/* How all the units leave when given a command to all exit */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EGarrisonUnloadAllMethod UnloadAllMethod;

	/** 
	 *	If true then units that enter this garrison can see out of it i.e. their vision 
	 *	radius and stealth reveal radius still matter.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint8 bCanSeeOutOf : 1;

	/** 
	 *	The building garrison network this is part of. Building garrison networks mean that 
	 *	units are shared across buildings 
	 *	e.g. 
	 *	- in C&C generals tunnel networks 
	 *	- in SCII nydus worms 
	 *
	 *	Use None if you do not want the building to be part of any network
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingNetworkType NetworkType;

	/* Possible future variables: */
	// bool bCanUnitsAttackFromInside;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bIsPartOfGarrisonNetwork;
#endif

public:

#if WITH_EDITOR
	void OnPostEdit()
	{
		bIsPartOfGarrisonNetwork = (NetworkType != EBuildingNetworkType::None);
	}
#endif
};


USTRUCT()
struct FBuildingAttributes : public FSelectableAttributesBasic
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HUDImage_Highlighted;

	/* Effective sight radius is the final sight radius to use for this building after taking 
	into account all the units garrisoned inside of it */
	UPROPERTY()
	float EffectiveSightRange;
	
	/* Same except for stealth reveal radius instead */
	UPROPERTY()
	float EffectiveStealthRevealRange;

	/**
	 *	The maximum amount of units/upgrades that can be queued in the training queue at once.
	 *
	 *	Only one unit is trained at a time no matter the queue size. If this building is
	 *	not a unit-producing building then setting this as 0 is a good way to save on memory.
	 *	Cancelling an item in production or adding an item to production is O(n)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMax = 250))
	uint8 ProductionQueueLimit;

	/* What a unit that is produced from this building does after it has spawned */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditProductionProperties))
	EUnitInitialSpawnBehavior ProducedUnitInitialBehavior;

public:

	/* Queue that handles production. This queue handles producing units and upgrades */
	UPROPERTY()
	FProductionQueue ProductionQueue;

	/* Another queue that handles production of buildings and upgrades when producing them from
	the HUD persistent panel. This is kept seperate from the other queue to allow players to build
	from the HUD persistent panel and use the context menu to produce stuff at the same time. Not
	something that a lot of people would probably use though */
	UPROPERTY()
	FProductionQueue PersistentProductionQueue;

protected:

	/* True if the building has a production queue limit of at least 1 and has a "Train" or
	"Upgrade" command as one of its context commands. Edited by PostEditProperty */
	UPROPERTY()
	bool bCanProduce;

	/** 
	 *	Whether this building is a construction yard type building or not. This slot will be used
	 *	to build buildings only and will use the persistent production queue 
	 *	Currently limited to 1 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, ClampMax = 1))
	int32 NumPersistentBuildSlots;

	/* Whether to override how the building is built defined in faction info */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (InlineEditConditionToggle))
	bool bOverrideFactionBuildMethod;

	/* The build method to use for this if choosing to override the faction default build method */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideFactionBuildMethod))
	EBuildingBuildMethod BuildMethodOverride;

	/** Whether to override AFactionInfo::BuildingProximityRange */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOverrideFactionBuildProximity;

	/** @See AFactionInfo::BuildingProximityRange */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideFactionBuildProximity, ClampMin = 0))
	float BuildProximityRange;

	/** 
	 *	Only applicable if: 
	 *	- this building uses the BuildsInTab build method. 
	 *	- this building does not have a "construction" animation
	 *	
	 *	The time it takes building to go from being placed to being fully constructed 
	 *
	 *	e.g. in Red Alert 3 and Allies power plant takes about 10secs to build, then you place it 
	 *	somewhere and it takes about another 2 seconds to construct. This variable refers to that 
	 *	2nd time I mentioned
	 *
	 *	Side note: If using a construct anim then make sure an anim notfy has been placed at the
	 *	end of the animation to signal construction finished 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.001"))
	float ConstructTime;

	/* Animations to play */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EBuildingAnimation, UAnimMontage *> Animations;

	/** Sound to play in the world when the building is placed into the world */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * JustPlacedSound;

	/** 
	 *	Sound to play while the building is being worked on. It should be looping. I think this 
	 *	will play even if your build method is one the 3 that builds itself 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * BeingWorkedOnSound;

	/** 
	 *	Array of resources that can be dropped off at this building by resource gatherers e.g. in
	 *	Starcraft II a nexus is a drop point for minerals and vespene gas 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<EResourceType> ResourceCollectionTypes;

	/* How close collector has to get to depot for it to be able to drop off resources. Measured
	from building's GetActorLocation() to dunno */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float ResourceDropRadius;

	/* Radius for how close unit has to get to building before it can lay down its foundations,
	measured from building's GetActorLocation() to dunno */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float FoundationsRadius;

#if WITH_EDITORONLY_DATA
	/* How much of the housing resources this building provides */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EHousingResourceType, int16 > HousingResourcesProvided;
#endif // WITH_EDITORONLY_DATA
	
	/* This array contains the values in HousingResourcesProvided and is updated on post edit. 
	It exists for performance only */
	UPROPERTY()
	TArray <int16> HousingResourcesProvidedArray;

	/* The items this shop has on display and optionally sells */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FShopInfo ShoppingInfo;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FBuildingGarrisonAttributes GarrisonAttributes;

	/** 
	 *	The particles to play when the building is placed into the world. Will not show if 
	 *	building is a starting selectable. 
	 *	They do not move as the building rises
	 *	This particle system will not be stopped so it should not be looping 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FParticleInfo PlacedParticles;

	/** 
	 *	The particles to play while the building's construction progress is being advanced. 
	 *	They do not move as the building rises
	 *	These particles should be looping. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FParticleInfo AdvancingConstructionParticles;

	/* Array for static buffs applied to this building */
	TArray < FStaticBuffOrDebuffInstanceInfo > StaticBuffs;

	/* Array of static debuffs applied to this building */
	TArray < FStaticBuffOrDebuffInstanceInfo > StaticDebuffs;

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME && ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS
	/* Array of tickable buffs applied to this building */
	TArray < FTickableBuffOrDebuffInstanceInfo > TickableBuffs;

	/* Array of tickable debuffs applied to this building */
	TArray < FTickableBuffOrDebuffInstanceInfo > TickableDebuffs;
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME && ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	/* Whether this building is a base defense type building */
	UPROPERTY()
	bool bIsDefenseBuilding;

public:

	/* Array of all attack components on this building */
	UPROPERTY()
	TArray<UMeshComponent *> AttackComponents_Turrets;

	/* Rotatable bases of the turrets */
	UPROPERTY()
	TArray<UMeshComponent *> AttackComponents_TurretsBases;

	/* How far above the building's mesh its persistent world widget should appear. 
	The axis are probably slightly misleading. The X is left and right and the Y is up and 
	down */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector2D PersistentWorldWidgetOffset;

	/* How far above the building's mesh its selection world widget should appear 
	The axis are probably slightly misleading. The X is left and right and the Y is up and 
	down */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FVector2D SelectionWorldWidgetOffset;

	FBuildingAttributes();

	const FSlateBrush & GetHUDImage_Highlighted() const { return HUDImage_Highlighted; }
	float GetEffectiveSightRange() const { return EffectiveSightRange; }
	float GetEffectiveStealthRevealRange() const { return EffectiveStealthRevealRange; }
	/* @param Contributor - the actor that NewValue belongs to */
	void SetEffectiveSightRange(float NewValue, ISelectable * Contributor);
	void SetEffectiveStealthRevealRange(float NewValue, ISelectable * Contributor);

	/* Capacity of context production queue */
	uint8 GetProductionQueueLimit() const { return ProductionQueueLimit; }
	/* Capacity of persistent build queue */
	int32 GetNumPersistentBuildSlots() const { return NumPersistentBuildSlots; }

	EUnitInitialSpawnBehavior GetProducedUnitInitialBehavior() const { return ProducedUnitInitialBehavior; }

	// TODO does it take into account persistent queues? DO they need to be taken into account?
	bool CanProduce() const { return bCanProduce; }
	bool IsAnimationValid(EBuildingAnimation AnimationType) const
	{
		return Animations.Contains(AnimationType) && Animations[AnimationType] != nullptr;
	}
	const TMap <EBuildingAnimation, UAnimMontage*> & GetAllAnimations() const { return Animations; }
	UAnimMontage * GetAnimation(EBuildingAnimation AnimationType) const { return Animations[AnimationType]; }
	USoundBase * GetJustPlacedSound() const { return JustPlacedSound; }
	USoundBase * GetBeingWorkedOnSound() const { return BeingWorkedOnSound; }
	bool OverridesFactionBuildMethod() const { return bOverrideFactionBuildMethod; }
	EBuildingBuildMethod GetBuildMethod() const { return BuildMethodOverride; }
	void SetBuildMethod(EBuildingBuildMethod InMethod) { BuildMethodOverride = InMethod; }
	bool OverrideFactionBuildProximity() const { return bOverrideFactionBuildProximity; }
	float GetBuildProximityRange() const { return BuildProximityRange; }
	float GetConstructTime() const { return ConstructTime; }
	const TArray < EResourceType > & GetResourceCollectionTypes() const { return ResourceCollectionTypes; }
	float GetResourceDropRadius() const { return ResourceDropRadius; }
	float GetFoundationsRadius() const { return FoundationsRadius; }
	const TArray < int16 > & GetHousingResourcesProvidedArray() const { return HousingResourcesProvidedArray; }
	bool IsDefenseBuilding() const { return bIsDefenseBuilding; }
	const FParticleInfo & GetPlacedParticles() const { return PlacedParticles; }
	const FParticleInfo & GetAdvancingConstructionParticles() const { return AdvancingConstructionParticles; }
	const FShopInfo & GetShoppingInfo() const { return ShoppingInfo; }
	FShopInfo & GetShoppingInfo() { return ShoppingInfo; }
	float GetTimeIntoZeroHealthAnimThatAnimNotifyIsSlow() const;
	const FBuildingGarrisonAttributes & GetGarrisonAttributes() const { return GarrisonAttributes; }
	FBuildingGarrisonAttributes & GetGarrisonAttributes() { return GarrisonAttributes; }

#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS
	TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticBuffs();
	const TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticBuffs() const;
	TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticDebuffs();
	const TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticDebuffs() const;
#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableBuffs();
	const TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableBuffs() const;
	TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableDebuffs();
	const TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableDebuffs() const;
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME 
#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditProductionProperties;
#endif

#if WITH_EDITOR
	virtual void OnPostEdit(AActor * InOwningActor) override;
#endif
};


USTRUCT()
struct FEnteringGarrisonAttributes
{
	GENERATED_BODY()

public:

	FEnteringGarrisonAttributes()
		: BuildingGarrisonSlotUsage(1) 
	{}

	int16 GetBuildingGarrisonSlotUsage() const { return BuildingGarrisonSlotUsage; }

protected:

	/* How many garrison slots this unit uses inside building garrisons */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int16 BuildingGarrisonSlotUsage;
};


/* Attributes common to all infantry */
USTRUCT()
struct FInfantryAttributes : public FSelectableAttributesBasic
{
	GENERATED_BODY()

	/* How much of the housing resource this unit costs */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EHousingResourceType, int16> HousingCosts;

	/** 
	 *	This is a contract variable. Setting this to true means it is possible for this unit to 
	 *	enter stealth at some point.
	 *
	 *	Setting this to false can be an optimization 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanEverEnterStealth;

	/** 
	 *	If true unit will spawn stealthed. 
	 *
	 *	Optionally you can have this unit reveal itself when it does something by adding the 
	 *	anim notify 'OnExitStealthMode' to the Attack/GatherResources/whatever animations.
	 *
	 *	My notes: I think this variable basically means the unit is a stealth unit 
	 *	e.g. jarmen kell, dark templars. If an upgrade now wants to make a unit stealth 
	 *	then this is the variable to set.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bSpawnStealthed;

	/** 
	 *	If true then the unit is in a state where if it decides to break its stealth then it 
	 *	will re-enter it after a StealthRevealTime delay. 
	 *	False basically means we won't try re-enter stealth 
	 *	
	 *	This will rarely change during gameplay. I don't even know but this variable might be
	 *	redundant. Anytime this is used bSpawnStealthed could possibly replace it 
	 */
	UPROPERTY()
	bool bInToggleStealthMode;

	/** 
	 *	Time from when the unit breaks stealth to when it should re-enter stealth. 
	 *
	 *	"Breaks stealth" means you executed an action that should remove your stealth, and has 
	 *	nothing to do with being detected by an enemy.
	 *	
	 *	Some special values for this:
	 *	- Setting this to 0 and/or not setting the 'OnExitStealthMode' anim notifies means unit will 
	 *	not reveal self ever (similar to ghosts or dark templars in SCII) unless given an explicit 
	 *	command to do so (e.g. using unclock for ghosts).
	 *	- Setting this to less than 0 means unit will never re-enter stealth after notify and 
	 *	will require an explicit command to re-enter stealth
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEverEnterStealth, ClampMin = "0.0"))
	float StealthRevealTime;

	/* Name of param in dynamic material instance that controls the stealth effect */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEverEnterStealth))
	FName StealthMaterialParamName;

	/** 
	 *	Value to set on stealth material instance when in stealth mode. 
	 *
	 *	If the param is controlling alpha then this will likely want to be something less than 1 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEverEnterStealth))
	float InStealthParamValue;

	/* Whether the unit can ever gain experience */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanGainExperience;

#if WITH_EDITORONLY_DATA
	/**
	 *	How much experience is required to reach the level.
	 *	Non-cumulative so these values are how much to go from the current level to the next. 
	 *
	 *	If this is greyed out check LevelingUpOptions::EXPERIENCE_ENABLED_GAME in ProjectSettings.h
	 */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (DisplayName = "Experience Requirements", EditCondition = bCanEditExperienceProperties))
	TMap <uint32, float> ExperienceRequirementsTMap;
#endif

	/** 
	 *	Values from TMap. Gets populated in post edit. 
	 *
	 *	Index 0 = first gainable level, index 1 = second gainable level, and so on 
	 *	e.g. If you start on level 1 and can level up to 3: 
	 *	index 0 = EXP requirement for level 2 
	 *	index 1 = EXP requirement for level 3 
	 */
	UPROPERTY()
	TArray <float> ExperienceRequirementsArray;

	/** 
	 *	Multiplies experience earned. 
	 *	
	 *	Higher = level up faster 
	 */
	UPROPERTY()
	float ExperienceGainMultiplier;

	/* Whether the unit can build buildings. This is set automatically during post edit, 
	usually if the context menu has at least one button for building a building */
	UPROPERTY()
	bool bCanBuildBuildings;

	/** Whether unit can repair buildings. Repairs only, not construction */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanRepairBuildings;

	/** 
	 *	Resource collection properties.
	 *	
	 *	The game wide rules for resource collection are as follows:
	 *	----- It's basically the same as SCII. To be more specific: ------
	 *	- Each resource spot (similar to a mineral patch in SCII) can provide only one type of
	 *	resource. 
	 *	- Each resource drop point (similar to a nexus in SCII) is allowed to take in multiple 
	 *	resource types.
	 *	- A collector (like probes in SCII) must stay its full duration at a resource spot uninterrupted
	 *	for it to gain its capacity in resources. If interrupted it must restart the timer for
	 *	gathering resource (same behavior as SCII I believe).
	 *	- If a collector holding resources chooses to gather from another resource spot of a different
	 *	resource type it will lose the resources it has already gathered upon successfully gathering 
	 *	the new resource type (same behavior as SCII I believe) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	FResourceGatheringProperties ResourceGatheringProperties;

	/* The items this unit has */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FInventory Inventory;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	FBuildingTargetingAbilitiesAttributes BuildingTargetingAbilityAttributes;

	/* These mouse cursors probably belong on FAttackAttributes more */
	/* The mouse cursor to show when player hovers their mouse over a hostile selectable that
	this unit can attack */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType CanAttackHostileMouseCursor;

	/* The mouse cursor to show when player hovers their mouse over a friendly selectable that
	this unit can attack */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EMouseCursorType CanAttackFriendlyMouseCursor;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FEnteringGarrisonAttributes GarrisonAttributes;

#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	/* Buffs applied to this infantry that do not expire on their own and have no 
	tick logic. Using an array because it helps with displaying them on UI */
	TArray<FStaticBuffOrDebuffInstanceInfo> StaticBuffs;

	/* Debuffs applied to this infantry that do not expire on their own and have no
	tick logic. Using an array because it helps with displaying them on UI */
	TArray<FStaticBuffOrDebuffInstanceInfo> StaticDebuffs;

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	
	/* Buffs applied to this infantry that can expire on their own and/or have tick logic.
	Advantages of using array:
	- faster iteration during tick
	Advantages of set:
	- faster containment checks for knowing if buff/debuff is applied which could be commonly 
	used for abilities and/or applying buffs/debuffs */
	TArray < FTickableBuffOrDebuffInstanceInfo > TickableBuffs;

	/* Just like TickableBuffs but for debuffs */
	TArray < FTickableBuffOrDebuffInstanceInfo > TickableDebuffs;

#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	/* Particles that are attached to the unit */
	TArray <FAttachedParticleInfo> AttachedParticles;

	/* How many move speed multipliers have been applied to this unit. Counts both speed ups 
	and slow downs. This is here to correct for gradual floating point number drift */
	int16 NumTempMoveSpeedModifiers;

	int16 NumTempExperienceGainModifiers;
	float DefaultExperienceGainMultiplier;

	/* How many temporary effects (such as those from buffs/debuffs) are on us that give us 
	stealth mode */
	int16 NumTempStealthModeEffects;

	/* The sound to try play when given a move command */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * MoveCommandSound;

	FInfantryAttributes();

protected:

	void TellHUDAboutChange_ExperienceGainMultiplier(ISelectable * Owner);

public:

	bool ShouldSpawnStealthed() const;
	/* Get the value to set on the param of a material */
	float GetInStealthParamValue() const;

	const TMap < EHousingResourceType, int16 > & GetHousingCosts() const { return HousingCosts; }

	USoundBase * GetMoveCommandSound() const { return MoveCommandSound; }
	virtual USoundBase * GetCommandSound(EContextButton Command) const override;

	const FInventory & GetInventory() const { return Inventory; }
	FInventory & GetInventory() { return Inventory; }

protected:
	/* Converts a level to the index in ExperienceRequirementsArray it should be */
	int32 LevelToArrayIndex(uint32 Level) const;
public:
	/* Get how much total experience is required to reach a level when on the previous level */
	float GetTotalExperienceRequirementForLevel(FSelectableRankInt Level) const;

	bool CanEverGainExperience() const { return bCanGainExperience; }

#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticBuffs();
	const TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticBuffs() const;
	TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticDebuffs();
	const TArray < FStaticBuffOrDebuffInstanceInfo > & GetStaticDebuffs() const;

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableBuffs();
	const TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableBuffs() const;
	TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableDebuffs();
	const TArray < FTickableBuffOrDebuffInstanceInfo > & GetTickableDebuffs() const;
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

	/* Get how many move temporary move speed modifiers have been applied to this unit */
	int16 GetNumTempMoveSpeedModifiers() const { return NumTempMoveSpeedModifiers; }

	/* Get the current experience gain multiplier */
	float GetExperienceGainMultiplier() const;
	/* Get the experience gain multiplier excluding any temporary effects */
	float GetDefaultExperienceGainMultiplier() const;

	/* Permanently change the experience gain multiplier */
	void SetExperienceGainMultiplier(float NewExperienceGainMultiplier, ISelectable * Owner);
	void SetExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);

	/* @return - experience gain multiplier after the change has been applied */
	float ApplyTempExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);
	float RemoveTempExperienceGainMultiplierViaMultiplier(float Multiplier, ISelectable * Owner);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditExperienceProperties;
#endif

#if WITH_EDITOR
	virtual void OnPostEdit(AActor * InOwningActor) override;
#endif
};


/* The selectables a player starts the match with. Kind of a game mode struct */
USTRUCT()
struct FStartingSelectables
{
	GENERATED_BODY()

protected:

	TArray < EBuildingType > Buildings;
	TArray < EUnitType > Units;

public:

	void AddStartingSelectable(EBuildingType InBuildingType);
	void AddStartingSelectable(EUnitType InUnitType);
	void SetStartingBuildings(const TArray < EBuildingType > & SelectableTypes);
	void SetStartingUnits(const TArray < EUnitType > & SelectableTypes);

	const TArray < EBuildingType > & GetBuildings() const;
	const TArray < EUnitType > & GetUnits() const;
};
