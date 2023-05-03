// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GameFramework/Selectable.h"
#include "Statics/Structs_1.h"
#include "Building.generated.h"

class UBoxComponent;
class USkeletalMeshComponent;
class ARTSGameState;
class AGhostBuilding;
class URTSGameInstance;
class ARTSPlayerState;
class ARTSPlayerController;
class AFactionInfo;
class AController;
class USceneComponent;
class UDecalComponent;
class UWidgetComponent;
class UParticleSystemComponent;
class USelectableWidgetComponent;
class UArrowComponent;
class UBillboardComponent;
class UStaticMeshComponent;
class UMySphereComponent;
struct FPropertyChangedChainEvent;
class UFogObeyingAudioComponent;
class IBuildingAttackComp_Turret;


/* Stores the animation state of the building */
USTRUCT()
struct FBuildingAnimState
{
	GENERATED_BODY()

public:

	// World time when animation started
	UPROPERTY()
	float StartTime;

	// Type of animation
	UPROPERTY()
	EBuildingAnimation AnimationType;

	// Constructor
	FBuildingAnimState()
		: StartTime(0.f)
		, AnimationType(EBuildingAnimation::None)
	{
	}
};


/* Information about what/when animation was paused */
USTRUCT()
struct FBuildingAnimPauseInfo
{
	GENERATED_BODY()

public:

	/* World time when animation started playing */
	float StartTime;

	// Type of animation that was paused
	EBuildingAnimation AnimationType;

	// Constructor
	FBuildingAnimPauseInfo()
		: AnimationType(EBuildingAnimation::None)
	{
	}
};


/** 
 *	A struct that stores the info about this building while it is being constructed. Using the 
 *	info in this struct it is possible to derive how far along the construction of this building is.
 *	
 *	If structs do not rep all at once (i.e. if both variables in this struct change but UE is 
 *	allowed to send only one of the two variables in this struct) then we have problems. 
 *	This article says they do, it is about 3 years old:
 *	https://answers.unrealengine.com/questions/234803/problems-with-the-order-replicated-variables-repli.html
 *
 *	If they don't both update at once then it *may* be possible, haven't thought about it thouroughly
 */
USTRUCT()
struct FConstructionProgressInfo
{
	GENERATED_BODY()

public:

	FConstructionProgressInfo() 
		: PercentageCompleteWhenConstructionLastStopped(0.f) 
		, LastTimeWhenConstructionStarted(-1.f)
	{
	}

	/* How far along construction was when it was halted. Range: [0, 1] */
	UPROPERTY()
	float PercentageCompleteWhenConstructionLastStopped;

	/* The world time when construction resumed. A value less than 0 means that the building 
	is currently not being worked on */
	UPROPERTY()
	float LastTimeWhenConstructionStarted;
};


/**
 *	Building terms:
 *	Constructed: refers to not being usable yet
 *	Being built: refers to if its construction is progressing, either because a worker is working
 *	on it or it uses a EBuildingBuildMethod that causes it to build itself
 *
 *	Tips:
 *	- Avoid editing Scale3D of Bounds, instead edit Extent. This will prevent particle effects,
 *	rally point etc from scaling too
 *	- Set blend in/out time for all anim montages to 0. By default I think it is something like 0.25.
 *	This can be done by editing every single anim montage asset, category = "Blend Option".
 */


/* My notes: 
- some OnRep funcs may need to check if HasBeenFullyDestroyed() */
UCLASS(Abstract)
class RTS_VER2_API ABuilding : public AActor, public ISelectable
{
	GENERATED_BODY()

	/* How long to spend sinking into the ground when the Destroyed animation finishes or 
	if no destroyed anim is set then from the time it reaches zero health */
	static const float TimeToSpendSinking;

public:
	
	ABuilding();

	virtual void PostLoad() override;

protected:

	virtual void PreInitializeComponents() override;

	virtual void PostInitializeComponents() override;

	void RunStartOfBeginPlayLogic();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ID();

	/* Even if not a starting selectable this will need to be called so we know for sure */
	UFUNCTION()
	void OnRep_CreationMethod();

	virtual void OnRep_Owner() override;

	/* This function being called can also be used as a signal that construction has completed */
	UFUNCTION()
	void OnRep_GameTickCountOnConstructionComplete();

	UFUNCTION()
	void OnRep_AnimationState();

	void PlayAnimInternal();

	//~ Begin ISelectable interface
	virtual void Setup() override;
	//~ End ISelectable interface

	void SetupSelectionInfo();

	void AdjustForUpgrades();

	/* Put any context buttons that have an initial cooldown on cooldown */
	void PutAbilitiesOnInitialCooldown();

