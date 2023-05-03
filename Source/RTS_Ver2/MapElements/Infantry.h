// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GameFramework/Selectable.h"
#include "Statics/Structs_1.h"
#include "Statics/Structs_2.h"
#include "Infantry.generated.h"

class ARTSGameState;
class AInfantryController;
class AProjectileBase;
class UStaticMeshComponent;
class UWidgetComponent;
class UDecalComponent;
class URTSGameInstance;
class ARTSPlayerController;
class ARTSPlayerState;
class AFactionInfo;
class UParticleSystemComponent;
class USelectableWidgetComponent;
class UBoxComponent;
struct FRankInfo;
class UFogObeyingAudioComponent;
class AInfantry;


/** 
 *	Contains all the informantion required to know whether the infantry is garrisoned inside 
 *	something and if so then what it is garrisoned inside of. 
 */
USTRUCT(Atomic) // Adding Atomic so the struct when sent over the wire it always sends both uint8 
				// instead of just 1. That's what that does right? Or is that the default behavior anyway? 
struct FInfantryGarrisonStatus
{
	GENERATED_BODY()

	friend AInfantry;

public:

	/* Ctor that should se the values such that the infantry is not inside any garrison */
	FInfantryGarrisonStatus()
		: GarrisonOwnerID(0)
		, GarrisonSelectableID(0)
	{}

	/** 
	 *	Set what selectable this unit is garrisoned inside of 
	 *	
	 *	@param EnteredSelectable - selectable this infantry is now garrisoned insdie of 
	 */
	void SetEnteredSelectable(ISelectable * EnteredSelectable);

	/* Signals unit is not inside a building */
	void ClearEnteredSelectable();

	/* Returns true if the unit is inside of a garrison */
	bool IsInsideGarrison() const { return GarrisonSelectableID != 0; }

	friend bool operator==(const FInfantryGarrisonStatus & Elem_1, const FInfantryGarrisonStatus & Elem_2)
	{
		return Elem_1.GarrisonOwnerID == Elem_2.GarrisonOwnerID && Elem_1.GarrisonSelectableID == Elem_2.GarrisonSelectableID;
	}

	friend bool operator!=(const FInfantryGarrisonStatus & Elem_1, const FInfantryGarrisonStatus & Elem_2)
	{
		return Elem_1.GarrisonSelectableID != Elem_2.GarrisonSelectableID || Elem_1.GarrisonOwnerID != Elem_2.GarrisonOwnerID;
	}

protected:

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	UPROPERTY()
	uint8 GarrisonOwnerID;

	/* Selectable ID of what this unit is garrisoned inside of */
	UPROPERTY()
	uint8 GarrisonSelectableID;
};


/*
Some how to's:
How to enable collecting resources:
- Under Attributes -> Resource Gathering Properties add an entry and set the key to the
resource type you want.
- In AI controller make sure Lenience_ResourceSpotCollection and Lenience_ResourceDepotDropOff
are large enough that the unit has no trouble getting close enough to resource spots/depots
to grab/deliver resources

Extra notes: in building blueprints make sure to add support for units being able to drop
off resources at it

- FObjectReplicator::ReplicateProperties figures out what to send in packet I think
*/

/** 
 *	Infantry class. 
 *	Infantry use the UE4 character class as a base.
 *
 *	Editing the capsule's bGeneratedOverlapEvents in editor will not work - it is always overriden 
 *	in post edit.
 */
UCLASS(Abstract)
class RTS_VER2_API AInfantry final : public ACharacter, public ISelectable
{
	GENERATED_BODY()

	friend AInfantryController;

public:

	AInfantry();

	/* Called when map is loaded and this is a part of it. Used for testing only */
	virtual void PostLoad() override;

protected:
	virtual void PostInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ID();

	UFUNCTION()
	void OnRep_CreationMethod();

	UFUNCTION()
	void OnRep_GameTickCountOnCreation();

	/* Use this function to setup a lot of selection info */
	virtual void OnRep_Owner() override;

public:

	//~ Begin ISelectable interface

	/* Setup a whole bunch of stuff once owner is known */
	virtual void Setup() override;

	//~ End ISelectable Interface

	virtual void Tick(float DeltaTime) override;

protected:

