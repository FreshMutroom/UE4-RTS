// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCameraMovement.h"

#include "Settings/ProjectSettings.h"
#include "Statics/Statics.h" // For ENVIRONMENT_CHANNEL


UPlayerCameraMovement::UPlayerCameraMovement(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPlayerCameraMovement::BeginPlay()
{
	Super::BeginPlay();

	/* Make sure camera starts on the ground */
	MoveToGround();
}

void UPlayerCameraMovement::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction *ThisTickFunction)
{	
	/** 
	 *	A lot of this functions code is taken from UFloatingPawnMovement::TickComponent
	 */
	
	if (ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PawnOwner == nullptr || UpdatedComponent == nullptr)
	{
		return;
	}

	const AController * Controller = PawnOwner->GetController();
	if (Controller != nullptr && Controller->IsLocalController())
	{
		// apply input for local player
		// Begin code taken from UFloatingPawnMovement::ApplyControlInputToVelocity
		const FVector ControlAcceleration = GetPendingInputVector().GetClampedToMaxSize(1.f);

		const float AnalogInputModifier = (ControlAcceleration.SizeSquared() > 0.f ? ControlAcceleration.Size() : 0.f);
		const float MaxPawnSpeed = GetMaxSpeed() * AnalogInputModifier;
		const bool bExceedingMaxSpeed = IsExceedingMaxSpeed(MaxPawnSpeed);

		if (AnalogInputModifier > 0.f && !bExceedingMaxSpeed)
		{
			// Apply change in velocity direction
			if (Velocity.SizeSquared() > 0.f)
			{
				// Change direction faster than only using acceleration, but never increase velocity magnitude.
				const float TimeScale = FMath::Clamp(DeltaTime * TurningBoost, 0.f, 1.f);
				Velocity = Velocity + (ControlAcceleration * Velocity.Size() - Velocity) * TimeScale;
			}
		}
		else
		{
			// Dampen velocity magnitude based on deceleration.
			if (Velocity.SizeSquared() > 0.f)
			{
				const FVector OldVelocity = Velocity;
				const float VelSize = FMath::Max(Velocity.Size() - FMath::Abs(Deceleration) * DeltaTime, 0.f);
				Velocity = Velocity.GetSafeNormal() * VelSize;

				// Don't allow braking to lower us below max speed if we started above it.
				if (bExceedingMaxSpeed && Velocity.SizeSquared() < FMath::Square(MaxPawnSpeed))
				{
					Velocity = OldVelocity.GetSafeNormal() * MaxPawnSpeed;
				}
			}
		}

		// Apply acceleration and clamp velocity magnitude.
		const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxPawnSpeed)) ? Velocity.Size() : MaxPawnSpeed;
		Velocity += ControlAcceleration * FMath::Abs(Acceleration) * DeltaTime;
		Velocity = Velocity.GetClampedToMaxSize(NewMaxSpeed);

		ConsumeInputVector();

		// End code taken from UFloatingPawnMovement::ApplyControlInputToVelocity

		/* Leaving this out. All bounds checking should be handled by the rectangles placed 
		by RTSLevelVolume and the line trace done here */
		//LimitWorldBounds();
		
		bPositionCorrected = false;

		// Move actor
		FVector Delta = Velocity * DeltaTime;

		// Do this after multi by DeltaTime because we want this to ignore it
		if (CameraOptions::bGlueToGround)
		{
			GlueToGround(Delta);
		}
		
		if (!Delta.IsNearlyZero(1e-6f))
		{
			const FVector OldLocation = UpdatedComponent->GetComponentLocation();
			const FQuat Rotation = UpdatedComponent->GetComponentQuat();

			FHitResult Hit(1.f);
			SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);

			if (Hit.IsValidBlockingHit())
			{
				HandleImpact(Hit, DeltaTime, Delta);
				// Try to slide the remaining distance along the surface.
				SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
			}

			// Update velocity
			// We don't want position changes to vastly reverse our direction (which can happen due to penetration fixups etc)
			if (!bPositionCorrected)
			{
				const FVector NewLocation = UpdatedComponent->GetComponentLocation();
				Velocity = ((NewLocation - OldLocation) / DeltaTime);
			}
		}

		// Finalize
		UpdateComponentVelocity();
	}
}

void UPlayerCameraMovement::MoveToGround()
{
	/* Actually probably better to have the player start raycast during design time and 
	figure out where the ground is then adjust its location. That way the player's camera 
	will spawn with correct Z axis value in the first place and now ray trace needs to 
	be done at runtime TODO make sure player starts do this although may want to add a 
	little height like 2.f or something dunno though will only be potential issue if the 
	spring arm is set to do a trace for collision */
}

void UPlayerCameraMovement::GlueToGround(FVector & InMovementVector)
{
	const float TraceHalfHeight = 2048.f;
	
	const FVector CompLoc = UpdatedComponent->GetComponentLocation();
	const FVector TraceStart = FVector(CompLoc.X, CompLoc.Y, CompLoc.Z + TraceHalfHeight);
	const FVector TraceEnd = FVector(CompLoc.X, CompLoc.Y, CompLoc.Z - TraceHalfHeight);

	FHitResult HitResult;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CAMERA_CHANNEL))
	{
		const float DifferenceZ = CompLoc.Z - HitResult.ImpactPoint.Z;
		
		InMovementVector.Z -= DifferenceZ;
	}
}

bool UPlayerCameraMovement::ResolvePenetrationImpl(const FVector & Adjustment, const FHitResult & Hit, const FQuat & NewRotationQuat)
{
	bPositionCorrected |= Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotationQuat);
	return bPositionCorrected;
}

void UPlayerCameraMovement::SetMaxSpeed(float InMaxSpeed)
{
	MaxSpeed = InMaxSpeed;
}

void UPlayerCameraMovement::SetAcceleration(float InAcceleration)
{
	Acceleration = InAcceleration;
}

void UPlayerCameraMovement::SetDeceleration(float InDeceleration)
{
	Deceleration = InDeceleration;
}

void UPlayerCameraMovement::SetTurningBoost(float InTurningBoost)
{
	TurningBoost = InTurningBoost;
}