	void SetupSelectionDecal(AFactionInfo * LocalPlayersFactionInfo);

	/* Set what color mesh should be given who owner is */
	void SetMeshColors();

	void SetupWidgetComponents();

	void SetupBuildingAttackComponents();

	/** 
	 *	Set inital values such as health 
	 *	
	 *	@param BuildingInfo - info struct for this building 
	 */
	void SetupInitialConstructionValues(const FBuildingInfo * BuildingInfo);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	void ProgressSinkingIntoGround(float DeltaTime);

	// Same deal here as with AInfantry::Type
//#if WITH_EDITORONLY_DATA
	/* Type of building this is. Each building in a faction info's building roster must have a
	unique type */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuildingType Type;
//#endif

	//=========================================================================================
	//	Components
	//=========================================================================================

	// Bounds for selecting with mouse.
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UBoxComponent * Bounds;

	// TODO ghost building needs regular Mesh comp too
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USkeletalMeshComponent * Mesh;

	/* Widget to display only when selected and visible. Leave user widget blank, it will be
	assigned from faction info */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USelectableWidgetComponent * SelectionWidgetComponent;

	/* Widget to display all the time when unit is visible. Leave user widget blank, it will be
	assigned from faction info */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USelectableWidgetComponent * PersistentWidgetComponent;

	/* Decal that appears under this when selected */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UDecalComponent * SelectionDecal;

	/* Particles to show when selected or right-clicked */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UParticleSystemComponent * SelectionParticles;

	//----------------------------------------------------------------
	//	Components for Unit Spawning
	//----------------------------------------------------------------

	/* The unit spawns at UnitSpawnLoc. It then moves to UnitFirstMoveLoc, then to
	UnitRallyLoc. UnitRallyLoc can be changed by the player during a match.
	If you want the unit to spawn and not move at all then set all 3 locations to be the same */

	// Location where unit spawns
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USceneComponent * UnitSpawnLoc;

	// Location unit first moves to after spawning
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USceneComponent * UnitFirstMoveLoc;

	/* Location unit moves to after moving to UnitFirstMoveLoc. This can be changed by the
	player during a match */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent * UnitRallyPoint;

#if WITH_EDITORONLY_DATA

	/* Arrow component to go on UnitSpawnLoc to help better know transform. Rotation from this
	will also be used when spawning unit */
	UPROPERTY()
	UArrowComponent * UnitSpawnLocArrow;

	/* Sprite to better locate UnitFirstMoveLoc in editor */
	UPROPERTY()
	UBillboardComponent * UnitFirstMoveSprite;

	/* Sprite to better locate UnitDefaultRallyPoint in editor */
	UPROPERTY()
	UBillboardComponent * UnitRallySprite;

#endif

	TWeakObjectPtr<UFogObeyingAudioComponent> BeingWorkedOnAudioComp;

	/* Really tried to get these spawn locations to use just FTransforms and FVectors instead.
	USceneComponent::PostEditComponentMove gets called but doesn't update the property - so
	strange. Setting it in PostEditChainChangedProperty works but isn't called when user
	moves component around in viewport. Tried using pointer to FVector - didn't work. Tried
	setting UPROPERTY value by name - didn't work. */


	//=========================================================================================
	//	Not Components
	//=========================================================================================

	/* BP for the ghost building that appears when choosing where to
	place this. It should have the exact same mesh properties but its
	material should be overridden with a translucent one */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf < AGhostBuilding > Ghost_BP;

	/* Core attributes */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Core Attributes", ShowOnlyInnerProperties))
	FBuildingAttributes Attributes;

	/* This was added for clients cause OnRep_ funcs can come in before BeginPlay is called */
	uint8 bHasRunStartOfBeginPlay : 1;

	/* If true then actor will not setup and will instead have an outside source set it up.
	Will only be true if using PIE and unit was placed in world via editor */
	uint8 bStartedInMap : 1;

#if WITH_EDITORONLY_DATA

	/* Whether to use human player or CPU player index for PIE */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (DisplayName = "Assign to CPU player"))
	uint8 PIE_bIsForCPUPlayer : 1;

	/* For development only. When skipping main menu, this is the index of the player that will
	own this unit. ARTSDevelopmentSettings defines teams for each player, and is assigned in
	UMyGameInstance
	0 = server, 1 = remote client 1, 2 = remote client 2 and so on.
	If the index is set higher than the number of players testing with in PIE then building
	will either be assigned to server player or removed from PIE completely */
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = bCanEditHumanOwnerIndex, ClampMin = 0))
	int32 PIE_HumanOwnerIndex;

	// Same as above but for CPU player
	UPROPERTY(EditInstanceOnly, Category = "RTS|Development", meta = (EditCondition = PIE_bIsForCPUPlayer, ClampMin = 0))
	int32 PIE_CPUOwnerIndex;

