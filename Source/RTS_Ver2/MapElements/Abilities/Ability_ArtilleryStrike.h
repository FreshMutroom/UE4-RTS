// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"

#include "Statics/Structs_2.h"
#include "Ability_ArtilleryStrike.generated.h"

class ACollidingProjectile;
class UCurveFloat;
class AObjectPoolingManager;
class ANoCollisionTimedProjectile;


/* Info for a single artillery strike instance */
USTRUCT()
struct FArtilleryStrikeInfo
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	FVector Location;

	/* Number of projectiles remaining to fire */
	UPROPERTY()
	uint32 NumProjectilesRemaining;

	/* The actor that instigated this artillery strike */
	TWeakObjectPtr <AActor> Instigator;

	/* The team of the ability's user. Here so we know what collision channel to set on the 
	projectiles */
	UPROPERTY()
	ETeam InstigatorsTeam;

	UPROPERTY()
	FRandomStream RandomStream;

	/* The yaw rotation of the last projectile spawned */
	UPROPERTY()
	float LastYawRot;

	/* Timer handle to control actions */
	UPROPERTY()
	FTimerHandle TimerHandle_Action;

public:

	// Never call this. Always call param version
	FArtilleryStrikeInfo();

	FArtilleryStrikeInfo(const FVector & InLocation, uint32 InNumProjectiles, AActor * InInstigator, 
		ETeam InInstigatorsTeam, const FRandomStream & InRandomStream);

	FVector GetLocation() const { return Location; }
	AActor * GetInstigator() const { return Instigator.Get(); }
	ETeam GetInstigatorsTeam() const { return InstigatorsTeam; }
	float GetLastYawRot() const { return LastYawRot; }
	void SetLastYawRot(float InValue) { LastYawRot = InValue; }
	FTimerHandle & GetActionTimerHandle() { return TimerHandle_Action; }
	uint32 GetNumProjectilesRemaining() const { return NumProjectilesRemaining; }
	void DecrementNumProjectiles() { NumProjectilesRemaining--; }
	FRandomStream & GetRandomStream() { return RandomStream; }
};


// TODO make sure object pools create projectiles for these
/**
 *	Optionally fires a projectile at a location. This can be a beacon or something.
 *	Then spawns several projectiles in a random fashion.	
 *
 *	It is very light on bandwidth and is intended to not be something that goes on for ages 
 *	(more than 30 seconds) as clients will no doubt drift from the state it is on the server.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_ArtilleryStrike : public AAbilityBase
{
	GENERATED_BODY()

public:

	AAbility_ArtilleryStrike();

private:

	/* Map from unique ID to state info about each active strike */
	UPROPERTY()
	TMap <int32, FArtilleryStrikeInfo> ActiveStrikes;

protected:

	virtual void BeginPlay() override;

	/* Reference to object pooling manager */
	UPROPERTY()
	AObjectPoolingManager * PoolingManager;

	/** 
	 *	Optional projectile to fire at the target location. This was added so something 
	 *	like a smoke beacon could be thrown at the location signalling a strike is coming. 
	 *	Then when this beacon projectile hits the first artillery projectile will be spawned 
	 *	after InitialDelay.
	 *	If this is kept null then the strike will just happen after InitialDelay.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Beacon Projectile"))
	TSubclassOf<ANoCollisionTimedProjectile> BeaconProjectile_BP;

	/* Damage attributes to pass in for the beacon. They have been set to do 0 damage */
	FBasicDamageInfo BeaconProjectileDamageInfo;

	/** Area projectiles can spawn in */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.001"))
	float Radius;

	/* Projectiles to spawn. Things such as initial speed and friendly fire should be defined in
	this BP */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf <ACollidingProjectile> Projectile_BP;

	/** Whether to override the projectiles damage attributes */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOverrideProjectileDamageValues;

	/** Damage of each projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ShowOnlyInnerProperties, EditCondition = bOverrideProjectileDamageValues))
	FBasicDamageInfo DamageInfo;

	/** How many projectiles to spawn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	uint32 NumProjectiles;

	/** 
	 *	Delay from when action is executed to when first projectile spawns or if beacon 
	 *	projecitle is set the time from when it arrives at the target location 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0"))
	float InitialDelay;

	/** 
	 *	Minimum amount of time between projectile spawn.
	 *	
	 *	To eliminate randomness set this equal to MaxInterval. If both are equal then all projectiles
	 *	spawn at the same time 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0"))
	float MinInterval;

	/** 
	 *	Maximum amount of time between projectile spawn.
	 *	
	 *	To eliminate randomness set this equal to MinInterval. If both equal to 0 then all projectiles
	 *	spawn at the same time 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0"))
	float MaxInterval;

	/** The height above spawn location to spawn projectiles from */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float SpawnHeight;

	/* Optional. A curve to define how yaw rotation is applied to the spawn location of each
	projectile relative to the last projectile spawned.

	Without a curve the rotation will be a random float in the range 0 to 360. This means
	there is a chance projectiles can spawn close to each other which may not be desired
	behaviour. With a curve you can define the chances of this happening.

	X axis = arbitrary. A random point will be picked along the X axis and its Y value
	(normalized to the range 0...180, and given random sign) will be the rotation to
	apply to the next projectile
	Y axis = rotation to apply to next projectile. For clarity you may want to set the
	range for the Y axis to be from 0 to 180 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "RTS")
	UCurveFloat * YawRotationCurve;

	/* Just like the rotation curve but for the distance to spawn the projectile from
	the center. This one is not relative to the last projectile spawned though.
	Automatically normalized at runtime. If no curve is specified a linear curve will
	be used (random value from 0 to Radius, with each value equal probability).

	X axis = arbitrary. A random point will be picked along the X axis and its Y value
	(normalized to the range 0...Radius) will be the distance from center next projectile
	will spawn at
	Y axis = distance from center (mapped from [YAxisMin, YAxisMax] to [0, Radius]) */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "RTS")
	UCurveFloat * DistanceFromCenterCurve;

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	virtual void OnListenedProjectileHit(int32 AbilityInstanceUniqueID) override;
	//~ End AAbilityBase interface

	/* Return random spot within radius around this, taking into account the constraints of
	the rotation and distance curves.
	@return - random spot in radius */
	FVector GetRandomSpot(int32 UniqueID);

	float GetRandomDistanceFromCenter(int32 UniqueID);
	/* Get random yaw rotation taking into account constraints from rotation curve.
	@param InYaw - yaw rotation to find new rotation from */
	float GetRandomYawRotation(float InYaw, int32 UniqueID);

	/* Spawn a projectile */
	UFUNCTION()
	void SpawnProjectile(int32 UniqueID);

	/* What we call when the artillirty strike is finished */
	void EndEffect(int32 UniqueID);

private:

	void SetupCurveValues();

	/* Axis values from curves */
	float RotationCurveMinX, RotationCurveMaxX, RotationCurveMinY, RotationCurveMaxY;
	float DistanceCurveMinX, DistanceCurveMaxX, DistanceCurveMinY, DistanceCurveMaxY;

	/* Call SpawnProjectile after delay
	@param Delay - delay for calling function */
	void Delay_SpawnProjectile(int32 UniqueID, float Delay);
};