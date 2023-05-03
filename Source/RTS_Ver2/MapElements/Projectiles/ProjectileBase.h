// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Public/CollisionQueryParams.h"

#include "Statics/CommonEnums.h"
#include "ProjectileBase.generated.h"

class AObjectPoolingManager;
class ARTSPlayerController;
class AMyGameInstance;
class ARTSGameState;
class UCameraShake;
class UCurveFloat;
class UParticleSystemComponent;
struct FBasicDamageInfo;
class URTSDamageType;
class AAbilityBase;


/* A particle system and a sound */
USTRUCT()
struct FParticleAudioPair
{
	GENERATED_BODY()

public:

	FParticleAudioPair()
		: ImpactParticles(nullptr)
		, ImpactSound(nullptr)
	{}

	UParticleSystem * GetImpactParticles() const { return ImpactParticles; }
	USoundBase * GetImpactSound() const { return ImpactSound; }

protected:

	//-----------------------------------------------------------
	//	Data
	//-----------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particles Systems")
	UParticleSystem * ImpactParticles;

	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * ImpactSound;
};


/*
 *	Base class for all projectiles.
 *
 *	FIXME
 *	Some things to do:
 *	- For performance: AddToPool is sometimes called more than once before adding projectile to 
 *	pool - first call is with some of the params false, then called again later with some params true. 
 *	Therefore it repeats the mesh hiding, tick disabling etc logic which is ineffecient. 
 *	I have created a new function SetupForEnteringObjectPool() to try and help with this but 
 *	it isn't called anywhere yet, so just baby steps towards fixing it.
 */
UCLASS(Abstract, NotBlueprintable)
class RTS_VER2_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:

	/* Number of inline array elements for fog locations arrays. Saves on performance. Increase
	this to adjust performance */
	static constexpr int32 NUM_FOG_LOCATIONS = 2;

	AProjectileBase();

protected:

	virtual void BeginPlay() override;

	/* Trail particles */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UParticleSystemComponent * TrailParticles;

	/** 
	 *	Overridden to prevent projectiles that accidently get shot below
	 *	world KillZ to be destroyed. Instead they get added back to the
	 *	object pool and a message is logged 
	 */
	virtual void FellOutOfWorld(const UDamageType & dmgType) override;

	/* Hide mesh, disable ticks, etc. Note: I do not call this anyway yet. It has been added 
	to transition from AddToPool being called 2+ times */
	virtual void SetupForEnteringObjectPool();

	/** Call right before adding to object pool
	@param bSetupForPool - whether to put the projectile into a state that makes it ready 
	for the object pool. Set to false if you have already done this.
	@param bActuallyAddToPool - for begin play these can't actually go in the pool but
	should call their AddToPool to put them in a state ready for the pool.
	@param bDisableTrailParticles - disable trail particles */
	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true);

	/** 
	 *	Disable trail particles and optionally add to pool
	 *	
	 *	@param AddToPool - just add to object pool 
	 */
	void DisableTrailParticles();

	/** Call DisableTrailParticles then add to object pool */
	void DisableTrailsAndTryAddToPool();

	/* Gets all selectables this can damage within AoERadius (measured from param Location
	to bounds of other selectables in 2D - no Z axis) and stores them in in hit results in
	the array HitResults */
	virtual void GetTargetsWithinRadius(const FVector & Location);

	/** 
	 *	Simulate the projectile hitting someone/somewhere. Note the hit result passed into
	 *	this may not always be an actual hit created by the physics system but should at least
	 *	have the target and impact point set by the caller 
	 */
	virtual void OnHit(const FHitResult & Hit);

	/* Call DealDamage after DealDamageDelay amount of time, then try add projectile back to pool */
	void CallDealDamageAfterDelay(const FHitResult & Hit);

	void DealDamageAndTryAddToPool();

	/** 
	 *	Deal damage; either AoE damage if AoE radius is > 0 or the actor storred in the
	 *	hit result
	 *	
	 *	@param Hit - hit result. Function will use ImpactPoint and Actor from hit result to
	 *	determine damage 
	 */
	virtual void DealDamage(const FHitResult & Hit);

	/** 
	 *	Return how much damage should be multiplied by because of distance from impact
	 *	(if projectile has an area of effect) 
	 */
	virtual float GetDamageDistanceMultiplier(float Distance) const;

	/* Get the particle system to show when the projectile hits something */
	UParticleSystem * GetImpactParticles(const FHitResult & Hit) const;

	/** 
	 *	Get the impact sound to play when the projectile hits something. 
	 *	
	 *	@param Hit - hit result for where the projectile is considered to have stopped at
	 */
	USoundBase * GetImpactSound(const FHitResult & Hit) const;

	/* Hit results from capsule sweep against viable collision channels */
	UPROPERTY()
	TArray <FHitResult> HitResults;

	/** Base damage to deal to what the projectile impacts. Overridable by firer */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (DisplayName = "Impact Damage"))
	float ImpactDamage;

	/* Type of damage this projectile deals ot what it impacts. Overridable by firer */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage", meta = (DisplayName = "Damage Type"))
	TSubclassOf<URTSDamageType> ImpactDamageType;

	/** 
	 *	Amount of randomness to impact damage. Overridable by firer.
	 *	OutgoingDamage = Damage * RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999"))
	float ImpactRandomDamageFactor;

	/* Base amount of AoE damage this projectile deals */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (DisplayName = "AoE Damage"))
	float AoEDamage;

	/* Damage type for AoE damage */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage", meta = (DisplayName = "AoE Damage Type"))
	TSubclassOf<URTSDamageType> AoEDamageType;

	/* Amount of randomness to apply to AoE damage */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999", DisplayName = "AoE Random Damage Factor"))
	float AoERandomDamageFactor;