#endif // WITH_EDITORONLY_DATA

	/* Unique ID for issuing commands efficiently over the wire. If raw pointers are more 
	bandwidth friendly then this variable should be removed */
	UPROPERTY(ReplicatedUsing = OnRep_ID)
	uint8 ID;

	/* Whether this building was spawned as a starting selectable or not. Important because if
	it was then clients need to know to construct it instantly. Also an extra copy of this
	variable is defined in FSelectableAttributes */
	UPROPERTY(ReplicatedUsing = OnRep_CreationMethod)
	ESelectableCreationMethod CreationMethod;

	/* Struct that hols enough information to derive how far along construction is */
	UPROPERTY(ReplicatedUsing = OnRep_ConstructionProgressInfo)
	FConstructionProgressInfo ConstructionProgressInfo;

	/* The custom RTS game tick count on the server when this unit was created. */
	UPROPERTY(ReplicatedUsing = OnRep_GameTickCountOnConstructionComplete)
	uint8 GameTickCountOnConstructionComplete;

	/* [Client] If true then Client_OnConstructionComplete has run. 
	Basically this exists because 2+ OnRep variables can rep through on same frame */
	bool bHasRunOnConstructionCompleteLogic;

	/* Animation state of building */
	UPROPERTY(ReplicatedUsing = OnRep_AnimationState)
	FBuildingAnimState AnimationState;

	/* Holds info about what/when animation was paused */
	FBuildingAnimPauseInfo PausedAnimInfo;

	/* Location of rally point */
	UPROPERTY(ReplicatedUsing = OnRep_RallyPointLocation)
	FVector_NetQuantize RallyPointLocation;

	UFUNCTION()
	void OnRep_RallyPointLocation();

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health;

	float TimeIntoZeroHealthAnimThatAnimNotifyIs;

	UFUNCTION()
	void OnRep_ConstructionProgressInfo();

	/* Because while being built health will change rapidly we do not want a ton of updates being
	sent to clients. Therefore this health value will be used as the buildings health while being
	built. Whenever damage is taken while being built the var Health will be updated. From there
	clients can work out how much health the building has and continue updating health on its own
	while it is being built */
	float HealthWhileUnderConstruction;

	/* Because of the nature of tick, need this so buildings health is not slightly off when it
	has finished construction */
	float DamageTakenWhileUnderConstruction;

	/* Reference to the player state of the player that controls this */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to faction info of player state */
	UPROPERTY()
	AFactionInfo * FI;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to game instance */
	UPROPERTY()
	URTSGameInstance * GI;

	/* Reference to local player controller. This is here for the defense structures */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Maps button to its timer handle that handles its cooldown */
	UPROPERTY()
	TMap<FContextButton, FTimerHandle> ContextActionCooldowns;

	/* Set of workers working on this building, constructing it. Unsure if multiple builders
	will work */
	UPROPERTY()
	TSet < AActor * > Builders;

	/* Called every tick. Progresses construction if this is being built */
	void ProgressConstruction(float DeltaTime);

	/* Called when construction is complete */
	void Server_OnConstructionComplete();
	
	/* Some different functions for when construction completes on the client. */
	void Client_SetAppearanceForConstructionComplete();
	void Client_FinalizeConstructionComplete();
	void Client_OnConstructionComplete();

	void SetInitialBuildingVisibility();
	void SetInitialBuildingVisibilityV2();

	// These 2 funcs are probably not good to call anywhere other than during Setup()
	void SetBuildingVisibility(bool bNewVisibility);
	void SetBuildingVisibilityV2(bool bNewVisibility);

	void HideBuilding();

	// Called when transitioning between in/out fog
	void OnEnterFogOfWar();
	void OnExitFogOfWar();

	/* Play the particles that should play when this building is placed into the world. They 
	should not be looping */
	void PlayJustPlacedParticles();

	/** 
	 *	Play the sound that should be played when the building has just been placed into the world  
	 */
	void PlayJustPlacedSound();

	/* Only relevant while being built. 
	Equal to MaxHealth * (1 - PC->GetBuildingStartHealthPercentage()). Used
	each tick while being built to determine how much health to gain */
	float HealthGainRate;

	/* Location when construction was started. Only relevant if not a starting selectable. 
	Needed to know ending location when construction is complete */
	FVector SpawnLocation;

	/* Set some values for production queues on begin play */
	void InitProductionQueues();

	/* The component that plays the 'progressing construction' particles. Spawned by 
	Statics::SpawnFogParticles */
	UPROPERTY()
	TWeakObjectPtr < UParticleSystemComponent > BeingBuiltParticleComponent;

	/* This bool is to know whether BeingBuiltParticleComponent is playing or not. I can't 
	seem to find one in UParticleSystemComponent so I'll use this */
	bool bIsBeingBuiltParticlesRunning;

	/* Set whether this building is having its construction progressed or not */
	void Server_SetBeingBuilt(bool bBeingBuilt);

	/* Play the particles that should play when this building is having its construction 
	progress advanced. The particles should be looping */
	void PlayBeingConstructedParticles();

	/* Stop emitting the particles for when this building is having its construction progress 
	advanced */
	void StopBeingConstructedParticles();

	/* Functions for the sound when the building is being worked on */
	void PlayBeingConstructedSound(float AudioStartTime);
	void StopBeingConstructedSound();

	/* If true, while construction is progressing the building will rise from the ground. This
	will be set true if no construction animation is set */
	bool bRiseFromGround;

	// How long this has been building for
	float TimeSpentBuilding;

	/* [Server] If this building has any attack components, make them start their behaviour */
	void ActivateAttackComponents();

	/* [Server] Play animation unless anims are paused. Either way it will replicate to clients */
	void PlayAnimation(EBuildingAnimation AnimType, bool bAllowReplaySameType);

	/**
	 *	Pause the animation that is currently playing 
	 */
	void PauseAnimation();
	void UnpauseAnimation();

	void DoNothing();

	/* Return whether this building is selected by local player */
	bool IsSelectableSelected() const;

	/* Shows selection decal */
	void ShowSelectionDecal() const;

	void ShowHoverDecal();

	/* Hides selection decal */
	void HideSelectionDecal() const;

	/* Return whether this building was a selectable the player started the match with */
	bool IsStartingSelectable() const;

	/* Whether this building is visible (meaning outside of fog and not stealthed) to the local 
	player */
	bool IsFullyVisibleLocally() const;

	/* Returns true if executing code on server, false if on a remote client */
	bool OnServer() const;

	/* Returns true if executing code on a remote client, false if on server */
	bool OnClient() const;

	/* Return whether for the local player the animations of the building are frozen */
	bool AreAnimationsPausedLocally() const;

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	/** 
	 *	Call function that returns void after delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(ABuilding:: *Function)(), float Delay);

	/* Call function that returns void after delay. Creates a timer handle for this */
	void Delay(void(ABuilding:: *Function)(), float DelayAmount);

	/* For sorting buildings in arrays */
	friend bool operator<(const ABuilding & Building1, const ABuilding & Building2);

	/* Whether the Destroyed anim has finished. If there is no Destroyed anim then this 
	will be false. Could possibly do away with this variable and figure it out some other way */
	uint8 bHasDestroyedAnimFinished : 1;

	/* Whether CleanUp() has run or not */
	uint8 bHasRunCleanUp : 1;

	/* True if the building is sinking into the ground. Can happen when the building reaches
	zero health */
	uint8 bSinkingIntoGround_Logically : 1;

	/* True if the building is sinking into ground and the local player can see it happening */
	uint8 bSinkingIntoGround_Visually : 1;

	/* Whether the local player has seen the building at least once */
	uint8 bHasLocalPlayerSeenBuildingAtLeastOnce : 1;

	/* Whether the local player has seen the building in a state that implies it has reached 
	zero health */
	uint8 bHasLocalPlayerSeenBuildingAtZeroHealth : 1;

	/* How long we have spent sinking into the ground */
	float TimeSpentSinking;

	FTimerHandle TimerHandle_Destroy;

	/* Height of bounds. Used when raising building during construction */
	float BoundsHeight;

	/* The locations that fog manager will check when deciding if this building is visible
	or not. This array is populated on post edit and needs to have world location and rotation
	taken into account when spawned during a match */
	UPROPERTY()
	TArray < FVector2D > FogLocations;

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	bool bCanEditHumanOwnerIndex;

