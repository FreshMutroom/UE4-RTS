// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Statics/CommonEnums.h"
#include "Statics/OtherEnums.h"
#include "Statics/Structs_4.h"
#include "Selectable.generated.h"

class AFactionInfo;
struct FInfantryAttributes;
struct FBuildingAttributes;
struct FSelectableAttributesBasic;
struct FAttackAttributes;
class AProjectileBase;
struct FBuildInfo;
struct FContextMenuButtons;
struct FContextButton;
struct FProductionQueue;
class ABuilding;
struct FAttachInfo;
struct FUnitInfo;
struct FStaticBuffOrDebuffInstanceInfo;
struct FTickableBuffOrDebuffInstanceInfo;
class URTSGameInstance;
struct FContextButtonInfo;
class ARTSPlayerController;
class URTSHUD;
class USelectableWidgetComponent;
struct FItemOnDisplayInShopSlot;
struct FShopInfo;
struct FInventory;
class ARTSPlayerState;
class UBoxComponent;
struct FSelectableAttributesBase;
class AInventoryItem;
struct FInventorySlotState;
struct FInventoryItemInfo;
class AResourceSpot;
class ARTSGameState;
struct FBuildingInfo;
struct FTickableBuffOrDebuffInfo;
struct FStaticBuffOrDebuffInfo;
struct FCursorInfo;
struct FBuildingTargetingAbilityPerSelectableInfo;
struct FBuildingGarrisonAttributes;


/* Macro for fatal logging name of function. To be placed inside the base implementation of 
functions that really should be pure virtual */
#define NO_OVERRIDE_SO_FATAL_LOG UE_LOG(RTSLOG, Fatal, TEXT("Virtual function %s not overridden"), *FString(__FUNCTION__));


struct FSelectableRootComponent2DShapeInfo
{
public:

	FSelectableRootComponent2DShapeInfo(float CapsuleRadius)
		: Data(FVector2D(CapsuleRadius, -1.f))
	{}

	FSelectableRootComponent2DShapeInfo(FVector2D BoxDimensions)
		: Data(BoxDimensions)
	{}

	/* Given the actor's rotation is 0 get the hald length of the collision component's 
	X axis half length. This is confusing... basically it's the radius of capsule if using 
	capsule. Otherwise it's just the half length of the box's X axis */
	float GetXAxisHalfDistance() const { return Data.X; }
	/* Same deal except for Y axis. Returns same value as X axis version if this struct 
	holds data for a capsule/sphere */
	float GetYAxisHalfDistance() const { return Data.Y < 0.f ? Data.X : Data.Y; }
	
	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	/* If Y is negative then X holds radius for a sphere or capsule. Otherwise it 
	holds data for a box, but remember this is 2D not 3D so the Z axis data is not here */
	FVector2D Data;
};


UINTERFACE(MinimalAPI)
class USelectable : public UInterface
{
	GENERATED_BODY()
};

/**
 *	Any class implementing this interface means they can be selected by the player during a match.
 */
class RTS_VER2_API ISelectable
{
	GENERATED_BODY()

public:

	/* Call once the selectables owner is known/replicated */
	virtual void Setup();

	/* Get attribute struct that all selectables are guaranteed to have */
	virtual const FSelectableAttributesBase & GetAttributesBase() const = 0;
	virtual FSelectableAttributesBase & GetAttributesBase() = 0;

	/* The rest of these attribute structs selectables aren't guaranteed to have */
	virtual const FSelectableAttributesBasic * GetAttributes() const;
	virtual const FBuildingAttributes * GetBuildingAttributes() const;
	// May want to change this to GetUnitAttributes or something, infantry is too specific
	virtual const FInfantryAttributes * GetInfantryAttributes() const;
	virtual const FAttackAttributes * GetAttackAttributes() const;
	virtual const FContextMenuButtons * GetContextMenu() const;

