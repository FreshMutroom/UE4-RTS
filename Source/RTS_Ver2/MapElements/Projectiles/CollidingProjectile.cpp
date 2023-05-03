// Fill out your copyright notice in the Description page of Project Settings.

#include "CollidingProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "MapElements/Projectiles/MovementComponents/RTSProjectileMovement.h"
#include "Statics/Statics.h"
//#include "Statics/Structs_2.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/DevelopmentStatics.h"


ACollidingProjectile::ACollidingProjectile()
{
	MoveComp = CreateDefaultSubobject<URTSProjectileMovement>(TEXT("Movement Comp"));
	MoveComp->PrimaryComponentTick.bStartWithTickEnabled = false;
	MoveComp->bRotationFollowsVelocity = true;
	MoveComp->OnProjectileStop.AddDynamic(this, &ACollidingProjectile::OnProjectileStop);
	MoveComp->ProjectileGravityScale = 0.f;	// Straight firing projectile
	MoveComp->InitialSpeed = 1000.f;
}

void ACollidingProjectile::SetupForEnteringObjectPool()
{
	MoveComp->Velocity = FVector::ZeroVector;
	// Seems to be the same
	//MoveComp->SetVelocityInLocalSpace(FVector::ZeroVector);
	MoveComp->SetComponentTickEnabled(false);

	Super::SetupForEnteringObjectPool();
}

void ACollidingProjectile::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	MoveComp->Velocity = FVector::ZeroVector;
	// Seems to be the same
	//MoveComp->SetVelocityInLocalSpace(FVector::ZeroVector);
	MoveComp->SetComponentTickEnabled(false);

	Super::AddToPool(bActuallyAddToPool, bDisableTrailParticles);
}

void ACollidingProjectile::OnProjectileStop(const FHitResult & Hit)
{
	OnHit(Hit);
}

void ACollidingProjectile::Delay(FTimerHandle & TimerHandle, void(ACollidingProjectile::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

void ACollidingProjectile::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	Super::FireAtTarget(Firer, AttackAttributes, AttackRange, Team, MuzzleLoc, ProjectileTarget, RollRotation);

	MoveComp->UpdatedComponent = SphereComp;

	if (MoveComp->ProjectileGravityScale != 0.f)
	{
		/* Arcing projectile */

		TArray <AActor *> IgnoredActors;
		IgnoredActors.Emplace(this);

		const bool bResult = UGameplayStatics::SuggestProjectileVelocity(this, MoveComp->Velocity,
			MuzzleLoc, ProjectileTarget->GetActorLocation(), MoveComp->InitialSpeed, bUsesHighArc,
			SphereComp->GetScaledSphereRadius(), MoveComp->GetGravityZ() /* 0 */,
			ESuggestProjVelocityTraceOption::DoNotTrace,
			FCollisionResponseParams::DefaultResponseParam, IgnoredActors, true);

#if WITH_EDITOR
		if (!bResult)
		{
			UE_LOG(RTSLOG, Fatal, TEXT("CollidingProjectile::Start() did not find arc to target "
				"for arcing type projectile"));
		}
#endif
	}
	else
	{
		/* Straight firing projectile */

		const FVector InitialVelocity = (ProjectileTarget->GetActorLocation() - MuzzleLoc).GetSafeNormal() * MoveComp->InitialSpeed;

		MoveComp->Velocity = InitialVelocity;
	}

	MoveComp->SetComponentTickEnabled(true);
}

void ACollidingProjectile::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	Super::FireAtLocation(Firer, AttackAttributes, AttackRange, Team, StartLoc, TargetLoc, RollRotation, ListeningAbility, ListeningAbilityUniqueID);

	MoveComp->UpdatedComponent = SphereComp;

	/* For now only straight firing projectiles are supported */
	assert(MoveComp->ProjectileGravityScale == 0.f);

	/* If running into issues on direct projectile fires may want to double check this calc is
	correct */
	const FVector Direction = TargetLoc - StartLoc;
	const FVector InitialVelocity = Direction.GetSafeNormal() * MoveComp->InitialSpeed;

	MoveComp->Velocity = InitialVelocity;

	MoveComp->SetComponentTickEnabled(true);
}

void ACollidingProjectile::FireInDirection(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FRotator & Direction, AAbilityBase * ListeningAbility,
	int32 ListeningAbilityUniqueID)
{
	// Implemented this 28/4/19

	Super::FireInDirection(Firer, AttackAttributes, AttackRange, Team, StartLoc, Direction, ListeningAbility, ListeningAbilityUniqueID);

	MoveComp->UpdatedComponent = SphereComp;

	const FVector InitialVelocity = Direction.Vector() * MoveComp->InitialSpeed;

	MoveComp->Velocity = InitialVelocity;

	MoveComp->SetComponentTickEnabled(true);
}

#if !UE_BUILD_SHIPPING
bool ACollidingProjectile::IsFitForEnteringObjectPool() const
{
	return Super::IsFitForEnteringObjectPool() 
		&& MoveComp->IsComponentTickEnabled() == false;
}
#endif
