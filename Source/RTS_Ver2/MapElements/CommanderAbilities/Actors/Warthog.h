// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Statics/Structs_5.h"
#include "Warthog.generated.h"

class UWarthogMovementComponent;
class AProjectileBase;
class UCurveFloat;
class AObjectPoolingManager;
class ARTSPlayerState;
enum class ETargetingType : uint8;
enum class EArmourType : uint8;
class ARTSGameState;
enum class ETeam : uint8;
class URTSGameInstance;
class UFogObeyingAudioComponent;
class UAnimMontage;


enum class EWarthogAttackPhase : uint8
{
	Phase1_MovingToTarget, 
	/* Lowering pitch so facing ground */
	Phase2_Descending,
	/* Is firing shots */
	Phase3_Firing,
	/* Has fired every shot */
	Phase4_PostFired,
};


USTRUCT()
struct FWarthogAttackAttributes
{
	GENERATED_BODY()

public:

	FWarthogAttackAttributes();

	/* 1st type of projectile to fire */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf<AProjectileBase> ProjectileType1_BP;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Override Projectile Type 1 Damage Values"))
	bool bOverrideProjectileType1DamageValues;

	/* Damage values for projectile type 1 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideProjectileType1DamageValues))
	FBasicDamageInfo ProjectileType1_DamageAttributes;

	/* 2nd type of projectile to fire. Fired as often or less often than projectile 1 */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf<AProjectileBase> ProjectileType2_BP;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Override Projectile Type 2 Damage Values"))
	bool bOverrideProjectileType2DamageValues;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideProjectileType2DamageValues))
	FBasicDamageInfo ProjectileType2_DamageAttributes;

	/**
	 *	Time between firing projectiles. Never use a Y value of <= 0 at any point on this curve. 
	 *	
	 *	X axis = how far into phase 3 the warthog is 
	 *	Y axis = time between shot 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * Curve_TimeBetweenShots;

	/**
	 *	For every Projectile2Ratio of projectile type 1 that is fired, a projectile type 2 will be fired. 
	 *	So the higher this value is the more of projectile type 1 that is fired and the less of 
	 *	projectile type 2 that is fired. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int32 Projectile2Ratio;

	/* Total number of projectiles to fire */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int32 NumProjectiles;

	/* Particles for every time a shot is fired */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UParticleSystem * MuzzleParticles;

	/* Socket on mesh that projectiles spawn from */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName MuzzleName;

	//---------------------------------------------------------------
	//	Accuracy properties

	/** 
	 *	Curve for the pitch. 
	 *	X axis = range [0, 1] 
	 *	Y axis = amount of pitch to apply to shot. Higher values = less accurate shots. Only 
	 *	needs to be positive - a random sign will be given to each value
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * Curve_PitchSpread;

	/** 
	 *	Curve for the yaw 
	 *	X axis = range [0, 1] 
	 *	Y axis = amount of yaw to apply to shot. Higher values = less accurate shots. Only 
	 *	needs to be positive - a random sign will be given to each value 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * Curve_YawSpread;

	/* The sound to play while the warthog is firing */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * FiringSound;
};


/** 
 *	Plane that will fly to a location and fire at it
 *	
 *	- the default values for this class are set up decently for the marketplace warthog model 
 *	with the muzzle socket added to Skeleton_Gun_End, rotating its yaw by -90 and giving it -5 pitch.
 *	- set the sound for the engine as EngineAudioComp::Sound
 *	- check the movement component - it may have some variables for you to adjust 
 *
 *	TODO only the engine anim plays. Implement the firing anim and possibly a pull up anim too.
 */
UCLASS(Abstract)
class RTS_VER2_API AWarthog : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AWarthog();

protected:
	
	virtual void BeginPlay() override;

	void CheckCurveAssets();