	/* Non-const version */
	virtual FBuildingAttributes * GetBuildingAttributesModifiable();
	virtual FSelectableAttributesBasic * GetAttributesModifiable();
	virtual FAttackAttributes * GetAttackAttributesModifiable();

	/* Popualate a FBuildInfo with all the things it needs. Called during faction info creation
	@param OutInfo - build info to be modified
	@param FactionInfo - faction info this is being setup for */
	virtual void SetupBuildInfo(FBuildInfo & OutInfo, const AFactionInfo * FactionInfo) const;
	/* Unit's version */
	virtual void SetupBuildInfo(FUnitInfo & OutInfo, const AFactionInfo * FactionInfo) const;

	/** 
	 *	Given experience is enabled for the project, whether this selectable is capable of gaining 
	 *	experience and ultimately level up. This doesn't not take into account any state so is 
	 *	likely to be a simple return true or return false. 
	 */
	virtual bool CanClassGainExperience() const;

	/** 
	 *	Get level/rank. Returns -1 (255) if the selectable does not care about experience and cannot 
	 *	rank up. 
	 */
	virtual uint8 GetRank() const;

	/* Get experience towards next rank/level, range 0 to FLOAT_MAX */
	virtual float GetCurrentRankExperience() const;

	/* Get how much experience must be aquired to reach the next level. This amount is only 
	the amount for going from the current level to the next and does not take into account 
	current expereience */
	virtual float GetTotalExperienceRequiredForNextLevel() const;

	/** Get how much experience is required to level up to a level. This amount is similar to 
	GetExperienceRequiredForNextLevel in that it isn't asking for total experience and isn't 
	affected by current experience if that makes sense */
	virtual float GetTotalExperienceRequiredForLevel(FSelectableRankInt Level) const;

	/** 
	 *	Gain experience and possibly level up 
	 *	
	 *	@param ExperienceBounty - the experience bounty we earned 
	 */
	virtual void GainExperience(float ExperienceBounty, bool bApplyRandomness);

	// Get bounds for selecting object
	virtual UBoxComponent * GetBounds() const;

	virtual FSelectableRootComponent2DShapeInfo GetRootComponent2DCollisionInfo() const;

	/* Called when selected */
	virtual void OnSingleSelect();

	/* Not pure virtual. Also return the units type and puts selectable ID in param
	@param SelectableID - the selectables ID used for issuing commands
	@return - the selectables unit type */
	virtual EUnitType OnMarqueeSelect(uint8 & SelectableID);

	/** Called when the selectable is selected using a ctrl group select action */
	virtual EUnitType OnCtrlGroupSelect(uint8 & OutSelectableID);

	virtual uint8 GetSelectableID() const;

	/* Get the player ID of the player that owns this selectable */
	virtual uint8 GetOwnersID() const;

	/* @return - selectable ID */
	virtual uint8 OnDeselect();

	virtual void OnRightClick();

	/* Called when hovered over with mouse on tick */
	virtual void OnMouseHover(ARTSPlayerController * LocalPlayCon);
	/* Called when no longer hovered over with mouse on tick. This also isn't
	strictly speaking true. Actually called every tick my marquee HUD whether
	still inside marquee box or not */
	virtual void OnMouseUnhover(ARTSPlayerController * LocalPlayCon);

	/* Called when the player's pending marquee selection box has this selectable inside it */
	virtual void OnEnterMarqueeBox(ARTSPlayerController * LocalPlayCon);

	/* Called when the selectable stops being inside the player's marquee selection box */
	virtual void OnExitMarqueeBox(ARTSPlayerController * LocalPlayCon);

	/* Not pure virtual */
	virtual float GetHealth() const;

	/* Get a reference to selectables health. */
	virtual float & GetHealthRef();

	/* Not pure virtual */
	virtual void SetHealth(float NewValue);

	/* Not pure virtual */
	virtual ETeam GetTeam() const;

