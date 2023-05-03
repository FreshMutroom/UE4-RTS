// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/CollidingProjectileBase.h"
#include "LeaveThenComeBackProjectile.generated.h"

class UPreciseProjectileMovement;


UENUM()
enum class EPreciseProjectileMovementMode : uint8
{
	/** 
	 *	Projectile will only travel in a straight line. Quick to set up but limited in what you 
	 *	can do.
	 */
	StraightLine,
	
	/* Use curve assets to specify exactly how projectile moves and rotates */
	CurveAssets 
};


/**
 *	A projectile that travels for a while, disappears, then reappears at another location.
 *	e.g. 
 *	- SCII zerg ravager's ability
 *	- terran nukes
 *
 *	Uses a projectile movement component that allows defining the exact position of the projectile. 
 *	The movement component has two modes:
 *	- straight firing mode, which would be something you could use for the zerg ravager ability.
 *	- precise mode, where you define the exact location/rotation of the projectile at all times 
 *	in its flight using curves (if using this mode make sure to specify the curves you want to 
 *	use on the movement component).
 *
 *	FIXME: Think of a different name maybe
 *
 *	My notes: quite a bit of unused data on this if choosing MovementMode == CurveAssets.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API ALeaveThenComeBackProjectile : public ACollidingProjectileBase
{
	GENERATED_BODY()
	
public:

	ALeaveThenComeBackProjectile(); 

	virtual void BeginPlay() override;

	//~ Begin AProjectileBase interface
	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;
	//~ End AProjectileBase interface

	//~ Begin ACollidingProjectileBase interface
	virtual void SetupCollisionChannels(ETeam Team) override;
	virtual FRotator GetStartingRotation(const FVector & StartLoc, const FVector & TargetLoc, float RollRotation) const override;
	//~ End ACollidingProjectileBase interface

	/** 
	 *	Get the velocity the projectile should start its ascent at in local space
	 *	
	 *	@param StartLocation - world location the projectile was fired from 
	 *	@param TargetLocation - world location the projectile is being fired at 
	 */
	FVector GetInitialVelocity(const FVector & StartLocation, const FVector & TargetLocation) const;

	/**
	 *	Get the rotation the projectile should have at the start of its descent
	 *
	 *	@param StartLocation - world location the projectile was fired from
	 *	@param TargetLocation - world location the projectile is being fired at
	 */
	FRotator GetDescentStartRotation(const FVector & StartLocation, const FVector & TargetLocation) const;

	/**
	 *	Get the world location the projectile should start its descent at
	 *
	 *	@param StartLocation - world location the projectile was fired from
	 *	@param TargetLocation - world location the projectile is being fired at
	 */
	FVector GetDescentStartLocation(const FVector & StartLocation, const FVector & TargetLocation, const FRotator & DescentStartRotation) const;

	/* Called when the ascent portion has completed */
	void OnAscentCompleted();

	void SetProjectileVisibility(bool bVisibility);

	/* Start the descent portion */
	void BeginDescent();

	void SetupCollisionForDescent(ETeam Team);

	void OnProjectileStop(const FHitResult & Hit); 
	
	//~ Begin AProjectileBase interface
	virtual void OnProjectileStopFromMovementComp(const FHitResult & Hit) override;
	virtual void SetupForEnteringObjectPool() override;
	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true) override;
	//~ End AProjectileBase interface

#if !UE_BUILD_SHIPPING
	virtual bool IsFitForEnteringObjectPool() const override;
#endif

private:

	void Delay(FTimerHandle & TimerHandle, void(ALeaveThenComeBackProjectile::* Function)(), float Delay);

protected:

	//-------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------

	/* Movement component */
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	UPreciseProjectileMovement * MoveComp;

	// These will go unused if MovementMode == CurveAssets
	FRotator DescentStartingRotation;
	FVector DescentStartingLocation;

	/* How long the projectile's ascent lasts */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = "0.0001"))
	float AscentDuration;

	/** 
	 *	The time between when the projectile finishes its ascent and when it reappear 
	 *	to do its descent. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = 0))
	float DescentDelay;

	/* If choosing CurveAssets make sure to set the curves on the movement component */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior")
	EPreciseProjectileMovementMode MovementMode;

	//------------------------------------------------------------------------------------------
	//	Basic move mode data

	/* How fast projectile moves when ascending */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (EditCondition = bCanEditStraightLineMoveModeProperties))
	float AscentSpeed;

	/** 
	 *	Angle to use for ascent. Yaw/Roll are relative to looking towards the target location 
	 *	and pitch is always whatever you set here (unnecessary piece of info). 
	 *	90 pitch = fires straight into the air. 
	 *
	 *	Note to self: large angles may cause projectile to be outside of fog area
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (EditCondition = bCanEditStraightLineMoveModeProperties))
	FRotator AscentRelativeAngle;

	/** 
	 *	Height to spawn projectile at for descent. 
	 *	
	 *	No matter DescentRelativeAngle's values the descent always starts this high above the 
	 *	target location. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (EditCondition = bCanEditStraightLineMoveModeProperties))
	float DescentStartingHeight;

	/** How fast to move for descent */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (EditCondition = bCanEditStraightLineMoveModeProperties))
	float DescentSpeed;

	/** 
	 *	Angle of projectile to use for decent. This is relative to looking towards the target 
	 *	location (unnecessary piece of info). 
	 *
	 *	-90 on pitch = straight down  
	 *	
	 *	Whatever angle you choose the projectile will always be aimed at (and hit if nothing 
	 *	else gets in the way) the target location.
	 *
	 *	Note to self: large angles may cause projectile to be outside of fog area
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (EditCondition = bCanEditStraightLineMoveModeProperties))
	FRotator DescentRelativeAngle;

	//-----------------------------------------------------------------------------------------

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditStraightLineMoveModeProperties;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
