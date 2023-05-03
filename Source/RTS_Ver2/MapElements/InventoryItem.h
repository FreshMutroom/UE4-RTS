// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Statics/Structs_1.h" // For selectable attributes
#include "Statics/Structs_4.h"
#include "GameFramework/Selectable.h"
#include "InventoryItem.generated.h"

struct FPropertyChangedChainEvent;
enum class EInventoryItem : uint8;
class UBillboardComponent;
class URTSGameInstance;
struct FInventoryItemInfo;
struct FSelectionDecalInfo;
class UParticleSystemComponent;
class ARTSPlayerController;
class UAnimMontage;


/**  
 *	Inventory items are items that can go in the inventory of a selectable.
 *	This class is an inventory item that has been placed/dropped/whatever into the world.
 *
 *	You can place these into maps to have them start there when the match begins, but they 
 *	only show as a sprite while during design time. Make sure to set their type in their blueprint.
 *
 *	There's no shape component used for selection - it's up to the mesh. Can't you make a custom 
 *	physics asset that can be like a cube and use that for the mesh's collision?
 *
 *	There is an expectation that this actor is visible so if you make blueprints of this class 
 *	do not adjust their visibilities.
 *
 *-----------------------------------------------------------------------------------------------
 *	My notes: can't quite get the functionality that I want in regards to meshes. 
 *	What I want is:
 *	- you define the mesh you want to use for an item type in GI blueprint
 *	- you can place these actors into the world. They will have that mesh that was set in GI BP.
 *
 *	In post edit world and GI are null. Would be able to accomplish this by:
 *	- learning how to modify another BP in post edit e.g. we edit this but also modify GI BP
 *	- writing the mesh reference to disk meaning in post edit we can access it. Maybe a 
 *	data table could accomplish this I don't know though, I know nothing about data tables.
 *
 *	So in the mean time its always the default cube in editor map during design time.
 *-----------------------------------------------------------------------------------------------
 */
UCLASS(Abstract, NotBlueprintable, 
	HideCategories = "Actor Tick", HideCategories = Rendering, HideCategories = Replication,
	HideCategories = Input, HideCategories = Actor, HideCategories = LOD, HideCategories = Cooking, 
	HideCategories = Collision)
class RTS_VER2_API AInventoryItem : public AActor, public ISelectable
{
	GENERATED_BODY()
	
public:	
	
	AInventoryItem();

protected:
	
	virtual void BeginPlay() override;
	
public:

	// NOOP currently, SetupStuff() does all stuff
	virtual void Setup() override;

	/* This function was made for the items that already start on the map */
	virtual void SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState, bool bMakeHidden);

protected:

	void SetupSomeStuff(const URTSGameInstance * GameInstance);

	void SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo, const URTSGameInstance * GameInst);
	void SetupParticles(AFactionInfo * LocalPlayersFactionInfo);

	//==========================================================================================
	//	Components
	//==========================================================================================

	/* Decal that appears under this when hovered/selected */
	UPROPERTY(VisibleDefaultsOnly)
	UDecalComponent * SelectionDecalComp; 

	/* Particle system to show when this is selected. Also shows the particles for when this 
	item is right-clicked. 
	Donit use infinite looping templates for this */
	UPROPERTY(VisibleDefaultsOnly)
	UParticleSystemComponent * SelectionParticlesComp;

	/* Templates for particle systems. Not strictly components but here for cache locality */
	UParticleSystem * SelectionParticlesTemplate;
	UParticleSystem * RightClickParticlesTemplate;

	//===========================================================================================
	//	Other stuff
	//===========================================================================================

	/** ID. There should never be a situation where two items on the map share the same ID. 
	If there is then we have problems */
	UPROPERTY()
	FInventoryItemID UniqueID;

	/** 
	 *	The type of item this is for. 
	 *	
	 *	My notes: this currently will not change the appearance of the item during design time. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EInventoryItem Type;

	/** 
	 *	How many of this item there are in this stack OR how many charges the item has. But 
	 *	if you're looking at this in editor blueprint then treat it as num in stack. 
	 */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (ClampMin = 1))
	int16 Quantity;

	/* How many charges the item has. Might be irrelevant if the item does not have charges */
	int16 NumItemCharges;

	/* Acceptance radius amount for orders to pick this up */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float AcceptanceRadius;

	/* The info struct that this item is for */
	const FInventoryItemInfo * ItemInfo;

	FSelectableAttributesBase Attributes;

	/* Info about the selection decal we're using */
	const FSelectionDecalInfo * SelectionDecalInfo;

	void ExitPool_BasicStuff(ARTSGameState * GameState, EInventoryItem InItemType, int16 InQuantity,
		int16 InNumItemCharges, const FInventoryItemInfo & InItemsInfo);

	void ExitPool_FinalStuff(ARTSGameState * GameState);

	/* Return whether this is a blueprint class */
	bool IsBlueprintClass() const;