	virtual uint8 GetTeamIndex() const;

	/* Not pure virtual */
	virtual void GetInfo(/*OUT*/ TMap < EResourceType, int32 > & OutCosts, /*OUT*/ float & OutBuildTime, /*OUT*/ FContextMenuButtons & OutContextMenu) const;

	/* Return array of all prerequisites this needs to be built.
	TODO: change to return reference (&), remove const and make it pure
	virtual */
	virtual TArray <EBuildingType> GetPrerequisites() const;

	/* Get cooldown remaining for ability.
	@return - cooldown remaining. 0 or negative numbers imply there
	is no cooldown remaining */
	virtual float GetCooldownRemaining(const FContextButton & Button);

	// TODO this and several other 'pointer to struct' funcs could  be made pure virtual and 
	// changed to reference returning funcs
	/* Get map that has cooldowns remaining for each context action */
	virtual const TMap < FContextButton, FTimerHandle > * GetContextCooldowns() const;

	/** 
	 *	Return whether this selectable has enough selectable resources to use an ability.
	 *
	 *	Selectable resources are things like mana or energy. 
	 */
	virtual bool HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const;

	/* Get the game tick count that this selectable is 'up to' */
	virtual uint8 GetAppliedGameTickCount() const;

	/** 
	 *	Regenerate selectable resources (e.g. mana) due to time passing 
	 *	
	 *	@param NumGameTicksWorth - how many custom RTS game ticks worth of resources we should 
	 *	regenerate 
	 */
	virtual void RegenSelectableResourcesFromGameTicks(uint8 NumGameTicksWorth, URTSHUD * HUDWidget,
		ARTSPlayerController * LocalPlayerController);