#endif

#if WITH_EDITOR 

	/* To change visibility of context button variables on changes in editor */
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

#endif

public:

	//~ Getters and seters
	
//#if WITH_EDITOR
	// Use GetAttributesBase().GetBuildingType() in non-editor builds
	EBuildingType GetType() const;
//#endif

	EFaction GetFaction() const;

	/* Relevant for local player only */
	bool IsInsideFogOfWar() const;

	float GetRemainingBuildTime() const;

	/* Get the world location the building should be at when it has been fully constructed */
	FVector GetFinalLocation() const;

	/* Return whether the building is fully constructed */
	bool IsConstructionComplete() const;

	/* Return whether something is constructing this building i.e. it is making progress on
	its construction */
	bool IsBeingBuilt() const;

	/* Whether building can accept a worker to work on it */
	bool CanAcceptBuilder() const;

	/* Whether building can accept a worker to work on it. Assumes that we know that the 
	build method is a method that needs workers working on it */
	bool CanAcceptBuilderChecked() const;

	/* Called when a builder starts working on this building */
	void OnWorkerGained(AActor * NewWorker);

	/* Called when a worker stops working on this */
	void OnWorkerLost(AActor * LostWorker);

	/* Whether this building is a drop point for a resource type */
	bool IsDropPointFor(EResourceType ResourceType) const;

	/* Get the world coords for where the units initial move once spawned should be to */
	FVector GetInitialMoveLocation() const;

	/* Get the world coords for the unit rally point */
	FVector GetRallyPointLocation() const;

	/* Get where a unit that has spawned from this building should initially do */
	EUnitInitialSpawnBehavior GetProducedUnitInitialBehavior() const;

	/* Returns whether the building is in a state where it is ok to set its rally point */
	bool CanChangeRallyPoint() const;

	/** 
	 *	Get the closest resource spot that a unit can collect from. Considers all the 
	 *	resource types the unit can collect. 
	 *	If either: 
	 *	- A resource spot cannot be found 
	 *	- unit cannot collect any resource types 
	 *	then this will return null 
	 *
	 *	Currently this is a relatively slow function. It iterates all resource spots on the map 
	 *	that the unit can gather from.
	 */
	AResourceSpot * GetClosestResourceSpot(const FResourceGatheringProperties & ResourceGatheringAttributes) const;

	/* Send message to server to try and change rally point location. Because unreliable it
	is possible location will not actually change even though audio cue will be given to
	player straight away. Actually because it's starting from client then going to server this
	can probably be changed to reliable since clients basically do nothing */
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_ChangeRallyPointLocation(const FVector_NetQuantize & NewLocation);

	/* This is a version of the function above to only be called on server. This function is
	only here as an optimization if calling RPC functions that do not actually go over the
	wire has overhead as opposed to calling the func directly */
	void ChangeRallyPointLocation(const FVector_NetQuantize & NewLocation);

	ARTSPlayerState * GetPS() const;
	ARTSGameState * GetGS() const;
	ARTSPlayerController * GetPC() const;

	TSubclassOf <AGhostBuilding> GetGhostBP() const;

	FORCEINLINE UBoxComponent * GetBoxComponent() const { return Bounds; }
	FORCEINLINE USkeletalMeshComponent * GetMesh() const { return Mesh; }

	const TArray < FVector2D > & GetFogLocations() const;

	bool CheckAnimationProperties() const;