public:

	/* Set the visibility of this actor.
	SetActorHiddenInGame is not an option because AActor::bHidden is replicated. If I
	eventually edit engine source then this func can just call SetActorHiddenInGame */
	virtual void SetVis(bool bIsVisible, bool bAffectsSelectionDecal = false);

	FInventoryItemID GetUniqueID() const;

	/* Get the type of item this actor is for */
	EInventoryItem GetType() const;

	/* Get how many of the item is in this stack */
	int16 GetItemQuantity() const;

	int16 GetNumItemCharges() const;

	/* Get the info struct for the item that this actor is for */
	const FInventoryItemInfo * GetItemInfo() const;

	/* Return whether we think this object is in the object pool. If this returns true we 
	shouldn't really be doing anything with it that's not object pool related like trying 
	to pick it up with a selectable */
	bool IsInObjectPool() const;

	/** 
	 *	Return whether another selectable is close enough to this item to be able to pick it up. 
	 *	
	 *	@param Selectable - the selectable we would like to know whether it is close enough 
	 */
	template <typename TSelectable>
	bool IsSelectableInRangeToPickUp(TSelectable * Selectable) const
	{
		/* Probably want to keep the acceptance radius of the AI controller's move to this in mind
		when choosing what to return here */

		const float Distance = (Selectable->GetActorLocation() - GetActorLocation()).Size();
		
		return Distance - Selectable->GetBoundsLength() - AcceptanceRadius < 0.f;
	}

	virtual void SetupForEnteringObjectPool(const URTSGameInstance * GameInstance);

	/** 
	 *	Called when this item is picked up by a selectable. 
	 *	
	 *	@param SelectableThatPickedUsUp - selectable that picked this up 
	 *	@param GameState - reference to the game state 
	 *	@param LocalPlayCon - GetWorld()->GetFirstPlayerController()
	 */
	virtual void OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState, 
		ARTSPlayerController * LocalPlayCon);

#if !UE_BUILD_SHIPPING
	/* Verify this actor is in a state that it is acceptable to enter the object pool e.g. tick
	is disabled, is hidden, etc */
	virtual bool IsFitForEnteringObjectPool() const;
#endif

	/* Set the visibility of this actor. This is called from the fog manager */
	void SetVisibilityFromFogManager(bool bInIsVisible);

	//==========================================================================================
	//	Begin ISelectable Interface
	//==========================================================================================

	virtual const FSelectableAttributesBase & GetAttributesBase() const override;
	virtual FSelectableAttributesBase & GetAttributesBase() override;
	virtual const FSelectableAttributesBasic * GetAttributes() const override;
	virtual void OnMouseHover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnMouseUnhover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnSingleSelect() override;
	virtual uint8 OnDeselect() override;
	virtual bool Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const override;
	virtual void OnRightClick() override;
	virtual void ShowTooltip(URTSHUD * HUDWidget) const override;

	virtual bool HasAttack() const override;
	virtual bool CanClassGainExperience() const override;
	virtual const FShopInfo * GetShopAttributes() const override;
	virtual const FInventory * GetInventory() const override;

#if WITH_EDITOR
	virtual bool PIE_IsForCPUPlayer() const override;
	virtual int32 PIE_GetHumanOwnerIndex() const override;
#endif // WITH_EDITOR

	//==========================================================================================
	//	End ISelectable Interface
	//==========================================================================================

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};


