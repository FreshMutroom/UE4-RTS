// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "CollidingProjectileBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UArrowComponent;


/**
 *	Base class for projectiles that collide with selectables and/or the environment
 */
UCLASS(Abstract, NotBlueprintable)
class RTS_VER2_API ACollidingProjectileBase : public AProjectileBase
{
	GENERATED_BODY()

public:

	ACollidingProjectileBase();

protected:

	virtual void BeginPlay() override;

	/* Collision comp. Also can be used for projectile mesh positioning */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	USphereComponent * SphereComp;

#if WITH_EDITORONLY_DATA
	/* Arrow to no know what way it faces in editor */
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UArrowComponent * Arrow;
#endif

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent * Mesh;

	/* Can this projectile collide with the world? i.e. non-selectables such as the
	landscape */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Collision")
	bool bCanHitWorld;

	/* Will colliding with a enemy trigger a hit? */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Collision", meta = (EditCondition = bCanEditTeamCollisionProperties))
	bool bCanHitEnemies;

	/** 
	 *	Will colliding with a friendly trigger a hit? Self included. This is important.
	 *	Make sure the socket if far enough away from the mesh so that it does not collide
	 *	instantly with the firer 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Collision", meta = (EditCondition = bCanEditTeamCollisionProperties))
	bool bCanHitFriendlies;

	/** 
	 *	Lifetime of projectile before it pseudo destroys itself and goes back to object pool.
	 *	This can be low to give projectile a lifetime similar to disruptor shots in starcraft II.
	 *	It's other purpose is a failsafe for stray projectiles that somehow never collide with
	 *	anything, possibly they were shot up in the air or cannot collide with world, enemy or
	 *	friendlies. Invisible bounds need to be put in place to stop them and return them to the
	 *	pool 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration", meta = (ClampMin = "0.001"))
	float Lifetime;

	/** 
	 *	If time expires does this projectile cause a hit where it is or just go silently?
	 *	E.g. Starcraft II disruptors would be an example of a true. Area of Effect Radius must
	 *	be greater than 0 for any damage to be dealt in this case. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Duration")
	bool bRegistersHitOnTimeout;

	virtual void SetupForEnteringObjectPool() override;

	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true) override;

	/* Reset collision channels for adding back to pool */
	void ResetTeamCollision(ETeam Team);

	/* Setup collision channels for what causes a hit (not for AoE though, see
	AProjectileBase::SetupAoECollisionChannels) */
	virtual void SetupCollisionChannels(ETeam Team);

	/* Returns the rotation to set on the projectile when it is fired at an actor */
	virtual FRotator GetStartingRotation(const FVector & StartLoc, const AActor * InTarget, float RollRotation) const;
	/* Returns the rotation to set on the projectile when it is fired at a world location */
	virtual FRotator GetStartingRotation(const FVector & StartLoc, const FVector & TargetLoc, float RollRotation) const;

	/* Call when projectile has not hit anything and times out */
	virtual void OnLifetimeExpired();

	/* A hit result to pass into some functions to avoid creating temporaries */
	FHitResult HitResult;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditTeamCollisionProperties;
#endif

private:

	void Delay(FTimerHandle & TimerHandle, void(ACollidingProjectileBase::* Function)(), float Delay);

public:

	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation) override;

	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

	virtual void FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
		float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

#if !UE_BUILD_SHIPPING
	virtual bool IsFitForEnteringObjectPool() const override;

	bool IsSphereCollisionAcceptableForEnteringObjectPool() const;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