#if WITH_EDITORONLY_DATA
	/** 
	 *	Particles and audio to play when projectile hits a surface. The entry for the key 'Default' 
	 *	will be used if the projectile hits a surface without a physical material 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Impact Effects", meta = (DisplayName = "Impact Effects"))
	TMap<TEnumAsByte<EPhysicalSurface>, FParticleAudioPair> ImpactEffectsTMap;
#endif

	/* The contents of ImpactEffectsTMap that gets populated in post edit. Here for performance */
	UPROPERTY()
	TArray<FParticleAudioPair> ImpactEffects;

	/** 
	 *	Delay between when a hit happens and when damage is dealt. Usually you want this as 0 
	 *	but things like nukes may want small delays. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = 0))
	float DealDamageDelay;

	/** 
	 *	TRAIL particles: duration to allow trail particles to appear after hit. If 0
	 *	then trail particles will disappear the moment hit is registered 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems", meta = (ClampMin = "0.0"))
	float TrailParticlesExtraTime;

	/** Camera shake to play at projectile impact point. Optional */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Camera Shake", meta = (DisplayName = "Impact Camera Shake"))
	TSubclassOf <UCameraShake> ImpactCameraShake_BP;

	/** Radius of ImpactCameraShake */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Camera Shake", meta = (EditCondition = bCanEditCameraShakeOptions))
	float ImpactShakeRadius;

	/* Falloff of ImpactCameraShake */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Camera Shake", meta = (EditCondition = bCanEditCameraShakeOptions))
	float ImpactShakeFalloff;

	/* An ability that wants to know when the projectile hits something */
	AAbilityBase * ListeningForOnHit;

	/* The unique ID for ListeningForOnHit */
	int32 ListeningForOnHitData;

	/* Reference to selectable that fired this projectile */
	UPROPERTY()
	AActor * Shooter;

	/* Reference to target this projectile is aimed at */
	UPROPERTY()
	AActor * Target;

	/* Reference to player controller to play camera shakes */
	UPROPERTY()
	ARTSPlayerController * PC;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	/* Reference to object pooling manager */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	/* BP to to use as TMap key to know what pool to add self back to
	when complete */
	UPROPERTY()
	TSubclassOf <AProjectileBase> Projectile_BP;

	/* World timer manager */
	FTimerManager * TimerManager;

	/* Timer handle for checking how long before we decide projectile will never register
	a hit so just psuedo destroy it and add it back to pool */
	FTimerHandle TimerHandle_Lifetime;

	/* Timer handle for calling DealDamage */
	FTimerHandle TimerHandle_DealDamage;

	/* Timer handle for duration of trail particles after a hit. Here so trail particles
	don't disappear the moment proejectile hits */
	FTimerHandle TimerHandle_TrailParticles;

	/** 
	 *	All actors that have a component overlapping inside this radius will be damaged.
	 *	0 = only damage impact target 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Area of Effect", meta = (ClampMin = "0.0", DisplayName = "Area of Effect Radius"))
	float AoERadius;

	/** 
	 *	Curve to use for the damage falloff of the AoE. If no curve is set then no falloff 
	 *	will happen.
	 *	
	 *	X axis = distance from impact point to target
	 *	Y axis = percentage of damage that will be dealt. Therefore a decreasing curve makes sense
	 *	(implies as range increases damage decreases)
	 *	
	 *	Some notes: This is automatically normalized to whatever range you use. i.e. the values
	 *	of the axis do not matter, what matters is the overall shape of the curve. For clarity
	 *	though you may want to have the Y axis in the range [0, 1] and the X axis in the range
	 *	[0, AoERadius]. A single straight line is currently not supported. If you want to do full
	 *	damage no matter the range you would need to do a horizontal line and after that have a line
	 *	that drops very sharply 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Area of Effect", meta = (EditCondition = bCanEditAoEOptions))
	UCurveFloat * DamageFalloffCurve;

	/* Team projectile belongs to. Can change because object pooling */
	ETeam OwningTeam;

	/* Can the AoE hit/damage enemies? */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Area of Effect", meta = (EditCondition = bCanEditAoEOptions, DisplayName = "Can AoE Hit Enemies"))
	bool bCanAoEHitEnemies;

	/* Can the AoE hit/damage selectables on our team? */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Area of Effect", meta = (EditCondition = bCanEditAoEOptions, DisplayName = "Can AoE Hit Friendlies"))
	bool bCanAoEHitFriendlies;

	/** 
	 *	Whether FCollisionQueryParams::bReturnPhysicalMateral should be set to true for hit results. 
	 *	Could just set it to true always but I feel that perhaps it's a little performance intensive 
	 *	to get the physical material for a hit result so hence this variable exists 
	 */
	UPROPERTY()
	bool bQueryPhysicalMaterialForHits;

	/* Holds what object types to check for when checking what is hit by AoE */
	FCollisionObjectQueryParams AoEObjectQueryParams;

	void SetupAoECollisionChannels(ETeam Team);

	/* DamageFalloffCurve values */
	float DamageCurveMinX, DamageCurveMaxX, DamageCurveMinY, DamageCurveMaxY;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditCameraShakeOptions;

	UPROPERTY()
	bool bCanEditAoEOptions;