#if !UE_BUILD_SHIPPING
	bool VerifyAllAnimNotifiesArePresentOnMontage(EBuildingAnimation AnimType, UAnimMontage * Montage, FString & OutErrorMessage) const;
#endif


	//=====================================================================================
	//	Queue Production
	//=====================================================================================

	const FProductionQueue * GetPersistentProductionQueue() const;
	FProductionQueue * GetPersistentProductionQueue();

	// This added for CPU player behavior
	const FProductionQueue & GetContextProductionQueue() const;

	/* Functions to tell server to try and start producing something. Several functions to cut
	down on bandwidth with params. TODO: param should be able to be removed as a bandwidth
	optimization and use seperate functions instead. Mainly there right now to avoid declaring
	like 10 more RPC UFUNCTIONS */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnProductionRequestBuilding(EBuildingType BuildingType, EProductionQueueType QueueType);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnProductionRequestUnit(EUnitType UnitType, EProductionQueueType QueueType);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnProductionRequestUpgrade(EUpgradeType UpgradeType, EProductionQueueType QueueType);
	/* @param bPerformChecks - whether to check if enough resources etc 
	@param InBuildingWeAreProducing - only relevant if we are fulfilling a request to produce a
	building that uses the BuildsItself build method */
	void Server_OnProductionRequest(const FContextButton & Button, EProductionQueueType QueueType, 
		bool bPerformChecks, ABuilding * InBuildingWeAreProducing);

