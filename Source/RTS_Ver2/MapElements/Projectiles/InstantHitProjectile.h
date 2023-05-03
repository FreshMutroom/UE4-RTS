// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Projectiles/ProjectileBase.h"
#include "InstantHitProjectile.generated.h"

class UParticleSystemComponent;
struct FAttackAttributes;


/**
 *	A projectile without a mesh that will cause damage the moment it is fired. 
 *	Currently best projectile to use for melee type attacks.
 */
UCLASS(Blueprintable)
class RTS_VER2_API AInstantHitProjectile final : public AProjectileBase
{
	GENERATED_BODY()

public:

	AInstantHitProjectile();

protected:

	float GetTraceDistance(float AttackRange) const;

	/* Set the source and target points for TrailParticles */
	void SetTrailParticlesBeamData();

	// TODO get this working with beam particles

	/* Hit result for passing into AProjectileBase::OnHit(). Declared here to avoid recreating
	every time fired */
	FHitResult HitResult;

	/* Location projectile was fired from. Cached for fog calculations */
	FVector StartLocation;

	/* Location of target when projectile was fired. Cached for fog calculations */
	FVector EndLocation;

public:

	virtual void FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
		float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation) override;

	virtual void FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
		float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

	virtual void FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
		float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction,
		AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID) override;

	virtual void GetFogLocations(TArray < FVector, TInlineAllocator<AProjectileBase::NUM_FOG_LOCATIONS> > & OutLocations) const override;
};