public:	
	
	/* Call this right after BeginPlay */
	void InitialSetup(URTSGameInstance * InGameInstance, AObjectPoolingManager * InObjectPoolingManager);

	/* @param TargetLocation - world location where the ability was used */
	void OnOwningAbilityUsed(ARTSGameState * GameState, ARTSPlayerState * InInstigator, const FVector & TargetLocation, int32 InRandomNumberSeed);

	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser) override;

	FWarthogAttackAttributes & GetAttackAttributes() { return AttackAttributes; }

	/** 
	 *	[Server] Return where in the world the plane should spawn 
	 *	
	 *	@param InInstigator - the player using the ability that spawns this warthog
	 *	@param TargetLocation - world location where the ability was used
	 *	@param OutYawBearing - the direction from the TargetLocation in which the warthog should spawn. 
	 *	This gets passed on to clients
	 *	@return - world location where the warthog should spawn
	 */
	FVector GetStartingLocation(ARTSPlayerState * InInstigator, const FVector & TargetLocation, float & OutYawBearing);
	
	/* [Client] Return starting location */
	FVector GetStartingLocation(const FVector & TargetLocation, float YawBearing) const;

	static FRotator GetStartingRotation(const FVector & SpawnLocation, float YawBearing);

protected:

	/* Fire a projectile or sometimes many because rate of fire could be quite fast */
	void FireProjectiles();

	/**
	 *	Return direction where a projectile should be fired at 
	 *	
	 *	@param MuzzleTransform - world transform for the muzzle socket 
	 */
	FRotator GetProjectileFireDirection(const FTransform & MuzzleTransform) const;

public:

	float GetPhase1AndPhase4MoveSpeed() const;
	float GetPhase2AndPhase3MoveSpeed() const;
	EWarthogAttackPhase GetAttackPhase() const;
	float GetPhase2Tilt() const;
	float GetTimeSpentInPhase2AndPhase3() const;
	float GetPhase4Tilt() const;
	float GetTimeSpentInPhase4() const;

protected:

	//--------------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------------

	/* Adding this cause the warthog skeletal mesh is rotated by 90 degrees and I can't find 
	any other way of getting it the right rotation. I think rotation can actually be changed 
	when you import the SK into UE but whatever */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	USceneComponent * DummyRoot;

	UPROPERTY(VisibleAnywhere, Category = "RTS")
	USkeletalMeshComponent * Mesh_SK;

	UPROPERTY(VisibleAnywhere, Category = "RTS")
	UWarthogMovementComponent * MoveComp;

	/* Audio component to play the sound of the engine */
	UPROPERTY(VisibleAnywhere, Category = "RTS")
	UFogObeyingAudioComponent * EngineAudioComp;

	/* World location where the ability was used */
	FVector AbilityUseLocation;

	EWarthogAttackPhase Phase;

	ETeam InstigatorsTeam;

	//----------------------------------------------------------------------
	//	Animations

	/** Should be looping */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UAnimMontage * AnimMontage_EnginesRunning;

	//----------------------------------------------------------------------
	//	Behavior properties

	/* Height it which warthog spawns. This is above the use location's Z value */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float SpawnHeight;

	/** How fast the warthog moves during phase1 and phase4 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MoveSpeed_Phase1AndPhase4;

	/** 
	 *	How close to the target location in 2D coordinates that the warthog should transition 
	 *	into phase 2 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float TransitionDistance_Phase2;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MoveSpeed_Phase2AndPhase3;

	/** 
	 *	How much pitch to apply when about to fire and firing. Usually this is negative so the 
	 *	plane looks downward towards ground 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Tilt_Phase2;

	/** 
	 *	How far the warthog should travel in 2D coordinates during phase2 before it should 
	 *	switch to phase3. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float TransitionDistance_Phase3;

	/** How long after firing the last shot before the warthog should transition to phase4 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float TransitionTime_Phase4;

	/** How much pitch for the plane to go towards for phase4 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Tilt_Phase4;

	//------------------------------------------------------------------------------
	//	Attack properties

	URTSGameInstance * GI;
	AObjectPoolingManager * PoolingManager;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FWarthogAttackAttributes AttackAttributes;

	FRandomStream RandomStream;

	float TimeSpentInPhase2AndPhase3;

	float TimeBetweenShotsCurveYValue;

	float TimeTillFireNextProjectile;

	/* Number of projectiles this plane still has to fire */
	int32 NumProjectilesRemaining;

	float TimeSpentInPhase4;

	//-----------------------------------------------------------------------
	//	Other properties

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxHealth;

	UPROPERTY()
	float Health;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ETargetingType TargetingType;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EArmourType ArmourType;

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

	void RunPostEditLogic();
#endif
};
