// Fill out your copyright notice in the Description page of Project Settings.

#include "InstantHitProjectile.h"

#include "Statics/DevelopmentStatics.h"
#include "Managers/ObjectPoolingManager.h"
#include "Particles/ParticleSystemComponent.h"
#include "Statics/Statics.h"


AInstantHitProjectile::AInstantHitProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(TrailParticles);

	SetActorEnableCollision(false);

	bReplicates = false;
	bReplicateMovement = false;
	bAlwaysRelevant = false;

	ImpactDamage = 60.f;
}

float AInstantHitProjectile::GetTraceDistance(float AttackRange) const
{
	return AttackRange > 0.f ? AttackRange : 10000.f;
}

void AInstantHitProjectile::SetTrailParticlesBeamData()
{
	/* Setting source/target point could possibly be quite expensive so null check at least. 
	Ideally would like a bool like bTrailParticlesIsABeamEmitter. Only problem is that a 
	user could modify their particles in cascade to use a beam but post edit on this class 
	will not trigger for that. It could be assigned in BeginPlay. I think the best solution 
	though is to have a static TMap<UClass*, FSomeIMportantProperties>. It gets populated 
	during setup. Then when this projectile spawns it just takes values from there instead 
	of having to calculate them on its BeginPlay */
	if (TrailParticles->Template != nullptr)
	{
		TrailParticles->SetBeamSourcePoint(0, StartLocation, 0);
		TrailParticles->SetBeamTargetPoint(0, EndLocation, 0);
		
		/* TODO trail particles will only show once. Once the projectile has been added to 
		pool it they won't ever show again. This might be an issue for all projectiles not 
		just instant hit ones */
	}
}

void AInstantHitProjectile::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	Super::FireAtTarget(Firer, AttackAttributes, AttackRange, Team, MuzzleLoc, ProjectileTarget, RollRotation);

	StartLocation = MuzzleLoc;
	EndLocation = ProjectileTarget->GetActorLocation();

	SetTrailParticlesBeamData();

	/* Simulate hit straight away */
	HitResult.PhysMaterial = nullptr;
	HitResult.Actor = ProjectileTarget;
	HitResult.ImpactPoint = ProjectileTarget->GetActorLocation();
	OnHit(HitResult);
}

void AInstantHitProjectile::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	Super::FireAtLocation(Firer, AttackAttributes, AttackRange, Team, StartLoc, TargetLoc, RollRotation, ListeningAbility, ListeningAbilityUniqueID);

	StartLocation = StartLoc;
	EndLocation = TargetLoc;

	SetTrailParticlesBeamData();

	/* Simulate hit straight away */
	HitResult.PhysMaterial = nullptr;
	HitResult.ImpactPoint = TargetLoc;
	OnHit(HitResult);
}

void AInstantHitProjectile::FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction, AAbilityBase * ListeningAbility,
	int32 ListeningAbilityUniqueID)
{
	Super::FireInDirection(Firer, AttackAttributes, AttackRange, Team, StartLoc, Direction, ListeningAbility, ListeningAbilityUniqueID);

	// Line trace to see if hit anything
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = bQueryPhysicalMaterialForHits;
	if (Statics::LineTraceSingleByChannel(GetWorld(), HitResult, StartLoc, StartLoc + (Direction.Vector() * GetTraceDistance(AttackRange)),
		SELECTABLES_AND_GROUND_CHANNEL, QueryParams))
	{
		StartLocation = StartLoc;
		EndLocation = HitResult.ImpactPoint;

		SetTrailParticlesBeamData();

		OnHit(HitResult);
	}
	else
	{
		AddToPool();
	}
}

void AInstantHitProjectile::GetFogLocations(TArray<FVector, TInlineAllocator<AProjectileBase::NUM_FOG_LOCATIONS>>& OutLocations) const
{
	/* Array expected to be empty before calling this */
	assert(OutLocations.Num() == 0);

	OutLocations.Emplace(StartLocation);
	OutLocations.Emplace(EndLocation);
}