protected:

	/* These functions are only called if adding something to the production queue successfully
	and it was the first item added therefore production on it should start immediately. Several
	functions to cut down on bandwidth with params */
	UFUNCTION(Client, Reliable)
	void Client_StartProductionBuilding(EBuildingType BuildingType, EProductionQueueType QueueType);
	UFUNCTION(Client, Reliable)
	void Client_StartProductionUnit(EUnitType UnitType, EProductionQueueType QueueType);
	UFUNCTION(Client, Reliable)
	void Client_StartProductionUpgrade(EUpgradeType UpgradeType, EProductionQueueType QueueType);
	void Client_StartProduction(const FTrainingInfo & InTrainingInfo, EProductionQueueType QueueType);

	/* Functions to tell client item was successfully added to queue but production should not
	start immediately */
	UFUNCTION(Client, Reliable)
	void Client_OnBuildingAddedToProductionQueue(EBuildingType BuildingType, EProductionQueueType QueueType);
	UFUNCTION(Client, Reliable)
	void Client_OnUnitAddedToProductionQueue(EUnitType UnitType, EProductionQueueType QueueType);
	UFUNCTION(Client, Reliable)
	void Client_OnUpgradeAddedToProductionQueue(EUpgradeType UpgradeType, EProductionQueueType QueueType);
	void Client_OnItemAddedToProductionQueue(const FTrainingInfo & InInfo, EProductionQueueType QueueType);

	/* Timer handle functions for different production queues */
	void Server_OnProductionComplete_Context();
	void Server_OnProductionComplete_Persistent();

	void Server_OnProductionComplete_Inner(FProductionQueue & Queue);

	/** 
	 *	[Server] Call from production queue when production for a unit completes 
	 *	
	 *	@param UnitType - unit that just completed production 
	 */
	void OnUnitProductionComplete(EUnitType UnitType);

public:

	/* Called by anim instance when the 'open door' anim reaches the point where the unit
	should be spawned */
	void AnimNotify_OnDoorFullyOpen();

protected:

	/* Create unit */
	void SpawnUnit(EUnitType UnitType);

	void Server_OnProductionCompletePart2_Persistent();
	void Server_OnProductionCompletePart2_Context();

	void Server_OnProductionCompletePart2_Inner(FProductionQueue & Queue);

	/* This updates the queue with real game state.
	@param - number of entries to remove from queue */
	UFUNCTION(Client, Reliable)
	void Client_OnProductionComplete(uint8 NumRemoved, EProductionQueueType QueueType);

	/* When production "completes" on the client nothing should happen - the timer handle is for
	visuals only, but if this is called before the server says its finished then timers are not
	running at the same speed on server and client. The behavior here is mark that the timer
	has completed so the HUD can show 100% complete in the meantime as opposed to 0%.
	Hopefully the difference between server and client is small and the freeze will not last long */
	void Client_OnProductionQueueTimerHandleFinished_Context();
	void Client_OnProductionQueueTimerHandleFinished_Persistent();

	//-----------------------------------------------------------------------------------------
	//	Production Queue CPU Player AI Controller Only Functions
	//-----------------------------------------------------------------------------------------
	//	I have decided to use completly seperate functions for CPU player AI controllers. 
	//	The main reason is that the current queue code is written with a human player in mind. 
	//	It would only need slight adjustments to get it to work for CPU players but I will just 
	//	keep this seperate for the time being

public:

	/* For a CPU player put something into the production queue. Buildings go into the persistent 
	queue while units and upgrades go in the context queue. These functions assume that EVERYTHING 
	has been checked - resources, prereqs, queue capacity, possibly more */
	void AICon_QueueProduction(const FCPUPlayerTrainingInfo & ItemWeWantToProduce);

	/** 
	 *	Version for queuing BuildsItself build method buildings 
	 *	
	 *	@param BuildingWeAreProducing - reference to the spawned building that we are producing 
	 *	using this building 
	 */
	void AICon_QueueProduction(const FCPUPlayerTrainingInfo & ItemWeWantToProduce, 
		ABuilding * BuildingWeAreProducing);

protected:

	void AICon_QueueProductionInner(FProductionQueue & Queue, const FCPUPlayerTrainingInfo & ItemWeWantToProduce);

	void AICon_OnQueueProductionComplete_Persistent();
	void AICon_OnQueueProductionComplete_Context();

	void AICon_OnQueueProductionComplete_Inner(FProductionQueue & Queue);

	void AICon_OnProductionCompletePart2_Persistent();
	void AICon_OnProductionCompletePart2_Context();

	void AICon_OnProductionCompletePart2_Inner(FProductionQueue & Queue);

	//==========================================================================================
	//	ISelectable Interface
	//==========================================================================================