	void SetupSelectionInfo();

	/* Called when possessed */
	virtual void PossessedBy(AController * NewController) override;

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

private:

	/* Weapon mesh */
	//UPROPERTY(VisibleDefaultsOnly)
	//UStaticMeshComponent * WeaponMesh;

	/* Mesh to show as a piece of gathered resources. Only relevant if this is a resource
	gathering unit */
	UPROPERTY(VisibleDefaultsOnly)
	UStaticMeshComponent * HeldResourceMesh;

	/* Widget to display only when selected and visible. Leave user widget blank, it will be
	assigned from faction info */
	UPROPERTY(VisibleDefaultsOnly)
	USelectableWidgetComponent * SelectionWidgetComponent;

	/* Widget to display all the time when unit is visible. Leave user widget blank, it will be
	assigned from faction info */
	UPROPERTY(VisibleDefaultsOnly)
	USelectableWidgetComponent * PersistentWidgetComponent;

	/* Selection decal to appear under unit when selected. Decal material does not need
	to be set through editor */
	UPROPERTY(VisibleDefaultsOnly)
	UDecalComponent * SelectionDecal;

	/* The particle system to show when the unit is selected. Usually these play around their
	feet but can be anywhere really */
	UPROPERTY(VisibleDefaultsOnly)
	UParticleSystemComponent * SelectionParticles;

	/* Audio component that plays the 'attack preparation' sound. Possibly could use this to 
	play all sorts of sounds for this unit */
	UPROPERTY()
	UFogObeyingAudioComponent * AttackPreparationAudioComp;

	// May actually be able to make this editor only but not worrying about it for now
//#if WITH_EDITORONLY_DATA
	/* Type this unit is */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EUnitType Type;
//#endif

	/* If true then actor will not setup and will instead have an outside source set it up.
	Will only be true if using PIE and unit was placed in world via editor */
	bool bStartedInMap;

	/* Many core attributes */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties))
	FInfantryAttributes Attributes;

	/* Whether this unit can attack. Harvester type units are a good example of units that may
	want this to be false */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bHasAttack;

	/* How much yaw rotation unit can be off by for it to attack. Negative value means unit
	does not have to face target at all. For yaw only */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMax = "180.0"))
	float AttackFacingRotationRequirement;

	/* This is kind of an AIController property. If false then when unit does an attack they will
	keep the same yaw rotation until the anim notify OnAttackAnimationFinished is called.
	Issuing them anther order will cause their rotation to change

	My notes: could use a seperate anim notify */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bAllowRotationStraightAfterAttack;

	/* Attack attributes */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Attack Attributes", EditCondition = bHasAttack, ShowOnlyInnerProperties))
	FAttackAttributes AtkAttr;

#if WITH_EDITORONLY_DATA

	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (DisplayName = "Assign to CPU player"))
	bool PIE_bIsForCPUPlayer;

	/* For development only. When skipping main menu, this is the index of the player that will
	own this unit. ARTSDevelopmentSettings defines teams for each player, and is assigned in
	UMyGameInstance
	0 = server, 1 = remote client 1, 2 = remote client 2 and so on */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = bCanEditHumanOwnerIndex, ClampMin = 0))
	int32 PIE_HumanOwnerIndex;

	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = PIE_bIsForCPUPlayer, ClampMin = 0))
	int32 PIE_CPUOwnerIndex;

#endif

	/* Unique ID for issuing commands efficiently over the wire */
	UPROPERTY(ReplicatedUsing = OnRep_ID)
	uint8 ID;

	/* Whether this is a starting selectable or not */
	UPROPERTY(ReplicatedUsing = OnRep_CreationMethod)
	ESelectableCreationMethod CreationMethod;

	/* The custom RTS game tick count on the server when this unit was created. */
	UPROPERTY(ReplicatedUsing = OnRep_GameTickCountOnCreation)
	uint8 GameTickCountOnCreation;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health;

	/* If this is a resource gathering unit, the type of resource this unit is holding */
	UPROPERTY(ReplicatedUsing = OnRep_HeldResourceType)
	EResourceType HeldResourceType;

	/* The resource the unit was holding before HeldResourceType */
	EResourceType PreviousHeldResourceType;

	/* Amount of HeldResourceType held */
	uint32 HeldResourceAmount;

