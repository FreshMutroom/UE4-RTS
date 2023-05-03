// Fill out your copyright notice in the Description page of Project Settings.

#include "HomingProjectileMovement.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"

#include "Statics/DevelopmentStatics.h"
#include "Statics/Statics.h"
#include "MapElements/Projectiles/HomingProjectile.h"

UHomingProjectileMovement::UHomingProjectileMovement()
{
}

void UHomingProjectileMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	//DEBUG_MESSAGE("Homing projectile velocity size", Velocity.Size());

	/* Super just nulls UpdatedComponent if it is pending kill */
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!KeepTrackingTarget())
	{
		Projectile->OnTargetNoLongerValid();
		return;
	}

	TimeSpentInFlight = GetWorld()->GetTimerManager().GetTimerElapsed(Projectile->GetLifetimeTimerHandle());

	if (Statics::IsValid(Projectile->GetTarget()))
	{
		LastTargetLocation = GetLockOnLocation();
	}

	//~ Most of below has been borrowed from ProjectileMovementComponents tick
	FHitResult Hit(1.f);
	const float TimeTick = DeltaTime;

	Hit.Time = 1.f;
	const FVector OldVelocity = Velocity;
	const FVector MoveDelta = ComputeMoveDelta(OldVelocity, TimeTick);

	const FQuat NewRotation = !OldVelocity.IsNearlyZero(0.01f) ? OldVelocity.ToOrientationQuat() : UpdatedComponent->GetComponentQuat();

	TGuardValue<EMoveComponentFlags> ScopedFlagRestore(MoveComponentFlags, MoveComponentFlags | MOVECOMP_NeverIgnoreBlockingOverlaps);
	MoveUpdatedComponent(MoveDelta, NewRotation, true, &Hit);

	if (!bIsActive)
	{
		return;
	}

	// Handle hit result after movement
	if (!Hit.bBlockingHit)
	{
		// Only calculate new velocity if events didn't change it during the movement update.
		if (Velocity == OldVelocity)
		{
			Velocity = ComputeVelocity(Velocity, TimeTick);
		}

		/* Check if have pseudo-hit target */
		if (IsCloseEnoughToTarget())
		{
			/* TODO: ImpactPoint could do with being more accurate maybe */
			FHitResult HitResult;
			HitResult.Actor = Projectile->GetTarget();
			HitResult.ImpactPoint = Projectile->GetActorLocation();
			Projectile->OnHit(HitResult);
			// If UpdateComponentVelocity() causes crashes then add this back in
			//return;
		}
	}
	else
	{
		// Only calculate new velocity if events didn't change it during the movement update.
		if (Velocity == OldVelocity)
		{
			// re-calculate end velocity for partial time
			Velocity = (Hit.Time > KINDA_SMALL_NUMBER) ? ComputeVelocity(OldVelocity, TimeTick * Hit.Time) : OldVelocity;
		}

		Projectile->OnHit(Hit);
	}

	UpdateComponentVelocity();
}

FVector UHomingProjectileMovement::ComputeMoveDelta(const FVector & InVelocity, float DeltaTime) const
{
	// Duplicate of UProjectileMovementComponent::ComputeMoveDelta

	const FVector NewVelocity = ComputeVelocity(InVelocity, DeltaTime);
	const FVector Delta = (InVelocity * DeltaTime) + (NewVelocity - InVelocity) * (0.5f * DeltaTime);

	return Delta;
}

FVector UHomingProjectileMovement::ComputeVelocity(FVector InitialVelocity, float DeltaTime) const
{
	/* Just use initial velocity but change it's direction to face target and
	add acceleration */
	const float CurrentSpeed = InitialVelocity.Size();
	FVector LookAtVector = LastTargetLocation - UpdatedComponent->GetComponentLocation();
	LookAtVector.Normalize();
	LookAtVector *= CurrentSpeed;

	/* Add on acceleration */
	const float AccelerationAmount = ComputeAccelerationAmount(InitialVelocity, DeltaTime);
	FVector NewVelocity = LookAtVector * (1.f + (AccelerationAmount /* * DeltaTime*/));

	ClampVelocity(NewVelocity);

	return NewVelocity;
}

void UHomingProjectileMovement::ClampVelocity(FVector & InVelocity) const
{
	/* Allowed to go 1% over max speed */
	const float ErrorTolerance = 1.01f;
	if (MaxSpeed > 0.f && (InVelocity.SizeSquared() > FMath::Square(MaxSpeed) * ErrorTolerance))
	{
		InVelocity = InVelocity.GetClampedToMaxSize(MaxSpeed);
	}
}

float UHomingProjectileMovement::ComputeAccelerationAmount(const FVector & InVelocity, float DeltaTime) const
{
	float AccelerationAmount = 0.f;

	if (AccelerationCurve != nullptr)
	{
		/* Already takes DeltaTime into consideration */
		AccelerationAmount = AccelerationCurve->GetFloatValue(TimeSpentInFlight);
	}

	return AccelerationAmount;
}

bool UHomingProjectileMovement::IsCloseEnoughToTarget() const
{
	const float Distance = (Projectile->GetActorLocation() - LastTargetLocation).SizeSquared()
		- FMath::Square(Projectile->GetSphere()->GetScaledSphereRadius() + TargetBoundsRadius);

	return Distance < 0.f;
}

bool UHomingProjectileMovement::KeepTrackingTarget() const
{
	if (Projectile->CanHitDefeatedTargets())
	{
		return true;
	}
	else
	{
		return Statics::IsValid(Projectile->GetTarget())
			&& !Statics::HasZeroHealth(Projectile->GetTarget());
	}
}

FVector UHomingProjectileMovement::GetLockOnLocation() const
{
	return Projectile->GetTarget()->GetActorLocation();
}

void UHomingProjectileMovement::SetInitialValues(AHomingProjectile * InProjectile)
{
	Projectile = InProjectile;

	InitialSpeed = InProjectile->GetInitialSpeed();
	MaxSpeed = InProjectile->GetMaxSpeed();
	AccelerationCurve = InProjectile->GetAccelerationCurve();
}

void UHomingProjectileMovement::OnOwningProjectileFired()
{
	TimeSpentInFlight = 0.f;
	LastTargetLocation = GetLockOnLocation();

	// TODO: this uses a cyliner. Consider a different approach
	/* Get target bounds */
	float Throwaway;
	Projectile->GetTarget()->GetComponentsBoundingCylinder(TargetBoundsRadius, Throwaway, false);

	/* Starting velocity */
	Velocity = LastTargetLocation - UpdatedComponent->GetComponentLocation();
	Velocity.Normalize();
	Velocity *= InitialSpeed;

	// TODO: Call this maybe? test it
	//UpdateComponentVelocity();
}