public:

	virtual class UBoxComponent * GetBounds() const override;
	virtual void OnSingleSelect() override;
	virtual EUnitType OnMarqueeSelect(uint8 & SelectableID) override;
	virtual uint8 OnDeselect() override;
	virtual uint8 GetSelectableID() const override;
	virtual bool Security_CanBeClickedOn(ETeam ClickersTeam, const FName & ClickersTeamTag, ARTSGameState * GameState) const override;
	virtual void OnRightClick() override;
	virtual void OnMouseHover(ARTSPlayerController * LocalPlayCon) override;
	virtual void OnMouseUnhover(ARTSPlayerController * LocalPlayCon) override;
	virtual ETeam GetTeam() const override;
	virtual uint8 GetTeamIndex() const override;
	virtual const FContextMenuButtons * GetContextMenu() const override;
	virtual void GetInfo(TMap < EResourceType, int32 > & OutCosts, float & OutBuildTime, FContextMenuButtons & OutContextMenu) const override;
	virtual TArray <EBuildingType> GetPrerequisites() const override;
	virtual float GetCooldownRemaining(const FContextButton & Button) override;
	virtual void OnContextCommand(const FContextButton & Button) override;
	virtual void OnContextCommand(const FContextButtonInfo & AbilityInfo, const FVector & ClickLoc) override;
	virtual void OnAbilityUse(const FContextButtonInfo & AbilityInfo, uint8 ServerTickCountAtTimeOfAbility) override;
	virtual const FAttachInfo * GetBodyLocationInfo(ESelectableBodySocket BodyLocationType) const override;
	virtual bool UpdateFogStatus(EFogStatus FogStatus) override;
	virtual void GetVisionInfo(float & SightRadius, float & StealthRevealRadius) const override;
	virtual const FProductionQueue * GetProductionQueue() const override;
	virtual void SetOnSpawnValues(ESelectableCreationMethod InCreationMethod, ABuilding * BuilderBuilding) override;
	virtual void SetSelectableIDAndGameTickCount(uint8 InID, uint8 GameTickCounter) override;
	virtual const FSelectableAttributesBase & GetAttributesBase() const override;
	virtual FSelectableAttributesBase & GetAttributesBase() override;
	virtual const FSelectableAttributesBasic * GetAttributes() const override;
	virtual const FBuildingAttributes * GetBuildingAttributes() const override;
	virtual FBuildingAttributes * GetBuildingAttributesModifiable() override;
	virtual float GetHealth() const override;
	virtual float GetBoundsLength() const override;
	/* This is called during FI creation and will probably be to know which projectiles to 
	pool. Will want to consider defense components but returns null for now */
	virtual TSubclassOf<AProjectileBase> GetProjectileBP() const override;
	virtual bool HasAttack() const override;
	virtual bool CanClassGainExperience() const override;
	virtual uint8 GetRank() const override;
	virtual float GetCurrentRankExperience() const override;
	virtual const TMap <FContextButton, FTimerHandle> * GetContextCooldowns() const override;
	virtual const FShopInfo * GetShopAttributes() const override;
	virtual const FInventory * GetInventory() const override;
	virtual float GetTotalExperienceRequiredForNextLevel() const override;
	virtual ARTSPlayerController * GetLocalPC() const override;
	virtual void OnItemPurchasedFromHere(int32 ShopSlotIndex, ISelectable * ItemRecipient, bool bTryUpdateHUD) override;
	virtual uint8 GetOwnersID() const override;
	virtual FVector GetActorLocationSelectable() const override;
	virtual bool IsInStealthMode() const override;
	virtual bool HasEnoughSelectableResources(const FContextButtonInfo & AbilityInfo) const override;
	virtual void ShowTooltip(URTSHUD * HUDWidget) const override;
	virtual void OnEnemyDestroyed(const FSelectableAttributesBasic & EnemyAttributes) override;
	virtual void AdjustPersistentWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
	virtual void AdjustSelectedWorldWidgetForNewCameraZoomAmount(float CameraZoomAmount, float CameraPitchAmount) override;
	virtual ARTSPlayerState * Selectable_GetPS() const override;
	virtual const FBuildingTargetingAbilityPerSelectableInfo * GetSpecialRightClickActionTowardsBuildingInfo(ABuilding * Building, EFaction BuildingsFaction, EBuildingType BuildingsType, EAffiliation BuildingsAffiliation) const override;
	virtual bool CanAquireTarget(AActor * PotentialTargetAsActor, ISelectable * PotentialTarget) const override;
	virtual URTSGameInstance * Selectable_GetGI() const override;

	//=========================================================================================
	/* Buffs and debuffs code */

	virtual bool CanClassAcceptBuffsAndDebuffs() const override;
#if ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS
	virtual const TArray <FStaticBuffOrDebuffInstanceInfo> * GetStaticBuffArray() const override;
	virtual const TArray <FStaticBuffOrDebuffInstanceInfo> * GetStaticDebuffArray() const override;
#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
	virtual const TArray <FTickableBuffOrDebuffInstanceInfo> * GetTickableBuffArray() const override;
	virtual const TArray <FTickableBuffOrDebuffInstanceInfo> * GetTickableDebuffArray() const override;
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME
#endif // ALLOW_BUFFS_AND_DEBUFFS_ON_BUILDINGS