#if EXPERIENCE_ENABLED_GAME
	/* Rank or level. Starts at value defined in LevelUpOptions */
	uint8 Rank;

	/* Rank before leveling up to know how many levels gained */
	uint8 PreviousRank;
#endif

	UPROPERTY(ReplicatedUsing = OnRep_AnimationState)
	FAnimationRepVariable AnimationState;

	//~ TODO: make bitfield? And might need a lock to access if fog manager runs 
	// on seperate thread
	/* True if stealthed, but may still be visible due to stealth detectors */
	UPROPERTY(ReplicatedUsing = OnRep_bInStealthMode)
	uint8 bInStealthMode : 1;

	/* Timer handle to controle when to reenter stealth mode after leaving stealth mode */
	FTimerHandle TimerHandle_ReenterStealth;

	/* What selectable this unit is garrisoned inside of */
	UPROPERTY(ReplicatedUsing = OnRep_GarrisonStatus)
	FInfantryGarrisonStatus GarrisonStatus;

	/** [Client] What the unit was garrisoned in last time we received the replicated variable. 
	Could also keep this up to date on the server but there's no point */
	FInfantryGarrisonStatus ClientPreviousGarrisonStatus;

	/* Experience gained towards next rank. Not total experience gained */
	UPROPERTY(ReplicatedUsing = OnRep_ExperienceTowardsNextRank)
	float ExperienceTowardsNextRank;

	/* Range squared * RangeLenience. Used to speed up some
	calculations */
	float RangeSquared;

	/* Animation montages */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap < EAnimation, UAnimMontage * > Animations;

	/* The rate the move and moving with resources animations play */
	float MoveAnimPlayRate;

	/* Maps each context button to a timer handle that keeps
	track of the cooldown for the ability */
	UPROPERTY()
	TMap < FContextButton, FTimerHandle > ContextCooldowns;

	/* Fill ContextCooldowns with default constructed timer handles. So far this is for the HUD.
	An alternative method if to call call a function which gets the cooldown and adds timer
	handle to TMap if it is not already there */
	void InitContextCooldowns();

	/* Called by context action cooldown timer handles when they
	complete */
	void OnCooldownFinished();

	/* Reference to AI controller */
	UPROPERTY()
	AInfantryController * Control;

	/* A reference to local player controller. Not necessarilty the player
	controller that is controlling this */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* A reference to the player state of the player that controls this */
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

	/* Show the selection decal under this */
	void ShowSelectionDecal();

	/* Hide selection decal under this */
	void HideSelectionDecal();

	/* Show a reduced alpha version of the selection decal */
	void ShowHoverDecal();

	void PutAbilitiesOnInitialCooldown();

	void SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo);

	/* Set what color this unit should be */
	void SetColorOnMaterials();

	UFUNCTION()
	void OnRep_AnimationState();

	UFUNCTION()
	void OnRep_bInStealthMode();

	UFUNCTION()
	void OnRep_GarrisonStatus();

	UFUNCTION()
	void OnRep_HeldResourceType();

	UFUNCTION()
	void OnRep_ExperienceTowardsNextRank();

	/* Apply bonuses from leveling up */
	void ApplyLevelUpBonuses(uint8 Level);

	/** 
	 *	Play level up particles if any
	 *	
	 *	@param RankInfo - info for the level we want to show particles for
	 */
	void PlayLevelUpParticles(const FRankInfo & RankInfo);

	/** 
	 *	Play sound for leveling up 
	 *	
	 *	@param RankInfo - info for the level we want to play sound for
	 */
	void PlayLevelUpSound(const FRankInfo & RankInfo);

	/* Reference to dynamic material instances created for both main mesh and weapon mesh.
	Ordering is defined in InitStealthingVariables() */
	UPROPERTY()
	TArray <UMaterialInstanceDynamic *> DynamicMaterialInstances;

	/* Value to change params to when out of stealth mode */
	UPROPERTY()
	TArray <float> OutOfStealthParamValues;

	/* Called by timer handle server-side */
	void EnterStealthMode();

	// Set visibility when first spawned
	void SetUnitInitialVisibility(bool bInitialVisibility);

	void SetUnitVisibility(bool bNewVisibility);

	void SetUnitCollisionEnabled(bool bCollisionEnabled);

	// Called when entering/exiting fog of war
	void OnEnterFogOfWar();
	void OnExitFogOfWar();

	/* GetController() returns null in BeginPlay with networking */
	void AssignController(class AController * NewController);

	void InitStealthingVariables();

	void SetupWeaponAttachment();

	/* Setup references if there widgets are used */
	void SetupWidgetComponents();

	// UFUNCTION because we call this from a timer handle using an FName 
	UFUNCTION()
	void OnInventoryItemUseCooldownFinished(uint8 ServerInventorySlotIndex);

	int32 GetNumWeaponMeshMaterials() const;

	/** 
	 *	[Server] Enter a building's garrison. Assumes all checks whether the unit can enter 
	 *	have already been made.
	 *	
	 *	@param Building - building to enter 
	 *	@param GarrisonAttributes - Building's garrison attributes for convenience 
	 */
	void ServerEnterBuildingGarrison(ABuilding * Building, FBuildingGarrisonAttributes & GarrisonAttributes);

	/* Return the location we should set for the unit when it enters a garrison */
	FVector GetActorLocationForGarrison(ABuilding * GarrisonedBuilding) const;