#endif

private:

	void SetupCurveValues();

	void Delay(FTimerHandle & TimerHandle, void(AProjectileBase::* Function)(), float Delay);

public:

	float GetImpactDamage() const;
	TSubclassOf <URTSDamageType> GetImpactDamageType() const;
	float GetImpactRandomDamageFactor() const;
	float GetAoEDamage() const;
	TSubclassOf <URTSDamageType> GetAoEDamageType() const;
	float GetAoERandomDamageFactor() const;

	void SetProjectileBP(TSubclassOf <AProjectileBase> NewValue);
	void SetPoolingManager(AObjectPoolingManager * InPoolingManager);

	/** 
	 *	Function for when a unit wants to fire. Must be overridden.
	 *	
	 *	@param Firer - the selectable that fired this projectile. To know who dealt damage
	 *	@param AttackAttributes - attributes of the selectable firing this projectile to know how to
	 *	deal damage
	 *	@param AttackRange - range of attacker. Not the distance from X to Y or anything, just 
	 *	the attack range of the firer. 0 or less means infinite range
	 *	@param Team - team selectable is on to know who to damage
	 *	@param MuzzleLoc - location of muzzle of selectable firing projectile
	 *	@param ProjectileTarget - who this projectile is intended for 
	 */
	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation);

	/** 
	 *	Fires projectile at a location
	 *	
	 *	@param Firer - actor counted as firing projectile for damage response purposes
	 *	@param AttackAttributes - attack attributes of firer
	 *	@param Team - team of firer 
	 */
	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID);

	/* Have recently added this. Classes should override it and implement it. I have not 
	implemented it in any child classes yet.
	Fires the projectile in a direction. 
	Note that I have a function in AObjectPoolingManager that is something like 
	Server/Client_FireProjectileInDirection. I don't actually call this func from it. 
	Instead I call FireAtLocation. So perhaps want to eventually change that */
	virtual void FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
		float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID);

	/** 
	 *	Get the location of the projectile for fog of war. Depending on how big the projectile
	 *	is or its type there may be more than one location 
	 */
	virtual void GetFogLocations(TArray <FVector, TInlineAllocator<AProjectileBase::NUM_FOG_LOCATIONS>> & OutLocations) const;

	/* Most projectiles hook into UProjectileMovementComponent::OnProjectileStop to handle 
	'hit' logic. Alternatively the move comp can call this function instead. Some projectiles 
	may not implement this */
	virtual void OnProjectileStopFromMovementComp(const FHitResult & Hit);

	// TODO add GetLocation func for getting location for fog, and add projectiles to 
	// array in GS for fog manager
	//virtual void GetLocation

#if !UE_BUILD_SHIPPING
	/** 
	 *	Returns true if the projectile is in the state it should be in for sitting in the object 
	 *	pool e.g. mesh is hidden, tick is turned off, whatever 
	 */
	virtual bool IsFitForEnteringObjectPool() const;

	/* Return true if all timer handles have been cleared with timer manager */
	virtual bool AreAllTimerHandlesCleared() const;
#endif
};
