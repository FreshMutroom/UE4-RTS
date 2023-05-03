// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/CollidingProjectileBase.h"
#include "CollidingProjectile.generated.h"

class URTSProjectileMovement;
struct FAttackAttributes;


/**
 *	A standard non-replicating projectile suitable for straight firing or arcing projectiles
 *	such as artillery rounds or arrows
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API ACollidingProjectile final : public ACollidingProjectileBase
{
	GENERATED_BODY()

public:

	ACollidingProjectile();

protected:

	UPROPERTY(EditDefaultsOnly)
	URTSProjectileMovement * MoveComp;

	/* Only relevant if movement components ProjectileGravityScale != 0. This decides
	whether high arc or low arc will be used when launching projectile. High arc is
	suitable for say a long range artillery shell while low arc would be more suited for
	tank rounds */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Movement")
	bool bUsesHighArc;

	virtual void SetupForEnteringObjectPool() override;

	/* Call right before adding to object pool */
	virtual void AddToPool(bool bActuallyAddToPool = true, bool bDisableTrailParticles = true) override;

	UFUNCTION()
	void OnProjectileStop(const FHitResult & Hit);

private:

	void Delay(FTimerHandle & TimerHandle, void(ACollidingProjectile::* Function)(), float Delay);

public:

	// TODO take into account the AttackRange param for the 3 FireAt* funcs. Maybe have the projectile 'time out' after 
	// it travels the AttackRange + maybe a bit extra. Remember that targets could be moving 
	// away. 

	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation) override;

	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

	virtual void FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
		float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

#if !UE_BUILD_SHIPPING
	virtual bool IsFitForEnteringObjectPool() const override;
#endif
};
