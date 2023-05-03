// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectileBase.h"
#include "NoCollisionTimedProjectile.generated.h"

class UParticleSystemComponent;
class UNoCollisionProjectileMovement;


/*
 *	A projectile with no collision that is not replicated. This kind of projectile
 *	is similar to quad cannons in C&C generals, but can optionally be set to deal damage
 *	after a delay instead of instantly. Hits always register at target location. 
 *	These projectiles can be arcing. They can also pseudo collide by just checking 
 *	if they are close enough to their target. 
 *
 *	The movement component has a few extra variables on it that you might want to look at if 
 *	this is an arcing projectile.
 */
UCLASS(Blueprintable)
class RTS_VER2_API ANoCollisionTimedProjectile final : public AProjectileBase
{
	GENERATED_BODY()

public:

	ANoCollisionTimedProjectile();

protected:

	/* Mesh */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent * Mesh;

	/* Movement component */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UNoCollisionProjectileMovement * MoveComp;

	/** 
	 *	A workaround if projectile appears to be disappearing before/after reaching target.
	 *	If projectiles appear to be disappearing too far before/after target then adjust this.
	 *	
	 *	Lower values imply hit happens sooner 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Lifetime")
	float LifetimeModifier;

	// TODO: test this with values != 1 or consider removing completly if issues arise 
	// (removing as editdefaultsonly is enough, make sure ctor sets it to 1)
	/** 
	 *	Decides how fast a hit is triggered. Only modify this if you want projectiles to
	 *	deal hits before/after the actual mesh reaches its target, for example like quad
	 *	cannons in C&C generals.
	 *	
	 *	0 = hit is dealt the moment projectile is fired
	 *	0.5 = hit is dealt after projectile travels half the distance between it and the target
	 *	1 = hit is dealt when projectile considers itself as reached target
	 *	1+ = hit is dealt sometime after projectile considers itself as reached target 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Lifetime")
	float HitSpeedModifier;

	/** 
	 *	If this is greater than 0 then when the projectile reaches its target or target location 
	 *	it will not disappear but will stay there. Then after this delay is will register a hit. 
	 *	This will be ignored if HitSpeedModifier != 1.f
	 *
	 *	I added this so artillery beacons can stay at where they are thrown for a little until 
	 *	disappearing.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Lifetime", meta = (ClampMin = 0, EditCondition = bCanEditRegisterHitDelay))
	float RegisterHitDelay;

	/** 
	 *	If projectile is fired and target falls to zero health before hit is registered should
	 *	this still register a hit i.e. for visuals and AoE damage purposes. If false then projectile
	 *	still travels full distance but then just disappears 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	bool bHitFallenTargets;

	virtual void SetupForEnteringObjectPool() override;

	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true) override;

	/** 
	 *	Return how long before considering target hit
	 *	
	 *	@param Distance - distance from firer to target 
	 */
	float CalculateLifetime(float Distance);

	/* Timer handle for when to register hit on target */
	FTimerHandle TimerHandle_Hit;

	/* Do OnMeshReachedTarget and OnHitTarget */
	void OnFireAtTargetComplete();

	void RegisterHitOnTarget();

public:

	void OnFireAtLocationComplete();

protected:

	void RegisterHitAtTargetLocation();

	/* When to hide the mesh */
	void OnMeshReachedTarget();

	/* When to register hit on target */
	void OnHitTarget();

	/* Called when projectile is fired at a location and the mesh reaches it */
	void OnMeshReachedTargetLocation();

	/* Called when projectile is fired at a location and should now register a hit */
	void OnHitTargetLocation();

	/* Hide mesh, stop movement */
	void MakeInactive();

	/* Hide trail particles */
	void HideTrails();

	/* Hit result for passing into AProjectileBase::OnHit(). Declared here to avoid recreating
	every time fired */
	FHitResult HitResult;

private:

	void Delay(FTimerHandle & TimerHandle, void(ANoCollisionTimedProjectile::* Function)(), float Delay);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditRegisterHitDelay;
#endif

public:

	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation) override;

	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

	/** 
	 *	Called when an arcing projectile fired at a location times out likely because it never 
	 *	got close enough to its target location for it to count as being hit
	 */
	void OnTimedOut();

#if !UE_BUILD_SHIPPING
	virtual bool IsFitForEnteringObjectPool() const override;
	virtual bool AreAllTimerHandlesCleared() const override;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

protected:
	void RunPostEditLogic();
#endif
};
