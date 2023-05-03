// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/CollidingProjectileBase.h"
#include "HomingProjectile.generated.h"

class UHomingProjectileMovement;
class USphereComponent;


/**
 *	A non-replicating projectile that always finds its target. An example would be a stalker
 *	shot in starctaft II. Can also be used for missles that seek their targets.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AHomingProjectile final : public ACollidingProjectileBase
{
	GENERATED_BODY()

public:

	AHomingProjectile();

protected:

	virtual void BeginPlay() override;

	/* Movement comp. Some values on this are overridden by properties in this class */
	UPROPERTY()
	UHomingProjectileMovement * MoveComp;

	/* If target falls to zero health should this projectile keep going or just disappear.
	If true AoE radius needs to	be greater than 0 for any damage to happen in this case.
	If false projectile will just disppear the moment target reaches zero health.

	Targets that become null will have their last known location hit if this is true */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Collision")
	bool bCanHitDefeatedTargets;

	/* In regards to selectables, will this projectile ignore every other selectable
	and only collide with target? This overrides bCanHitEnemies and bCanHitFriendlies.
	bCanHitWorld still has an effect */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Collision")
	bool bCanOnlyHitTarget;

	/* Initial speed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float InitialSpeed;

	/* Max speed if using an acceleration curve. 0 = no limit. Max speed can also kind of
	be derived from the acceleration curve */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	float MaxSpeed;

	/* Curve for acceleration to apply each frame. If no curve is specified no acceleration
	will be applied and projectile will be at a constant speed.

	X axis = time
	Y axis = (Value + 1) * CurrentSpeed

	Probably wanna use really small Y axis values in the range of like 0 to 0.01 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	UCurveFloat * AccelerationCurve;

	virtual void SetupForEnteringObjectPool() override;

	/* Call right before adding to object pool */
	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true) override;

	virtual void SetupCollisionChannels(ETeam Team) override;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

private:

	void SetupAccelerationCurve();

	/* Function to call with timer handle. Timer handle needs to run for movement component
	to get correct acceleration */
	void DoNothing();

	/* Call function with no return after delay
	@param TimerHandle - timer handle to use
	@param Function - function to call
	@param Delay - delay for calling function */
	void Delay(FTimerHandle & TimerHandle, void(AHomingProjectile:: *Function)(), float Delay);

public:

	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation) override;

	virtual void OnHit(const FHitResult & Hit) override;

	//~ Getters and setters 

	AActor * GetTarget() const;
	const FTimerHandle & GetLifetimeTimerHandle() const;
	float GetInitialSpeed() const;
	float GetMaxSpeed() const;
	UCurveFloat * GetAccelerationCurve() const;
	bool CanOnlyHitTarget() const;
	USphereComponent * GetSphere() const;
	bool RegistersHitOnTimeout() const;
	bool CanHitDefeatedTargets() const;

	/* To be called by movement component when target is no longer valid */
	void OnTargetNoLongerValid();

#if !UE_BUILD_SHIPPING
	virtual bool IsFitForEnteringObjectPool() const override;
#endif
};
