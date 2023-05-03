// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "MapElements/BuildingComponents/BuildingAttackComp_Turret.h"
#include "Statics/Structs_2.h"
#include "Managers/HeavyTaskManagerTypes.h"
#include "BuildingAttackComponent_SM.generated.h"

class UFogObeyingAudioComponent;


/**
 *	A component that goes on a building. It can attack. This version is for a static mesh 
 *	and is for components that do not need to rotate their pitch. It can optionally be added as a 
 *	child of a IBuildingAttackComp_TurretsBase in which case the base will rotate the yaw and this too.
 *
 *	Couple examples of what structures could be made with this:
 *	- nod obelisk of light
 *	- nod turret if you attach this to a rotatable base (UBuildingAttackCompStructBase_SM/SK)
 *
 *	If you need animations then use UBuildingAttackComponent_SK
 *	If you want a structure that can also change it's pitch then use a UPitchChangeBuildingAttackComp_SM/SK
 */
UCLASS(Blueprintable, ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class RTS_VER2_API UBuildingAttackComponent_SM : public UStaticMeshComponent, public IBuildingAttackComp_Turret
{
	GENERATED_BODY()
	
public:

	UBuildingAttackComponent_SM();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

protected:

	// Attack Target
	void ServerDoAttack();

	//~ Begin IBuildingAttackComp_Turret interface
	virtual void ClientDoAttack(AActor * AttackTarget) override;
	//~ End IBuildingAttackComp_Turret interface

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const override;

	//~ Begin IBuildingAttackComp_Turret interface
	virtual UMeshComponent * GetAsMeshComponent() override;
	virtual void SetupAttackComp(ABuilding * InOwningBuilding, ARTSGameState * GameState, AObjectPoolingManager * InPoolingManager, uint8 InUniqueID, IBuildingAttackComp_TurretsBase * InTurretRotatingBase) override;
	virtual void ServerSetupAttackCompMore(ETeam InTeam, const FVisibilityInfo * InTeamsVisibilityInfo, FCollisionObjectQueryParams InQueryParams) override;
	virtual void ClientSetupAttackCompMore(ETeam InTeam) override;
	virtual FOverlapDelegate * GetTargetingTraceDelegate() override;
	virtual FCollisionObjectQueryParams GetTargetingQueryParams() const override;
	virtual FVector GetTargetingSweepOrigin() const override;
	virtual float GetTargetingSweepRadius() const override;
	virtual void SetTaskManagerBucketIndices(uint8 BucketIndex, int16 ArrayIndex) override;
	virtual void SetTaskManagerArrayIndex(int16 ArrayIndex) override;
	virtual void GrabTaskManagerBucketIndices(uint8 & OutBucketIndex, int16 & OutArrayIndex) const override;
	virtual void OnParentBuildingEnterFogOfWar(bool bOnServer) override;
	virtual void OnParentBuildingExitFogOfWar(bool bOnServer) override;
	//~ End IBuildingAttackComp_Turret interface

protected:
	
	/* Set whether this turret has a parent component that can rotate yaw */
	void SetRotatingBase(IBuildingAttackComp_TurretsBase * InTurretRotatingBase);

	/* True if this turret is attached to a component that rotates yaw for it */
	bool HasRotatingBase() const;

	/* If true then attack is ready to use. Warmup may still need to run if attack requres it though */
	bool IsAttackOffCooldown() const;

	/** 
	 *	Whether the structure has to warm up before attacking e.g. obelisk of light has like a
	 *	4 second warm up. 
	 */
	bool HasAttackWarmup() const;

	/**
	 *	Return whether attack has fully warmed up and therefore can be used if structure has
	 *	a viable target.
	 */
	bool HasAttackFullyWarmedUp() const;

	/* Return whether attack is currently warming up but not ready yet */
	bool IsAttackWarmingUp() const;

	/**
	 *	Called when a sweep has completed. The sweep gets all actors within range 
	 *	
	 *	@param TraceData - result of trace 
	 */
	void OnSweepForTargetsComplete(const FTraceHandle & TraceHandle, FOverlapDatum & TraceData);

	/** 
	 *	Return whether an actor can be aquired as a target. Assumes the actor has been checked 
	 *	to see if it is in range. 
	 */
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor) const override;
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const override;

	/* Returns whether Target is still a valid target. Might not necessarily be able to be 
	attacked right now e.g. structure is not facing them yet */
	bool IsCurrentTargetStillAquirable() const;

	enum class ETargetTargetingCheckResult : uint8
	{
		/* Attack can be made on target */
		CanAttack, 
		/* Attack cannot be made on target but they can stay aquired e.g. structure is not 
		facing the target but has a rotating base so it can face them eventually */
		CannotAttack, 
		/* Target cannot be attacked and should be unaquired e.g. inside fog of war */
		Unaquire, 
	};

	ETargetTargetingCheckResult GetCurrentTargetTargetingStatus() const;

	/* Return whether an actor is in range to be attacked */
	bool IsActorInRange_NoLenience(AActor * Actor) const;
	/* Return whether an actor is in range to be attacked. This version will use range lenince */
	bool IsActorInRange_UseLeneince(AActor * Actor) const;

	/** 
	 *	[Server] Return how much yaw the building would need to rotate to be considered facing the actor. 
	 *	Returns absolute values so range is [0, 180] 
	 */
	float GetYawToActor(AActor * Actor) const;

	/* [Server] This version also outputs the look at FRotator */
	float GetYawToActor(AActor * Actor, FRotator & OutLookAtRot) const;

	/** 
	 *	Get in world space the component's yaw rotation for targeting purposes, but it may not be 
	 *	the actual rotation that GetComponentRotation().Yaw would return.
	 */
	float GetEffectiveComponentYawRotation() const;

	//~ Begin IBuildingAttackComp_Turret interface
	virtual void SetTarget(AActor * InTarget) override;
	virtual AActor * GetTarget() const override;
	//~ End IBuildingAttackComp_Turret interface

	/* Start warming up attack */
	void StartAttackWarmUp();

	/* Stop warming up attack */
	void ServerStopAttackWarmUp();
	void ClientStopAttackWarmUp();

	UFUNCTION()
	void OnRep_TimeAttackWarmupStarted();

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	/* Particle system component that shows the attack warmup particles */
	UPROPERTY()
	UParticleSystemComponent * WarmupParticlesComp;

	/* Audio component that plays the attack warmup sound */
	UPROPERTY()
	UFogObeyingAudioComponent * WarmupAudioComp;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FAttackAttributes AttackAttributes;

	/** 
	 *	Multiplies attack range. The result is used to check if an actor that has already been 
	 *	aquired as a target is in range i.e. once a target has been aquired they can go out of 
	 *	range a bit and still be shot at 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "1.0"))
	float RangeLenienceMultiplier;

	UPROPERTY()
	float TimeRemainingTillAttackOffCooldown;

	const FVisibilityInfo * TeamVisibilityInfo;

	/* The actor this building has aquired as a target to attack */
	TWeakObjectPtr<AActor> Target;

	ABuilding * OwningBuilding;
	ARTSGameState * GS;
	AObjectPoolingManager * PoolingManager;

	/* Delegate that fires when an async sweep for targets within range finishes */
	FOverlapDelegate TargetingTraceDelegate;
	
	/* Where in UHeavyTaskManager::BuildingAttackComps this object is stored at */
	FTaskManagerBucketInfo TaskManagerBucket;

	/**
	 *	Time it takes to prepare attack. Use 0 to attack as soon as possible. Probably
	 *	also belongs on FAttackAttributes.
	 *
	 *	e.g. obelisk of light takes like 5 seconds
	 *
	 *	This + FAttackAttributes::AttackRate add up to give the total time between attacks
	 *	e.g. If FAttackAttributes::AttackRate is 2 and this is 3 then it's a total of 5 seconds
	 *	between attacks.
	 *
	 *	My notes: checks to see if warmup should stop happen every time a sweep result comes in
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float AttackWarmupTime;

	/**
	 *	World time when attack warmup started. -1.f means not warming up
	 */
	UPROPERTY(ReplicatedUsing = OnRep_TimeAttackWarmupStarted)
	float TimeAttackWarmupStarted;

	float TimeSpentWarmingUpAttack;

	/* The team this structure is on */
	ETeam Team;

	/** How the structure selects it's target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ETargetAquireMethodPriorties TargetAquireMethod;

	/** 
	 *	Whether the structure likes selecting targets closer or further away. Will only be 
	 *	relevant if TargetAquireMethod considers distance. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EDistanceCheckMethod TargetAquirePreferredDistance;

	/** 
	 *	If true then the building will not change targets until it's current target becomes 
	 *	untargetable. If false then everytime a sweep result comes in it will reevaluate it's target. 
	 *	Better performance if this is true.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bHasTunnelVision;

	/** 
	 *	Attack preperation particles. Should not be looping. Probably belong 
	 *	on FAttackAttributes 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * AttackPreparationParticles;

	/* Query params this structure will use when sweeping for targets */
	FCollisionObjectQueryParams QueryParams;

	/** 
	 *	How much yaw can be off by for structure to fire at target.
	 *	180 = can attack at targets at any angle.
	 *	If this is <180 then the the building will need a rotating base to attack from every angle. 
	 *	If it does not have a rotating base then it will have a blind spot behind it so the 
	 *	rotation of the building when it is placed matters quite a bit.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float YawFacingRequirement;

	uint8 AttackCompUniqueID;

	/** 
	 *	The yaw rotating turret base this is a child of. Can be null if not attached to one
	 */
	IBuildingAttackComp_TurretsBase * RotatingBase;

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