/* Inventory item actor class that uses a static mesh */
UCLASS(Blueprintable)
class RTS_VER2_API AInventoryItem_SM final : public AInventoryItem
{
	GENERATED_BODY() 

public:

	AInventoryItem_SM();

	virtual void SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState, bool bMakeHidden) override;

	virtual void SetupForEnteringObjectPool(const URTSGameInstance * GameInstance) override;

protected:
	
	/* Mesh for this item. */
	UPROPERTY(VisibleDefaultsOnly)
	UStaticMeshComponent * Mesh;

public:

	virtual void SetVis(bool bIsVisible, bool bAffectsSelectionDecal = false) override;

	/* @param InItemType - the item type this actor will represent in the world
	@param InQuantity - how mnay of InItemType are in this stack. Should be at least 1
	@param InItemsInfo - info struct for InItemInfo as a convenience */
	void ExitPool(ARTSGameState * GameState, EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, 
		const FInventoryItemInfo & InItemsInfo, const FVector & Location, const FRotator & Rotation, 
		EItemEntersWorldReason ReasonForSpawning, bool bMakeVisible);

	/* Version where we do not set the location/rotation (although we still need to move it based
	on InItemsInfo mesh variables). This is called when object pooling manager has to spawn a
	new actor because pool is empty */
	void ExitPool_FreshlySpawnedActor(URTSGameInstance * GameInst, ARTSGameState * GameState,
		EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, 
		const FInventoryItemInfo & InItemsInfo, EItemEntersWorldReason ReasonForSpawning, bool bMakeHidden);

	virtual void OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState,
		ARTSPlayerController * LocalPlayCon) override;

#if !UE_BUILD_SHIPPING
	/* Verify this actor is in a state that it is acceptable to enter the object pool e.g. tick
	is disabled, is hidden, etc */
	virtual bool IsFitForEnteringObjectPool() const override;
#endif
};


/** 
 *	Inventory item actor class that uses a skeletal mesh. 
 *	
 *-------------------------------------------------------------------------------------------------
 *	My notes about ticking:
 *	Mesh will tick while on map if and only if an OnDropped animation was played 
 *-------------------------------------------------------------------------------------------------
 */
UCLASS(Blueprintable)
class RTS_VER2_API AInventoryItem_SK final : public AInventoryItem
{
	GENERATED_BODY()

public:

	AInventoryItem_SK();

	virtual void SetupStuff(const URTSGameInstance * GameInstance, ARTSGameState * GameState, bool bMakeHidden) override;

	virtual void SetupForEnteringObjectPool(const URTSGameInstance * GameInstance) override;

protected:
	
	UPROPERTY(VisibleDefaultsOnly)
	USkeletalMeshComponent * Mesh;

public:

	virtual void SetVis(bool bIsVisible, bool bAffectsSelectionDecal = false) override;

	/* @param InItemType - the item type this actor will represent in the world
	@param InQuantity - how mnay of InItemType are in this stack. Should be at least 1
	@param InItemsInfo - info struct for InItemInfo as a convenience */
	void ExitPool(ARTSGameState * GameState, EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, 
		const FInventoryItemInfo & InItemsInfo, const FVector & Location, const FRotator & Rotation, 
		EItemEntersWorldReason ReasonForSpawning, bool bMakeVisible);

	/* Version where we do not set the location/rotation (although we still need to move it based
	on InItemsInfo mesh variables). This is called when object pooling manager has to spawn a
	new actor because pool is empty */
	void ExitPool_FreshlySpawnedActor(URTSGameInstance * GameInst, ARTSGameState * GameState,
		EInventoryItem InItemType, int16 InQuantity, int16 InNumItemCharges, 
		const FInventoryItemInfo & InItemsInfo, EItemEntersWorldReason ReasonForSpawning, bool bMakeHidden);

	virtual void OnPickedUp(ISelectable * SelectableThatPickedUsUp, ARTSGameState * GameState,
		ARTSPlayerController * LocalPlayCon) override;

#if !UE_BUILD_SHIPPING
	/* Verify this actor is in a state that it is acceptable to enter the object pool e.g. tick
	is disabled, is hidden, etc */
	virtual bool IsFitForEnteringObjectPool() const override;
#endif
};