public:

	/** 
	 *	Leave a garrison
	 *	
	 *	@param EvacLocation - location of this unit upon evac 
	 *	@param EvacRotation - rotation of this unit upon evac 
	 */
	void ServerEvacuateGarrison(const FVector & EvacLocation, const FQuat & EvacRotation);

private:

	/** 
	 *	Call function with no return after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(AInfantry:: *Function)(), float Delay);
	
	/** Version that will create a timer handle */
	void Delay(void(AInfantry:: *Function)(), float Delay);

	/* Fill Animations with structs: one for each animation type.
	This helps the user fill in the array by not having to select
	one of each type and try and keep track of what types need to be
	done */
	void InitAnimationsForConstructor();

	/* Makes turing smoother and can be adjusted with movement component's
	rotation rate. Also sets up avoidance for units running into each other */
	void InitMovementDefaults();

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	bool bCanEditHumanOwnerIndex;

#endif

#if WITH_EDITOR

	/* To change visibility of context button variables on changes in editor */
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;

#endif

	/* Set what our initial damage values are by querying faction info's struct about us. This 
	function was added to accomidate allowing damage values on to be defined either on this 
	or on projectiles */
	void SetDamageValues();

	//~ Begin ISelectable Interface

	/* Adjust stats/appearance for upgrades on spawn */
	virtual void AdjustForUpgrades() override;

	//~ End ISelectable interface

	/* The building that produced this unit. Will be null if the player started the match 
	with this unit */
	ABuilding * BuildingSpawnedFrom;

	/* The location of the rally point of the building that this unit was produced from */
	FVector BarracksRallyLoc;

public:

//#if WITH_EDITOR
	// Use GetAttributesBase().GetUnitType() in non-editor builds
	EUnitType GetType() const;
//#endif

	bool IsStartingSelectable() const;

	bool CheckAnimationProperties() const;

	/* Check if an animation has been set in editor */
	bool IsAnimationAssigned(EAnimation AnimType) const;

#if !UE_BUILD_SHIPPING
	/**
	 *	Verify that all the anim notifies required for the game to function for a particular 
	 *	animation montage are present on it. 
	 *	
	 *	@param OutErrorMessage - message that says which anim notifies are missing
	 *	@return - true if all anim notifies are present 
	 */
	static bool VerifyAllAnimNotifiesArePresentOnMontage(EAnimation AnimType, UAnimMontage * Montage, FString & OutErrorMessage);
