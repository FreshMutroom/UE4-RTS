// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "HomingProjectileMovement.generated.h"

class UCurveFloat;
class AHomingProjectile;

/**
 *	Movement component for homing projectiles that never miss their target. Very basic. Just
 *	always make updated component face target
 */
UCLASS()
class RTS_VER2_API UHomingProjectileMovement final : public UMovementComponent
{
	GENERATED_BODY()

public:

	UHomingProjectileMovement();

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/* ProjectileMovementComponent.h: "Compute the distance we should move in the given time,
	at a given a velocity" */
	virtual FVector ComputeMoveDelta(const FVector& InVelocity, float DeltaTime) const;

	virtual FVector ComputeVelocity(FVector InitialVelocity, float DeltaTime) const;
	void ClampVelocity(FVector & InVelocity) const;

	virtual float ComputeAccelerationAmount(const FVector& InVelocity, float DeltaTime) const;

	/* If projectiles bCanOnlyHitTarget is true then this is used for 'collision' with target.
	Note takes all 3 axis into account since homing projectiles are likely to be used for
	surface-to-air/air-to-surface projectiles */
	virtual bool IsCloseEnoughToTarget() const;

	/* Check whether we should keep going or stop */
	bool KeepTrackingTarget() const;

	/* Time projectile has been in flight */
	float TimeSpentInFlight;

	/* Location target was at last tick */
	FVector LastTargetLocation;

	/* Radius of targets bounds */
	float TargetBoundsRadius;

	/* Return the location to be heading towards */
	FVector GetLockOnLocation() const;

	//~ Variables set by owning projectile 

	float InitialSpeed;
	float MaxSpeed;

	UPROPERTY()
	UCurveFloat * AccelerationCurve;

	/* Reference to owning projectile */
	UPROPERTY()
	AHomingProjectile * Projectile;

public:

	void SetInitialValues(AHomingProjectile * InProjectile);

	virtual void OnOwningProjectileFired();
};