#if WITH_EDITOR
	virtual bool PIE_IsForCPUPlayer() const override;
	virtual int32 PIE_GetHumanOwnerIndex() const override;
	virtual int32 PIE_GetCPUOwnerIndex() const override;
#endif
	//~ End ISelectable interface 


	//=========================================================================================
	//	Other Stuff
	//=========================================================================================

	void SetupBuildingInfo(FBuildingInfo & OutInfo, const AFactionInfo * FactionInfo) const;

	/** 
	 *	True: CPU player owns this unit 
	 *	False: human player owns this unit 
	 */
	bool IsOwnedByCPUPlayer() const;

	/* Overridden from AActor */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;

protected:

	/* AnimNotify_OnZeroHealthAnimationFinished */
	void OnZeroHealthAnimFinished();

	void RunOnZeroHealthAnimFinishedLogic();

	void OnSinkingIntoGroundComplete();

	/** 
	 *	Return whether the building has been destroyed. By completely destroyed we mean that 
	 *	AActor::Destroy wants to be called on it, but perhaps the building is in fog of war, 
	 *	and we don't want to unhide it and reveal the fact it is gone by calling Destroy yet. 
	 */
	bool HasBeenCompletelyDestroyed() const;

	/* [Server] This is a final function to be called when the building has been destroyed */
	void ServerCleanUp();

	/* [Client] This is a final function to be called when the building has been destroyed */
	void ClientCleanUp(bool bForceDestroy);

	// Networking - called on client when actor is torn off
	virtual void TornOff() override;
	
	void DestroyBuilding();

	/** 
	 *	Run preparation logic for ability after a delay 
	 *	
	 *	@param CooldownAmount - the amount of time till the ability comes off cooldown, not the 
	 *	time till preparation anim should play 
	 */
	void CallAbilityPreparationLogicAfterDelay(const FContextButtonInfo & AbilityInfo, float CooldownAmount);

	/** 
	 *	Called when an ability that wants some preparation logic to run cools down enough that 
	 *	it can run. 
	 */
	UFUNCTION()
	void OnAbilityPreparationCooldownFinished(const FContextButtonInfo & AbilityInfo);
	
	void OnAbilityCooldownFinished();

	UFUNCTION()
	void OnRep_Health();

	enum class EOnZeroHealthCallSite : uint8
	{
		FromTakeDamage, 
		FromOnRepHealth,
		Other,
	};

	void OnZeroHealth(EOnZeroHealthCallSite CalledFrom);

	/* [Server] Tells starts sinking into ground locally and also tells clients to by using 
	anim state */
	void ServerInitiateSinkingIntoGround();
	
	void BeginSinkingIntoGround();

	/* Called if this building is being produced using BuildsItself build method by another 
	building but that other building reaches zero health */
	void OnProducerBuildingZeroHealth();

	float CalculateBoundsHeight() const;
	
public:

	//============================================================================================
	//	------- Building Attack Components -------
	//============================================================================================

	/**
	 *	Called when an attack component makes an attack 
	 *	
	 *	@param AttackCompID - unique ID of the component doing the attack 
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_OnBuildingAttackComponentAttackMade(uint8 AttackCompID, AActor * AttackTarget);

protected:

	/* Get a building attack comp given a unique ID */
	IBuildingAttackComp_Turret * GetBuildingAttackComp_Turret(uint8 CompUniqueID) const;
};


// Some things that could be done:
// Inject checks for IsAtSelectableCap at times where it is appropriate. At least a few places 
// I can think where this is appropriate:
// - I've done it already in GM PIE setup code
// - could add it to starting grid spawn logic. Would probably be doubling up on the 
// IsAtSelectableCap check since already do it in GM code but that's for PIE/standalone so 
// no real biggie.
// - when trying to build a unit/building server-side, so probably at the same time resource 
// checks are done. 
// - client-side when pressing buttons
// - CPU player actions
// These were just some off the top off my head. Haven't thought too much about it tthough
// Could implement selectable uniqueness, similar to tanya in RA2
// Probably have to do it in faction info instead of putting it on selectables
// Could implement housing. Should make it general though so users can define multiple 
// housing type resources if desired

/* 
	for queues and the selectable cap: could add variable like: PS::NumSelecatblesInQueues
	that is incremented when an item is added to any queue, and the decremented when 
	an item is removed
*/

// If implementing unique units then basically every time you cehck selectable cap cehck unit 
// uniqueness too I guess


// So after finally getting compile going 3 things to check: 
// - unique buildings/units works 
// - the HUD persistent panel 4 display types work 
// - production queues work
