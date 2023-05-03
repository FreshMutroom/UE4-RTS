// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "PreciseProjectileMovement.generated.h"

class UCurveVector;
class USphereComponent;
class AProjectileBase;
enum class EPreciseProjectileMovementMode : uint8;


enum class EStopSimulatingReason : uint8
{
	HitSomething, 
	OutsideWorldBounds,
};


/**
 *	Has two modes:
 *	- 'Regular straight shooting' mode 
 *	- 'Control exact location and rotation of projectile by using curve assets' mode, 
 *	hence the name of this projectile
 *
 *	Probably won't support sliding (for performance).
 *	Probably won't support bouncing (for performance).
 *
 *	Perhaps deriving from UActorComponent could be possible. 
 */
UCLASS(ShowCategories = (Velocity))
class RTS_VER2_API UPreciseProjectileMovement : public UMovementComponent
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	// Just copied UProjectileMovementComponent
	bool HasStoppedSimulation() const; 
	bool CheckStillInWorld();

public:

	void SetMovementMode(EPreciseProjectileMovementMode InMovementMode);

	/* Called by the owning projectile when it is fired */
	void OnProjectileFired(AProjectileBase * InProjectile, USphereComponent * ProjectilesSphereComp);

	void CurveMode_OnDescentStart(const FVector & DescentReferenceLocation);

	/* Set whether tick does anything or not */
	void SoftSetTickEnabled(bool bTickEnabled);

protected:

	void HandleBlockingHit(const FHitResult & Hit);

	/* Make sure to tell projectile about this */
	void StopSimulating(const FHitResult & HitResult, EStopSimulatingReason StopReason);

	/* Get the projectile that this component is a prt of */
	AProjectileBase * GetProjectile() const;

	//-------------------------------------------------------
	//	Data
	//-------------------------------------------------------

	/** 
	 *	Curve that has the position offsets relative to where the projectile was launched.
	 *
	 *	X axis of curve = time 
	 *	Y axis of curve = offset 
	 *
	 *	Positions are relative to the launch location. Then at some point they switch to 
	 *	being relative to the target location. The switch happens at 
	 *	ALeaveThenComeBackProjectile::AscentDuration + AscentDelay into the curve.
	 *
	 *	Tip for creating these curves:
	 *	Make sure the mesh's location relative to the sphere comp is how you want it because 
	 *	when you start changing it then these curves will start to look different. 
	 *	Note to self: think of a way to correct for this. A simple 'offset' FVector might 
	 *	be enough.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveVector * PositionCurve;
	
	/** 
	 *	Rotation around axis. 
	 *	X curve = roll 
	 *	Y curve = pitch 
	 *	Z curve = yaw
	 *
	 *	X axis = time 
	 *	Y axis = amount of rotation in degrees, range: [0, 360)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveVector * RotationCurve;

	/* Location/rotation curves are relative to */
	FVector ReferenceLocation;
	FRotator ReferenceRotation;

	float TimeSpentAlive; 

	UPROPERTY()
	EPreciseProjectileMovementMode MovementMode;

	bool bTickMostlyPaused;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