#endif

	/* Play a random animation montage for the type
	@param AnimType - the type of animation you want to play
	@param bAllowReplaySameType - play the same montage again from
	the start even if it is already playing? */
	void PlayAnimation(EAnimation AnimType, bool bAllowReplaySameType);

	/* Get the rate an animation should play at */
	float GetAnimPlayRate(EAnimation AnimType) const;

	/* Return health percentage between 0.f and 1.f */
	float GetHealthPercentage() const;

	bool IsInsideGarrison() const;

	/* TODO: make some of these final like GetUnitType(). Do same with Building/vehicle
	classes etc */

	//~ Begin ISelectable interface
	virtual UBoxComponent * GetBounds() const override;
	virtual FSelectableRootComponent2DShapeInfo GetRootComponent2DCollisionInfo() const override;
	virtual void OnRightClickCommand(const FVector & Location) override;
	virtual void OnRightClickCommand(ISelectable * TargetAsSelectable, const FSelectableAttributesBase & TargetInfo) override;
	virtual void IssueCommand_MoveTo(const FVector & Location) override;
	virtual void IssueCommand_PickUpItem(const FVector & SomeLocation, AInventoryItem * TargetItem) override;
	virtual void IssueCommand_RightClickOnResourceSpot(AResourceSpot * ResourceSpot) override;
	virtual void OnSingleSelect() override;
	virtual EUnitType OnMarqueeSelect(uint8 & SelectableID) override;
	virtual EUnitType OnCtrlGroupSelect(uint8 & OutSelectableID) override;
	virtual uint8 OnDeselect() override;
	virtual uint8 GetSelectableID() const override;
	virtual bool Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const override;
	virtual void OnRightClick() override;
	virtual void OnMouseHover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnMouseUnhover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnEnterMarqueeBox(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnExitMarqueeBox(ARTSPlayerController * LocalPlayCon) override;
	virtual ETeam GetTeam() const override;
	virtual uint8 GetTeamIndex() const override;
	virtual const FContextMenuButtons * GetContextMenu() const override;
	virtual void GetInfo(TMap < EResourceType, int32 > & OutCosts, float & OutBuildTime, FContextMenuButtons & OutContextMenu) const override;
	virtual void OnLayFoundationCommand(EBuildingType BuildingType, const FVector & Location, const FRotator & Rotation) override;
	/* For two-click context commands */
	virtual void OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc, ISelectable * Target, const FSelectableAttributesBase & TargetInfo) override;
	virtual void OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc) override;
	/* For instant context commands */
	virtual void OnContextCommand(const FContextButton & Button) override;
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo) override;
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, const FVector & Location) override;
	virtual void IssueCommand_UseInventoryItem(uint8 InventorySlotIndex, EInventoryItem ItemType, const FContextButtonInfo & AbilityInfo, ISelectable * Target) override;
	virtual void IssueCommand_SpecialBuildingTargetingAbility(ABuilding * TargetBuilding, const FBuildingTargetingAbilityPerSelectableInfo * AbilityInfo) override;
	virtual void IssueCommand_EnterGarrison(ABuilding * TargetBuilding, const FBuildingGarrisonAttributes & GarrisonAttributes) override;
	virtual void OnContextMenuPlaceBuildingResult(ABuilding * PlacedBuilding, const FBuildingInfo & BuildingInfo, EBuildingBuildMethod BuildMethod) override;
	virtual void OnWorkedOnBuildingConstructionComplete() override;
	virtual float GetBoundsLength() const override;
	virtual float GetSightRadius() const override;
	virtual void OnUpgradeComplete(EUpgradeType UpgradeType, uint8 UpgradeLevel) override;
	virtual void OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes) override;
	virtual void Client_OnLevelGained(uint8 NewLevel) override;
	virtual void GetVisionInfo(float & SightRadius, float & StealRevealRadius) const override;
	virtual bool UpdateFogStatus(EFogStatus NewFogStatus) override;
	virtual bool IsInStealthMode() const override;
	virtual TSubclassOf <AProjectileBase> GetProjectileBP() const override;
	virtual void SetupBuildInfo(FUnitInfo & OutInfo, const AFactionInfo * FactionInfo) const override;
	virtual float GetHealth() const override;
	virtual const FAttachInfo * GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const override;
	virtual void SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding) override;
	virtual void SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter) override;
	virtual bool CanClassGainExperience() const override;
#if EXPERIENCE_ENABLED_GAME
	virtual float GetCurrentRankExperience() const override;
	virtual float GetTotalExperienceRequiredForNextLevel() const override;
	virtual float GetTotalExperienceRequiredForLevel(FSelectableRankInt Level) const override;