	/** 
	 *	Called when an ability is used by this selectable. Will generally spend resources 
	 *	(as in mana type resources) and start the cooldown timer handle. 
	 *	
	 *	@param AbilityInfo - info struct of the ability that is being used 
	 *	@param ServerTickCountAtTimeOfAbility - server's GS::GetGameTickCounter at the time the 
	 *	ability was used on the server. Can ignore this param if we're the server
	 *
	 *	TODO rename to something like OnContextMenuAbilityUse
	 */
	virtual void OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility);

	virtual float GetSightRadius() const;

	virtual void GetVisionInfo(float & SightRadius, float & StealthRevealRadius) const;

	/* Used for knowing if in range of something. Just using a simple sphere
	for range calculations. Buildings that have length and width that are
	different are not supported well with this implementation */
	virtual float GetBoundsLength() const;

	/** 
	 *	Called every time fog of war manager ticks regardless of if status has
	 *	changed or not
	 *	
	 *	@param FogStatus - visibility status of tile this selectable is on
	 *	@return - true if it can be seen 
	 */
	virtual bool UpdateFogStatus(EFogStatus FogStatus);

	/** 
	 *	[Server] Return whether the selectable can be clicked on. This is intended to be called 
	 *	by the server to check whether a player was actually allowed to click on what they say 
	 *	they clicked on.
	 *	
	 *	@param ClickersTeam - team of player that is doing the clicking 
	 *	@param ClickersTeamTag - team but in FName form 
	 *	@return - true if they can be clicked on 
	 */
	virtual bool Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const;

	virtual bool IsInStealthMode() const;

	/** 
	 *	Called by every owned selectable when an upgrade completes. Does not apply the upgrade 
	 *	but allows the selectable to respond to it. 
	 *	
	 *	Is not called on initial spawn when all completed upgrades apply 
	 *
	 *	@param UpgradeLevel - at what level the upgrade is. Often this will just be 1. 
	 *	e.g. in SCII the 10% damage increase upgrade has 3 levels to it.
	 */
	virtual void OnUpgradeComplete(EUpgradeType UpgradeType, uint8 UpgradeLevel);

	/* Function only used during faction info creation. Called to setup object
	pooling manager */
	virtual TSubclassOf <AProjectileBase> GetProjectileBP() const;

	/* Get a reference to the player controller you would get with GetWorld()->GetFirstPlayerController() */
	virtual ARTSPlayerController * GetLocalPC() const;

	/* Return whether this selectable is capable of attacking. Resource depots will likely be 
	an obvious false. Infantry can also return false. Buildings without attack components will 
	likely return false */
	virtual bool HasAttack() const;

	/** 
	 *	Set whether selectable can take damage. 
	 *	
	 *	For implementing this I will likely use AActor::bCanBeDamaged 
	 */
	virtual void SetIsInvulnerable(bool bInvulnerable);

	/* Get the world location projectiles should spawn from this selectable. I may be able to make 
	this a non-virtual and put it on FAttackAttributes by having it store the mesh */
	virtual FVector GetMuzzleLocation() const;

	/** 
	 *	Return true if the selectable can aquire another selectable as an attack target. 
	 *	Range is not considered unless perhaps this selectable cannot move. 
	 */
	virtual bool CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const;

	/**
	 *	Return whether this selectable has a special action instead of attacking when the
	 *	player right clicks on a building e.g. in C&C engineers will capture the building, spies
	 *	reveal what is being produced, etc. 
	 *	Will return null if this selectable does not have any special building targeting ability
	 *	towards Building
	 *
	 *	@param Building - target building
	 *	@param BuildingAffiliation - affiliation of building as a convenience
	 *	@return - pointer to info struct if this selectable has a special ability towards Building. 
	 *	Otherwise will return null.
	 */
	virtual const FBuildingTargetingAbilityPerSelectableInfo * GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const;

	/** 
	 *	Return mouse cursor info that is the cursor that should be displayed when the player 
	 *	hovers their mouse over a hostile unit and this selectable can attack that unit. 
	 *	This can return null meaning 'there isn't one'. In that case probably a cursor 
	 *	on faction info will be used.
	 *	Might wanna do a ContainsCustomCursor check during this function and if it returns 
	 *	false then return null
	 */
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackHostileUnit(URTSGameInstance * GameInst, AActor * HostileAsActor, ISelectable * HostileUnit) const;
	/** Version for friendly targets */
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackFriendlyUnit(URTSGameInstance * GameInst, AActor * FriendlyAsActor, ISelectable * FriendlyUnit, EAffiliation UnitsAffiliation) const;
	
	/* Same deal except for buildings instead */
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackHostileBuilding(URTSGameInstance * GameInst, ABuilding * HostileBuilding) const;
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackFriendlyBuilding(URTSGameInstance * GameInst, ABuilding * FriendlyBuilding, EAffiliation BuildingsAffiliation) const;

	//================================================================================
	//	Command issuing functions
	//================================================================================

	/** 
	 *	Called when player orders to move to location 
	 *	
	 *	@param Location - location player clicked at 
	 */
	virtual void OnRightClickCommand(const FVector & Location);

	/**
	 *	Called when player right clicks an another selectable while this one is selected.
	 *
	 *	@param TargetAsSelectable - reference to selectable that was right clicked on as a
	 *	selectable
	 */
	virtual void OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo);

	/* Tell selectable to go pick up an inventory item that exists in the world. Assumes that 
	the selectable at least has an inventory */
	virtual void IssueCommand_PickUpItem(const FVector & SomeLocation, AInventoryItem * TargetItem);

	/* Recently added this. Pretty much the same as OnRightClickCommand. We're basically just 
	saying move to a location. This func name is just more accurate because sometimes we 
	tell selectables to move somewhere but it wasn't actually a right click that did it */
	virtual void IssueCommand_MoveTo(const FVector & Location);

	virtual void IssueCommand_RightClickOnResourceSpot(AResourceSpot * ResourceSpot);

	/**
	 *	Issue a command to use a building targeting ability on a building 
	 *	
	 *	@param TargetBuilding - building to use the ability on 
	 *	@param AbilityInfo - info struct for the ability to use 
	 */
	virtual void IssueCommand_SpecialBuildingTargetingAbility(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo);

	/** 
	 *	Issue a command to get in range of a building and enter it 
	 *	
	 *	@param TargetBuilding - garrison to enter 
	 *	@param GarrisonAttributes - TargetBuilding's garrison attributes as a convenience
	 */
	virtual void IssueCommand_EnterGarrison(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes);

	/* Special context command for going to a location and spawning a building as a foundation
	there */
	virtual void OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation);

	/** 
	 *	Issue a 2-click context command that requires another selectable as a target. Can 
	 *	assume target is valid. Can assume ability if off cooldown. Basically everything 
	 *	should have been checked by PC.
	 *	
	 *	@param Command - the action to carry out
	 *	@param ClickLoc - the world coords where the mouse was clicked
	 *	@param Target - the selectable that was clicked on
	 *	@param TargetInfo - selection info about the target 
	 */
	virtual void OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc,
		ISelectable * Target, const FSelectableAttributesBase & TargetInfo);

	/** 
	 *	Issue a 2-click context command that requires a location as the target. Can assume ability 
	 *	is off cooldown.
	 *
	 *	@param AbilityInfo - info about the action to carry out
	 *	@param ClickLoc - the world coords where the mouse was clicked 
	 */
	virtual void OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc);

	/** 
	 *	Issue a 1-click context command 
	 *	
	 *	@param AbilityInfo - info about the context command 
	 */
	virtual void OnContextCommand(const FContextButton & Button);

	/** 
	 *	Issue a use inventory item command. This func will likely just bubble to AI controller. 
	 *	
	 *	@param InventorySlotIndex - the inventory slot we are using. Server index  
	 */
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex,
		EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo);
	/* Version for item uses that target a location in the world */
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex,
		EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo,
		const FVector & Location);
	/* Version for item use that targets another selectable */
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex,
		EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo,
		ISelectable * Target);

	/* [Server] Call when destroying an enemy selectable */
	virtual void OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes);

	virtual void Client_OnLevelGained(uint8 NewLevel);

	/* Get the production queue if a building. Here instead of in Building.h because
	context menu uses reference to ISelectable */
	virtual const FProductionQueue * GetProductionQueue() const;

	/* Cancel producing something if building. Here instead of in Building.h because
	context menu uses reference to ISelectable */
	virtual void OnProductionCancelled(uint8 Index);

	/* Get how far along the construction queue for a building is in a range
	from 0 to 1 */
	virtual float GetBuildingQueuePercentage() const;

	/* Flag whether this selectable is being spawned as one of the first selectables of the
	game that players start with
	@param bStartingSelectable - whether selectable is one that player started game with
	@param BuilderBuilding - building that built this, or null if none */
	virtual void SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding);

	/* Set the selectable ID for this selectable. Also set the value the game tick counter 
	was at when this selectable was created. I guess you could call this on clients but it 
	is ment for the server to set these and have them replicated to clients */
	virtual void SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter);

	/* Get info about the scene component that corrisponds to a particular body location
	@param BodyLocationType - the part of the body to get socket name for
	@return - info that contains the scenecomponent, bone/socket name and attach transform */
	virtual const FAttachInfo * GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const;

	/* @param ParticleIndex - a value that references a certain particle system of the ability. An ability 
	can attach up to 8 particles to a single target so range: [0, 7]. Actually the ParticleIndex 
	thing can probably be ditched. It's really only required for looping particles but no 
	ability will probably put looping particles on something. A buff/debuff that the ability 
	APPLIES might but not the ability in any other way */
	virtual void AttachParticles(const FContextButtonInfo & AbilityInfo, UParticleSystem * Template, 
		ESelectableBodySocket AttachLocation, uint32 ParticleIndex);
	virtual void AttachParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
	virtual void AttachParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);

	/* Remove attached particles that were the result of hacing a buff/debuff */
	virtual void RemoveAttachedParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo);
	virtual void RemoveAttachedParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo);

	/* Called by PC on server when a building is tried to be placed from the context menu.
	@param PlacedBuilding - building that was placed successfully. Will be null if building could
	not be placed
	@param BuildMethod - build method building uses to build */
	virtual void OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, 
		EBuildingBuildMethod BuildMethod);

	/* Called by building on all units working on it to let them know it has finished */
	virtual void OnWorkedOnBuildingConstructionComplete();

	/* Called by server only when the owning player has been defeated but the match is still going */
	virtual void Server_OnOwningPlayerDefeated();

	/** 
	 *	Should just call GetActorLocation() 
	 */
	virtual FVector GetActorLocationSelectable() const;

	/** 
	 *	Get how far the selectable considers itself from another selectable for the purposes 
	 *	of deciding whether it is in range to use an ability. 
	 *	
	 *	@return - distance squared. This may return negative though which should be ok. This 
	 *	could be the case if say we take our own bounds into account and are overlapping the 
	 *	target.
	 */
	virtual float GetDistanceFromAnotherForAbilitySquared(const FContextButtonInfo & AbilityInfo,
		ISelectable * Target) const;

	/** 
	 *	Get how far the selectable considers itself from a world location for the purpsoes of 
	 *	deciding whether it is range to use an ability 
	 *	
	 *	@param Location - world location we are getting the distance from to us
	 *	@return - distance squared. This may return negative though which is still ok. 
	 */
	virtual float GetDistanceFromLocationForAbilitySquared(const FContextButtonInfo & AbilityInfo,
		const FVector & Location) const;

	/**
	 *	To consume a selectable means basically to instantly kill it. 
	 *	Examples: 
	 *	- in C&C when an engineer enters a building to either capture or repair it the engineer 
	 *	is basically destroyed
	 */
	virtual void Consume_BuildingTargetingAbilityInstigator();

	//==================================================================
	//	Buffs and Debuffs
	//==================================================================

	/** 
	 *	Returns whether the class is one that can have buffs/debuffs applied to it. This 
	 *	shouldn't take any state into account so is likely just to be either a straight "true" 
	 *	or "false". If this returns false then the selectable will never have any buffs/debuffs 
	 *	applied to it. 
	 *	Selectables such as resource spots may want this to be false (or not)
	 */
	virtual bool CanClassAcceptBuffsAndDebuffs() const;

	/* Functions that return pointers to the appropriate arrays. If the selectable does not 
	support having buffs/debuffs applied to it then these should return null */
	virtual const TArray < FStaticBuffOrDebuffInstanceInfo > * GetStaticBuffArray() const;
	virtual const TArray < FTickableBuffOrDebuffInstanceInfo > * GetTickableBuffArray() const;
	virtual const TArray < FStaticBuffOrDebuffInstanceInfo > * GetStaticDebuffArray() const;
	virtual const TArray < FTickableBuffOrDebuffInstanceInfo > * GetTickableDebuffArray() const;

	/** 
	 *	Get the state info for a buff/debuff, or nullptr if selectable does not have the buff/debuff 
	 *	applied to them
	 *	
	 *	@param BuffOrDebuffType - buff/deuff to check for 
	 *	@param Type - whether it is a buff or debuff. Passing this in helps speed up function 
	 *	but could be removed if too inconvenient
	 *	@return - nullptr means does not have debuff, otherwise returns pointer to the state info 
	 *	for the buff/debuff requested
	 */
	virtual FStaticBuffOrDebuffInstanceInfo * GetBuffState(EStaticBuffAndDebuffType BuffType);
	virtual FTickableBuffOrDebuffInstanceInfo * GetBuffState(ETickableBuffAndDebuffType BuffType);
	virtual FStaticBuffOrDebuffInstanceInfo * GetDebuffState(EStaticBuffAndDebuffType DebuffType);
	virtual FTickableBuffOrDebuffInstanceInfo * GetDebuffState(ETickableBuffAndDebuffType DebuffType);

	/** 
	 *	Register that this selectable has a buff/debuff. Also sets the duration to full. Should
	 *	update widgets too
	 *	This is local only, not replicated in any way 
	 */
	virtual void RegisterBuff(EStaticBuffAndDebuffType BuffType, AActor * BuffInstigator, ISelectable * InstigatorAsSelectable);
	virtual void RegisterDebuff(EStaticBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable);
	virtual void RegisterBuff(ETickableBuffAndDebuffType BuffType, AActor * BuffInstigator, ISelectable * InstigatorAsSelectable);
	virtual void RegisterDebuff(ETickableBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable);

	/** 
	 *	Remove a buff/debuff from this selectable. Assumes that we have checked that the 
	 *	buff/debuff is applied. Don't know if this will play well with networking though 
	 *	
	 *	@param BuffType - type of buff to remove 
	 *	@param RemovalInstigator - what is instigating the removal of the buff. Can be null 
	 *	@param InstigatorAsSelectable - Instigator as an ISelectable. Can be null
	 *	@param RemovalReason - what is causing the buff to be removed 
	 */
	virtual EBuffOrDebuffRemovalOutcome RemoveBuff(EStaticBuffAndDebuffType BuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason);
	virtual EBuffOrDebuffRemovalOutcome RemoveDebuff(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason);
	virtual EBuffOrDebuffRemovalOutcome RemoveBuff(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason);
	virtual EBuffOrDebuffRemovalOutcome RemoveDebuff(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason);

	/* [Client] Remove a buff/debuff given an outcome of it that was calculated by the server */
	virtual void Client_RemoveBuffGivenOutcome(EStaticBuffAndDebuffType BuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome);
	virtual void Client_RemoveDebuffGivenOutcome(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome);
	virtual void Client_RemoveBuffGivenOutcome(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome);
	virtual void Client_RemoveDebuffGivenOutcome(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator,
		ISelectable * InstigatorAsSelectable, EBuffOrDebuffRemovalOutcome Outcome);

	//=======================================================================================
	//	Added for buffs/debuffs but not required to be exclusively used by them

	/** 
	 *	Here in case we only have an ISelectable pointer but want to deal damage to it. 
	 *	Implementation should just call TakeDamage 
	 *	@See AActor::TakeDamage
	 */
	virtual float TakeDamageSelectable(float DamageAmount, FDamageEvent const& DamageEvent,
		AController * EventInstigator, AActor * DamageCauser);

	/* Return whether the selectable has zero health. The function Statics::HasZeroHealth(AActor *) 
	and this should return the exact same result */
	virtual bool HasZeroHealth() const;

	/**
	 *	[Server] Apply a movespeed modifier that is considered only temporary. It is important 
	 *	when calling this to make sure the buff/debuff that is causing this was not already on 
	 *	the selectable
	 *
	 *	@param Multiplier - amount to multiply current movespeed by
	 *	@return - move speed after the modifier is applied
	 */
	virtual float ApplyTempMoveSpeedMultiplier(float Multiplier);

	/**
	 *	[Server] Remove a movespeed modifier that is considered only temporary. It is important 
	 *	when calling this to make sure the buff/debuff that is causing this was already on 
	 *	the selectable
	 *
	 *	@param Multiplier - amount to multiply current movespeed by
	 *	@return - move speed after the modifier is removed
	 */
	virtual float RemoveTempMoveSpeedMultiplier(float Multiplier);

	/**
	 *	[Server] Make the unit enter stealth mode temporarily
	 *
	 *	@return - whether the unit is in stealth mode after the change, so true every time
	 */
	virtual bool ApplyTempStealthModeEffect();

	/**
	 *	[Server] Remove the effects of a temporary enter stealth mode effect
	 *
	 *	@return - whether the unit is in stealth mode after the change
	 */
	virtual bool RemoveTempStealthModeEffect();

	/* Get pointer to the game instance. Should do exactly what GetGI does */
	virtual URTSGameInstance * Selectable_GetGI() const;

	/* Get a pointer to the player that owns this selectable */
	virtual ARTSPlayerState * Selectable_GetPS() const;

	/** 
	 *	Show the tooltip for this selectable. If the selectable does not want to show a 
	 *	tooltip then make this func do nothing. 
	 */
	virtual void ShowTooltip(URTSHUD * HUDWidget) const;

	virtual USelectableWidgetComponent * GetPersistentWorldWidget() const;
	virtual USelectableWidgetComponent * GetSelectedWorldWidget() const;

	// This one doesn't need to be virtual 
	virtual void AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount);
	virtual void AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount);

	//==========================================================================================
	//	Item browsing/shopping
	//==========================================================================================

	/* Get the shop attributes for this selectable. Returns null if the selectable does not 
	display or sell items */
	virtual const FShopInfo * GetShopAttributes() const;

	/** 
	 *	Called when an item is purchased from this selectable. 
	 *	
	 *	@param ShopSlotIndex - index in the shop items array that the purchase was made 
	 *	@param ItemRecipient - selectable that purchased the item
	 *	@param bTryUpdateHUD - if true and the selectable is selected then update the HUD e.g. 
	 *	change the quantity of the item in the shop if it's a limited quantity item etc
	 */
	virtual void OnItemPurchasedFromHere(int32 ShopSlotIndex, ISelectable * ItemRecipient, 
		bool bTryUpdateHUD);

	/* Returns whether a shop the selectable can sell items to is in range. */
	virtual bool IsAShopInRange() const;

	//==========================================================================================
	//	Inventory
	//==========================================================================================

	/* Get a pointer to the selectable's inventory. Returns null if they do not have an 
	inventory. Will likely return null if the inventory has a capacity of 0 (effectively 
	meaning they don't have an inventory). Actually I think this will return a valid pointer 
	but use FInventory::GetCapacity() to know capacity 0 meaning the selectable cannot 
	carry any items */
	virtual const FInventory * GetInventory() const;
	virtual FInventory * GetInventoryModifiable();

	/** 
	 *	Called when the selectable uses an item in their inventory. Should put it on cooldown, 
	 *	reduce number of charges of item if applicable and spend selectable resource cost. Make 
	 *	sure HUD knows about it - it will need to decrement number of charges and display the 
	 *	cooldown remaining.
	 *	
	 *	@param ServerInventorySlotIndex - FInventory::SlotsArray index of what was used 
	 *	@param AbilityInfo - info struct for the effect of the use 
	 */
	virtual void OnInventoryItemUse(uint8 ServerInventorySlotIndex, const FContextButtonInfo & AbilityInfo,
		uint8 ServerTickCountAtTimeOfAbility);

	/* This should just start the timer handle set to call a OnTimerFinished type func that 
	updates the HUD. */
	virtual void StartInventoryItemUseCooldownTimerHandle(FTimerHandle & TimerHandle, uint8 ServerInventorySlotIndex, float Duration);

#if WITH_EDITOR

	/* Returns whether the selectable is setup to be used with either a CPU player or human
	player */
	virtual bool PIE_IsForCPUPlayer() const;

	/* Get the human player owner index for PIE */
	virtual int32 PIE_GetHumanOwnerIndex() const;

	/* Get the CPU player owner index for PIE */
	virtual int32 PIE_GetCPUOwnerIndex() const;

#endif // WITH_EDITOR

	/* Functions that should be overridden by all that implement this interface but
	only called by themselves */
private:

	virtual void AdjustForUpgrades();
};
