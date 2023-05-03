// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PlayerCameraMovement.generated.h"


/**
 *	The movement component that decides how the players pawn (which has a camera attached to it) 
 *	moves. It is pretty much a copy of UFloatingPawnMovement but does a raycast each tick to 
 *	make sure updated component stays glued to the ground
 *
 */
UCLASS()
class RTS_VER2_API UPlayerCameraMovement : public UPawnMovementComponent
{
	GENERATED_BODY()
	
public:

	UPlayerCameraMovement(const FObjectInitializer & ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	
protected:

	/* This and GlueToGround sound too similar. This moves the component while the other 
	adds to velocity vector (probably same effect anyway) but should maybe change the 
	func names */
	void MoveToGround();

	/* Do ray trace against environment so the updated component can be placed on the ground 
	as opposed to ignoring gravity. */
	void GlueToGround(FVector & InMovementVector);

	/* Some variables. A lot of them taken from UFloatingPawnMovement. None of them are exposed 
	to blueprint since they will be overridden by the game user settings. In order to edit the 
	behavior of the camera the prefered way is to either: 
	1. edit settings inside URTSGameUserSettings via C++
	2. do it through your settings menu in-game. 
	This ensures the settings will be written to disk */

	/** Maximum velocity magnitude allowed for the controlled Pawn. */
	float MaxSpeed;

	/** Acceleration applied by input (rate of change of velocity) */
	float Acceleration;

	/** Deceleration applied when there is no input (rate of change of velocity) */
	float Deceleration;

	/**
	 * Setting affecting extra force applied when changing direction, making turns have less drift and become more responsive.
	 * Velocity magnitude is not allowed to increase, that only happens due to normal acceleration. It may decrease with large direction changes.
	 * Larger values apply extra force to reach the target direction more quickly, while a zero value disables any extra turn force.
	 */
	float TurningBoost;

	/** Set to true when a position correction is applied. Used to avoid recalculating velocity when this occurs. */
	UPROPERTY(Transient)
	uint32 bPositionCorrected : 1;

private:

	/* Overridding this as per UFloatingPawnMovement */
	virtual float GetMaxSpeed() const override { return MaxSpeed; }

protected:
	/* Overridding this as per UFloatingPawnMovement */
	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotationQuat) override;

public:

	void SetMaxSpeed(float InMaxSpeed);
	void SetAcceleration(float InAcceleration);
	void SetDeceleration(float InDeceleration);
	void SetTurningBoost(float InTurningBoost);
};