#endif
	virtual void GainExperience(float ExperienceBounty, bool bApplyRandomness) override;
	virtual uint8 GetOwnersID() const override;
	virtual void RegenSelectableResourcesFromGameTicks(uint8 NumGameTicksWorth, URTSHUD * HUDWidget, ARTSPlayerController * LocalPlayerController) override;
	virtual bool HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const override;

	/* Should return cooldown remaining, or -2.f if this does not support that button. */
	virtual float GetCooldownRemaining(const FContextButton & Button) override;
	virtual const TMap < FContextButton, FTimerHandle > * GetContextCooldowns() const override;
	virtual void OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility) override;
	virtual void OnInventoryItemUse(uint8 ServerInventorySlotIndex, const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility) override;
	virtual void StartInventoryItemUseCooldownTimerHandle(FTimerHandle & TimerHandle, uint8 ServerInventorySlotIndex, float Duration) override;
	virtual URTSGameInstance * Selectable_GetGI() const override;
	virtual ARTSPlayerState * Selectable_GetPS() const override;
	virtual bool HasZeroHealth() const override;
	virtual bool CanClassAcceptBuffsAndDebuffs() const override;
	virtual const TArray < FStaticBuffOrDebuffInstanceInfo > * GetStaticBuffArray() const override;
	virtual const TArray < FTickableBuffOrDebuffInstanceInfo > * GetTickableBuffArray() const override;
	virtual const TArray < FStaticBuffOrDebuffInstanceInfo > * GetStaticDebuffArray() const override;
	virtual const TArray < FTickableBuffOrDebuffInstanceInfo > * GetTickableDebuffArray() const override;
	virtual FStaticBuffOrDebuffInstanceInfo * GetBuffState(EStaticBuffAndDebuffType BuffType) override;
	virtual FStaticBuffOrDebuffInstanceInfo * GetDebuffState(EStaticBuffAndDebuffType DebuffType) override;
	virtual FTickableBuffOrDebuffInstanceInfo * GetBuffState(ETickableBuffAndDebuffType BuffType) override;
	virtual FTickableBuffOrDebuffInstanceInfo * GetDebuffState(ETickableBuffAndDebuffType DebuffType) override;
	virtual void RegisterDebuff(EStaticBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable) override;
	virtual void RegisterBuff(ETickableBuffAndDebuffType BuffType, AActor * BuffInstigator, ISelectable * InstigatorAsSelectable) override;
	virtual void RegisterDebuff(ETickableBuffAndDebuffType DebuffType, AActor * DebuffInstigator, ISelectable * InstigatorAsSelectable) override;
	virtual EBuffOrDebuffRemovalOutcome RemoveBuff(EStaticBuffAndDebuffType BuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) override;
	virtual EBuffOrDebuffRemovalOutcome RemoveDebuff(EStaticBuffAndDebuffType DebuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) override;
	virtual EBuffOrDebuffRemovalOutcome RemoveBuff(ETickableBuffAndDebuffType BuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) override;
	virtual EBuffOrDebuffRemovalOutcome RemoveDebuff(ETickableBuffAndDebuffType DebuffType, AActor * RemovalInstigator, ISelectable * InstigatorAsSelectable, EBuffAndDebuffRemovalReason RemovalReason) override;
	virtual uint8 GetAppliedGameTickCount() const override;
	virtual FVector GetActorLocationSelectable() const override;
	virtual float GetDistanceFromAnotherForAbilitySquared(const FContextButtonInfo & AbilityInfo, ISelectable * Target) const;
	virtual float GetDistanceFromLocationForAbilitySquared(const FContextButtonInfo & AbilityInfo, const FVector & Location) const;
	virtual ARTSPlayerController * GetLocalPC() const override;
	virtual bool HasAttack() const override;
	virtual const FShopInfo * GetShopAttributes() const override;
	virtual const FInventory * GetInventory() const override;
	virtual FInventory * GetInventoryModifiable() override;
	virtual float & GetHealthRef() override;
	virtual void ShowTooltip(URTSHUD * HUDWidget) const override;
	virtual USelectableWidgetComponent * GetPersistentWorldWidget() const override;
	virtual USelectableWidgetComponent * GetSelectedWorldWidget() const override;
	virtual void AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
	virtual void AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
	virtual void AttachParticles(const FContextButtonInfo & AbilityInfo, UParticleSystem * Template, ESelectableBodySocket AttachLocation, uint32 ParticleIndex) override;
	virtual void AttachParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo) override;
	virtual void AttachParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo) override;
	virtual void RemoveAttachedParticles(const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo) override;
	virtual void RemoveAttachedParticles(const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo) override;
	virtual FVector GetMuzzleLocation() const override;
	virtual bool CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const override;
	virtual const FBuildingTargetingAbilityPerSelectableInfo * GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const override;
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackHostileUnit(URTSGameInstance * GameInst, AActor * HostileAsActor, ISelectable * HostileUnit) const override;
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackFriendlyUnit(URTSGameInstance * GameInst, AActor * FriendlyAsActor, ISelectable * FriendlyUnit, EAffiliation UnitsAffiliation) const override;
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackHostileBuilding(URTSGameInstance * GameInst, ABuilding * HostileBuilding) const override;
	virtual const FCursorInfo * GetSelectedMouseCursor_CanAttackFriendlyBuilding(URTSGameInstance * GameInst, ABuilding * FriendlyBuilding, EAffiliation BuildingsAffiliation) const override;

