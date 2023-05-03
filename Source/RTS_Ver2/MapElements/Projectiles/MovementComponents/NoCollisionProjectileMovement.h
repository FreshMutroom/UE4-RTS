// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/MovementComponents/RTSProjectileMovement.h"
#include "NoCollisionProjectileMovement.generated.h"

class ANoCollisionTimedProjectile;


UENUM()
enum class EArcingProjectileTrajectoryMethod : uint8
{
	/** 
	 *	With this method the projectile's initial velocity is always the same. It is the arc that 
	 *	will vary. 
	 *	UProjectileMovementComponent::InitialSpeed is what you should tweak to get the projectile 
	 *	firing how you want it. Also UNoCollisionProjectileMovement::bUseHighArc also matters.
	 *	For this to work InitialSpeed has to be high enough. 
	 */
	ChooseInitialVelocity, 

	/**   
	 *	UNoCollisionProjectileMovement::ArcValue is what you should tweak. InitialSpeed is ignored. 
	 *	With this method it's the arc that will stay the same. It is the projectile's launch speed 
	 *	that will vary (I think). 
	 *	
	 *	I think this option is more safer in that it can never fail to find an arc (I think. I know 
	 *	ChooseInitialVelocity can fail if InitialSpeed is too low).
	 */
	ChooseArc
};

UENUM()
enum class ENoCollisionProjectileMode : uint8
{
	/* Fires straight and is being fired at a actor */
	StraightFiringAtTarget, 
	
	/* Fires straight and is being fired at a location */
	StraightFiringAtLocation, 
	
	// ArcedFiringAtTarget - do not allow this right now
	
	/* Fires in an arc and is being fired at a location */
	ArcedFiringAtLocation,
};


/**
 *	Projectile movement component for ANoCollisionTimedProjectile.
 */
UCLASS()
class RTS_VER2_API UNoCollisionProjectileMovement : public URTSProjectileMovement
{
	GENERATED_BODY()
	
public: 

	UNoCollisionProjectileMovement();

protected:

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

	/** 
	 *	Return true if the projectile is considered close enough to where its being fired that 
	 *	it considers it has hit it. 
	 */
	bool IsCloseEnoughToTargetLocation() const;

public:

	EArcingProjectileTrajectoryMethod GetArcCalculationMethod() const;
	float GetProjectileLifetime() const;
	bool UseHighArc() const;
	float GetArcValue() const;

protected:

	//------------------------------------------------------
	//	Data
	//------------------------------------------------------

	/* Projectile this move comp belongs to */
	ANoCollisionTimedProjectile * Projectile;

public:

	/* The world location the projectile is being fired at but only if this projectile also 
	arcs. If it's being fired at an actor then this will likely be set to some invalid value */
	FVector TargetLocation;

	ENoCollisionProjectileMode TargetType;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditGravityRelatedProperties))
	EArcingProjectileTrajectoryMethod ArcCalculationMethod;

	/** Value that gets passed into UGameplayStatics::SuggestProjectileVelocity */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditInitialVelocityArcCalcProperties))
	bool bUseHighArc;

	/** 
	 *	The value that gets passed into UGameplayStatics::SuggestProjectileVelocity_CustomArc. 
	 *	Play around with this to get the arc you want. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditArcArcCalcProperties))
	float ArcValue;

	/** 
	 *	Only relevant if the projectile is being fired at a location and has non-zero gravity. 
	 *	How close the projectile has to get to the target location for it to be considered hit it 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = bCanEditGravityRelatedProperties))
	float HitDistance;

protected:

	/** 
	 *	Only relevant if the projectile is being fired at a location and has non-zero gravity. 
	 *	How long the projectile can exist for before it will magically be added to pool. No hit will 
	 *	be registered. Counted from the time it is fired.
	 *	
	 *	If the projectile is constantly missing its target then increase HitDistance
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001", EditCondition = bCanEditGravityRelatedProperties))
	float Lifetime;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditGravityRelatedProperties;
	
	UPROPERTY()
	bool bCanEditInitialVelocityArcCalcProperties;
	
	UPROPERTY()
	bool bCanEditArcArcCalcProperties;
#endif

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
