// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GameFramework/Selectable.h"
#include "Statics/Structs_1.h"
#include "ResourceSpot.generated.h"

class ARTSGameState;
class ABuilding;
class AInfantry;
class AFactionInfo;
class UParticleSystemComponent;
class UDecalComponent;
class URTSGameInstance;
class USelectableWidgetComponent;
struct FSelectionDecalInfo;
class ARTSPlayerState;


/**
 *	A resource location. Only one unit can gather from it at a time (similar to minerals in SCII)
 *
 *	Some notes:
 *	- Selection decal size scales with mesh scale. To change its size change the decal components
 *	scale
 */
UCLASS()
class RTS_VER2_API AResourceSpot : public AActor, public ISelectable
{
	GENERATED_BODY()

public:

	AResourceSpot();

protected:

	virtual void BeginPlay() override;

public:

	/* Setup stuff for local player */
	void SetupForLocalPlayer(URTSGameInstance * InGameInstance, AFactionInfo * InFactionInfo,
		ETeam LocalPlayersTeam);

	//~ Begin ISelectable interface
	virtual void Setup() override;
	//~ End ISelectable interface 

private:

	/* Setup references and add to resource location list in game state */
	void Init();

	void SetupSelectionInfo();

	/* If adding a shape component as the root then consider changing AICon_GetBaseBuildRadius 
	to use it instead of the mesh */

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent * Mesh;

	/* Widget to display only when selected and visible. */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USelectableWidgetComponent * SelectionWidgetComponent;

	/* Widget to display all the time */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USelectableWidgetComponent * PersistentWidgetComponent;

	/* Selection decal to appear under unit when selected. Decal material does not need
	to be set through editor */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UDecalComponent * SelectionDecal;

	/* Partciles to appear when resource spot is selected */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UParticleSystemComponent * SelectionParticles;

	/* Resource type this is for. Cannot be "None" */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EResourceType Type;

	/* How many resources this spot starts with */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	uint32 Capacity;

	/* How close to spot a collector has to get for them to be able to gather resources, measured
	from this actors GetActorLocation() to something else */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float CollectionAcceptanceRadius;

#if WITH_EDITORONLY_DATA

	/* These are editor only because they are set on the FSelectableAttributes struct on 
	post edit and therefore not needed any other time */

	/* Name of unit to optionally appear in HUDs */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FText Name;

	/* Image to appear on HUDs when selecting this. Can be left null */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FSlateBrush HUDImage;

#endif // WITH_EDITORONLY_DATA

	/* An map holding the closest resource depot to this for
	each player */
	UPROPERTY()
	TMap < ARTSPlayerState *, ABuilding * > ClosestDepots;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to local players faction info */
	UPROPERTY()
	AFactionInfo * FI;

	// If true then local player is an observer, not a player
	bool bIsLocalPlayerObserver;

	/* True if mouse is hovering over this */
	bool bIsMouseHovering;

	/* How many resources still collectable from spot */
	uint32 CurrentAmount;

	/* Queue of collectors waiting to collect from this. Last = front of queue = collector
	collecting from here right now */
	UPROPERTY()
	TArray <AInfantry *> Queue;

	// Reference to selection decal info 
	const FSelectionDecalInfo * SelectionDecalInfo;

	/* Dummy attributes to pass to HUD. This struct may have many unneeded variables - could 
	try going one less in the heirarchy */
	UPROPERTY()
	FSelectableAttributesBasic Attributes;

	/* Returns the front of the queue, assuming queue has at least one element */
	AInfantry * GetQueueFront() const;

	/* Called when resources deplete
	@param GatherersPlayerState - player state of the selectable that took the last of the resources */
	void OnResourcesDepleted(ARTSPlayerState * GatherersPlayerState);

	/** 
	 *	Call function that returns void after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(AResourceSpot:: *Function)(), float Delay);

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

	/* Get the type of resources this spot is for */
	EResourceType GetType() const;

	/* Get how close to spot a collector has to get for them to be able to gather resources */
	float GetCollectionAcceptanceRadius() const;

	/* Returns true if there are no more resources at this spot */
	bool IsDepleted() const;

	/* Get the amount of resources left at this spot */
	uint32 GetCurrentResources() const;

	/* Called by a collector when it gets to this spot and wants to collect from it
	@param Collector - collector to register in queue
	@return - true if collector was first in queue and can collect straight away */
	bool Enqueue(AInfantry * Collector);

	/* Take resources from this spot, making sure to not let gatherer take more than there is
	@param GathererCapacity - how much of this resource the gatherer can hold
	@param GatherersPlayerState - owning player state of gatherer
	@return - the amount of resources gathered */
	uint32 TakeResourcesFrom(uint32 GathererCapacity, ARTSPlayerState * GatherersPlayerState);

	/* Called by a collector when it leaves the spot, either because it is done collecting
	resources or it was destroyed or it was commanded to do something else */
	void OnCollectorLeaveSpot(AInfantry * Collector);

	/* Get the closest depot to this for a certain player
	@param PlayerState - The player state whose depot we want to find is
	the closest
	@return - The cloest depot; nullptr if player has 0 depots */
	ABuilding * GetClosestDepot(ARTSPlayerState * PlayerState) const;

	/* Set the closest depot for a player, overriding the previous
	closest depot if there was one */
	void SetClosestDepot(ARTSPlayerState * PlayerState, ABuilding * Depot);

	/* This is used by CPU players when deciding how close to build their depots to this. */
	float AICon_GetBaseBuildRadius() const;

	//~ Begin ISelectable interface
	virtual class UBoxComponent * GetBounds() const override;
	virtual float GetBoundsLength() const override;
	virtual void OnSingleSelect() override;
	virtual uint8 GetSelectableID() const override;
	virtual uint8 OnDeselect() override;
	virtual void OnRightClick() override;
	virtual void ShowTooltip(URTSHUD * HUDWidget) const override;
	//~ End ISelectable interface

protected:

	void HideSelectionDecal();
	void ShowSelectionDecal();
	void ShowHoverDecal();

public:

	//~ Begin ISelectable interface
	virtual bool Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const override;
	virtual void OnMouseHover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnMouseUnhover(ARTSPlayerController * LocalPlayCon) override;
	virtual const FSelectableAttributesBase & GetAttributesBase() const override;
	virtual FSelectableAttributesBase & GetAttributesBase() override;
	virtual const FSelectableAttributesBasic * GetAttributes() const override;
	virtual bool HasAttack() const override;
	virtual bool CanClassGainExperience() const override;
	virtual const FInventory * GetInventory() const override;
	virtual void AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
	virtual void AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
#if WITH_EDITOR
	virtual bool PIE_IsForCPUPlayer() const override;
	virtual int32 PIE_GetHumanOwnerIndex() const override;
	virtual int32 PIE_GetCPUOwnerIndex() const override;
#endif
	//~ End ISelectable interface

	//~ Begin AActor interface
	virtual float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;
	//~ End AActor interface
};