#if WITH_EDITOR
	virtual bool PIE_IsForCPUPlayer() const override;
	virtual int32 PIE_GetHumanOwnerIndex() const override;
	virtual int32 PIE_GetCPUOwnerIndex() const override;
#endif
	//~ End ISelectable interface

	// Overridden from AActor
	virtual void FellOutOfWorld(const UDamageType & dmgType) override;

	/* Overridden from AActor */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;

	/* Overridden from ISelectable */
	virtual float TakeDamageSelectable(float DamageAmount, FDamageEvent const& DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;

	//~ Begin ISelectable interface
	virtual void Consume_BuildingTargetingAbilityInstigator() override;
	//~ End ISelectable interface

protected:

	UFUNCTION()
	void OnRep_Health();

	void OnZeroHealth();

	void FinalizeDestruction();

public:

	/* An anim notify that when called will make this unit exit stealth mode if they are
	a stealthing unit */
	void AnimNotify_ExitStealthMode();

	void AnimNotify_PlayAttackPreparationSound();

	/* Called when fire animation fires weapon. */
	void AnimNotify_FireWeapon();

	/* Called when weapon firing animation is finished. This signals that
	the unit can move again */
	void AnimNotify_OnAttackAnimationFinished();

	/* Called when a context action animation reaches point where it should
	carry out the action */
	void AnimNotify_ExecuteContextAction();

	/* Called when a context action animation reaches point where behavior will resume */
	void AnimNotify_OnContextAnimationFinished();

	/* Called to signal resources have been dropped off at depot */
	void AnimNotify_DropOffResources();

	void AnimNotify_TryPickUpInventoryItemOffGround();

	/* Called by an anim notify when the zero health animation has finished */
	void AnimNotify_OnZeroHealthAnimationFinished();

	/* Whether this unit can construct buildings (but not necessarily repair them) */
	bool CanBuildBuildings() const;

	/* Return whether this unit can harvest at least one type of resource */
	bool IsACollector() const;

	/* Return whether this unit can harvest a resource type */
	bool CanCollectResource(EResourceType ResourceType) const;

	/* Getters and setters, even though AIController is a friend anyway. Used
	for upgrades too though */

	virtual const FSelectableAttributesBase & GetAttributesBase() const override;
	virtual FSelectableAttributesBase & GetAttributesBase() override;
	virtual const FSelectableAttributesBasic * GetAttributes() const override;
	virtual const FInfantryAttributes * GetInfantryAttributes() const override;
	virtual const FAttackAttributes * GetAttackAttributes() const override;

	/* Non-const versions. Added here so upgrade effects can modify things */
	virtual FSelectableAttributesBasic * GetAttributesModifiable() override;
	FInfantryAttributes * GetInfantryAttributesModifiable();
	virtual FAttackAttributes * GetAttackAttributesModifiable() override;

	/* Set whether this unit has an attack or not */
	void SetHasAttack(bool bCanNowAttack);
	
	/** Get the unit's rank or level */
	virtual uint8 GetRank() const override;
	
	/** Get the faction the player who owns this unit is playing as */
	EFaction GetFaction() const;

	const FName & GetTeamTag() const;

	/* Set the play rate for the movement animation */
	void SetMoveAnimPlayRate();

	/* [Server] Get the movespeed taking into account any temporary effects too */
	float GetMoveSpeed() const;

	/** 
	 *	[Server] Get movespeed not taking into account any temporary effects. This value WILL take 
	 *	into account any upgrades or level up bonuses 
	 */
	float GetDefaultMoveSpeed() const;

	/* Get the unit's default move speed that they started the match with. This never changes */
	float GetStartingMoveSpeed() const;

private:

	/** 
	 *	[Server] Set what the new default move speed is but do not apply it 
	 */
	void Internal_SetNewDefaultMoveSpeed(float NewValue);

public:

	/** 
	 *	[Server] Set what the new default movespeed is. This is for a permanent change in movespeed. 
	 *	Call this when applying upgrades or level up bonuses. Also applies it using percentage 
	 *	rules
	 *	
	 *	@param NewMoveSpeed - new default move speed
	 *	@return - the new current movespeed after the default has been change, not the default 
	 *	movespeed
	 */
	float SetNewDefaultMoveSpeed(float NewMoveSpeed);
	
	/** 
	 *	[Server] Version of SetNewDefaultMoveSpeed that takes a multiplier instead 
	 *	
	 *	@param Multiplier - how much to multiply default movespeed by 
	 *	@return - the new current movespeed after the default has been change, not the default 
	 *	movespeed
	 */
	float SetNewDefaultMoveSpeedViaMultiplier(float Multiplier);

	/**
	 *	[Server] Apply a movespeed modifier that is considered only temporary.
	 *	
	 *	@param Multiplier - amount to multiply current movespeed by 
	 *	@return - move speed after the modifier is applied 
	 */
	virtual float ApplyTempMoveSpeedMultiplier(float Multiplier) override;

	/** 
	 *	[Server] Remove a movespeed modifier that is considered only temporary 
	 *
	 *	@param Multiplier - amount to multiply current movespeed by 
	 *	@return - move speed after the modifier is removed 
	 */
	virtual float RemoveTempMoveSpeedMultiplier(float Multiplier) override;

protected:

	/** 
	 *	[Server] Reset movespeed back to what it should be with no resources being held 
	 *	
	 *	@return - move speed after dropping resource 
	 */
	float RemoveMoveSpeedEffectOfResources();

public:

	/** 
	 *	[Server] Make the unit enter stealth mode temporarily 
	 *	
	 *	@return - whether the unit is in stealth mode after the change, so true every time 
	 */
	virtual bool ApplyTempStealthModeEffect() override;
	
	/** 
	 *	[Server] Remove the effects of a temporary enter stealth mode effect 
	 *	
	 *	@return - whether the unit is in stealth mode after the change
	 */
	virtual bool RemoveTempStealthModeEffect() override;

	const TMap < EResourceType, FResourceCollectionAttribute > & GetResourceGatheringProperties() const;
	
	/* Get by how much to multiply movement speed by when carrying a resource */
	float GetMoveSpeedMultiplierForHoldingResources() const;
	
	/* Return whether this unit is holding any resources */
	bool IsHoldingResources() const;

	/* Get the resource this unit is holding. Will return "None" if not holding any resource */
	EResourceType GetHeldResourceType() const;
	void SetHeldResource(EResourceType NewHeldResource, uint32 Amount);
	uint32 GetHeldResourceAmount() const;
	float GetResourceGatherRate(EResourceType ResourceType) const;

	/* Get the max amount of a resource type this unit can carry */
	uint32 GetCapacityForResource(EResourceType ResourceType) const;

	/** Get the AI controller that controls this unit. Has nothing to do with CPU players */
	AInfantryController * GetAIController() const;

#if WITH_EDITOR
	void DisplayAIControllerOnScreenDebugInfo();
#endif

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_OnAttackMade(AActor * Target);
};
