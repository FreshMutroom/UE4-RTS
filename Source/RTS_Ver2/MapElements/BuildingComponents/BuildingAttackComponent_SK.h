// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "MapElements/BuildingComponents/BuildingAttackComp_Turret.h"
#include "Statics/Structs_2.h"
#include "Managers/HeavyTaskManagerTypes.h"
#include "BuildingAttackComponent_SK.generated.h"

enum class ETeam : uint8;
class ARTSGameState;
class ABuilding;
class UFogObeyingAudioComponent;


/**
 *	The same as UBuildingAttackComponent_SM except this is for a skeletal mesh instead of a 
 *	static mesh. Also optionally you can use idle and attack animations. 
 *
 *	This class is very similar to the SM version except it can have animations.
 */
UCLASS(Blueprintable, ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class RTS_VER2_API UBuildingAttackComponent_SK : public USkeletalMeshComponent, public IBuildingAttackComp_Turret
{
	GENERATED_BODY()
	
public:

	UBuildingAttackComponent_SK();

	// Some ideas:
	// a bool bCancelRotationWhilePlayingAttackAnim

	virtual void BeginPlay() override;

protected:

	/* @return - true if anim blueprint has no errors */
	bool CheckAnimBlueprint();

	/**
	 *	Check whether the animations have thr required anim notifies on them 
	 *	
	 *	@return - true if all required anim notifies are present on all the montages for this component 
	 */
	bool CheckAnimNotifies();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

	// Attack Target
	void ServerDoAttack();

	//~ Begin IBuildingAttackComp_Turret interface
	virtual void ClientDoAttack(AActor * AttackTarget) override;
	//~ End IBuildingAttackComp_Turret interface

public:

	/* Called when the anim notify that does attack is reached */
	void ServerAnimNotify_DoAttack();

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
	virtual void OnParentBuildingExitFogOfWar(bool bOnServer) override;
	//~ End IBuildingAttackComp_Turret interface

protected:

	/* Set whether this turret has a parent component that can rotate yaw */
	void SetRotatingBase(IBuildingAttackComp_TurretsBase * InTurretRotatingBase);

	/* True if this turret is attached to a component that rotates yaw for it */
	bool HasRotatingBase() const;

	/* True if the structure uses an idle animation */
	bool HasIdleAnimation() const;
	/* True if the structure uses an animation for it's attack or attack warmup - same thing */
	bool HasAttackAnimationOrAttackWarmupAnimation() const;
	/* True if structure uses an animation for it's attack warmup */
	bool HasAttackWarmupAnimation() const;
	/* True if structure uses an animation for it's attack */
	bool HasAttackAnimation() const;

	/* Return true if playing animation for attack */
	bool IsPlayingAttackAnimation() const;
	/* Return true if playing animation for attack or attack warmup */
	bool IsPlayingAttackOrWarmupAnimation() const;

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

	void PlayAttackWarmupParticlesAndSound(float StartTime);

	/* Stop warming up attack */
	void ServerStopAttackWarmUp();
	void ClientStopAttackWarmUp();

	void StopWarmupParticles();

	void ServerStartAttackWarmupAnimation();
	void ServerStartAttackAnimation();
	void ServerStopAttackOrWarmupAnimation();
	void ServerStopAttackAnimation();
	void ClientPlayAttackWarmupAnimation(float AnimStartTime);
	void ClientPlayAttackAnimation(float AnimStartTime);
	void ClientStopAttackWarmupAnimation();

	/* Enable ticking so animations can play */
	void SetComponentAnimationTickingEnabled(bool bEnabled);

	/* Will likely want to disable ticking in this function */
	UFUNCTION()
	void ClientOnMontageEnded(UAnimMontage * Montage, bool bInterrupted);

	UFUNCTION()
	void OnRep_TimeAttackWarmupOrAnimStarted();

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

	/* Optional idle animation. Should be looping */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UAnimMontage * IdleAnim;

	/** 
	 *	Optional animation for attack. Should have a section at the end of it that loops idle anim
	 *	but only if you're actually want an idle anim.
	 *	You will probably want to look at the anim notifies on the anim instance class for this 
	 *	structure and may want to put some on this anim 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UAnimMontage * AttackAnim;

	/** 
	 *	If true then it is assumed AttackAnim includes your attack warmup period if there 
	 *	is one. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bUseAttackAnimForWarmupToo;

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
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = "!bUseAttackAnimForWarmupToo"))
	float AttackWarmupTime;

	/**
	 *	World time when attack warmup started or when AttackAnim started playing
	 *	-1.f means not warming up or anim not playing
	 */
	UPROPERTY(ReplicatedUsing = OnRep_TimeAttackWarmupOrAnimStarted)
	float TimeAttackWarmupOrAnimStarted;

	/* This might only be relevant if the attack warmup does not use an animation */
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
