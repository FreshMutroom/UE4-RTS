// Fill out your copyright notice in the Description page of Project Settings.

#include "HomingProjectile.h"
#include "Public/TimerManager.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

//#include "Statics/Structs_2.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/Projectiles/MovementComponents/HomingProjectileMovement.h"
#include "GameFramework/RTSGameState.h"

AHomingProjectile::AHomingProjectile()
{
	MoveComp = CreateDefaultSubobject<UHomingProjectileMovement>(TEXT("Movement Comp"));
	MoveComp->PrimaryComponentTick.bStartWithTickEnabled = false;

	bCanHitDefeatedTargets = true;
	bCanHitWorld = false;
	bCanOnlyHitTarget = true;
#if WITH_EDITORONLY_DATA
	bCanEditTeamCollisionProperties = !bCanOnlyHitTarget;
#endif

	InitialSpeed = 500.f;
}

void AHomingProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetupAccelerationCurve();

	MoveComp->SetInitialValues(this);
}

void AHomingProjectile::SetupForEnteringObjectPool()
{
	MoveComp->Velocity = FVector::ZeroVector;
	MoveComp->SetComponentTickEnabled(false);
	
	Super::SetupForEnteringObjectPool();
}

void AHomingProjectile::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	MoveComp->Velocity = FVector::ZeroVector;
	MoveComp->SetComponentTickEnabled(false);

	Super::AddToPool(bActuallyAddToPool, bDisableTrailParticles);
}

void AHomingProjectile::SetupCollisionChannels(ETeam Team)
{
	if (bCanOnlyHitTarget)
	{
		/* Special case where we ignore all team channels and check distance to target
		to decide if we have hit anything */
	}
	else
	{
		Super::SetupCollisionChannels(Team);
	}
}

#if WITH_EDITOR
void AHomingProjectile::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditTeamCollisionProperties = !bCanOnlyHitTarget;
}
#endif

void AHomingProjectile::SetupAccelerationCurve()
{
	if (AccelerationCurve != nullptr)
	{
		float MinX, MaxX;
		AccelerationCurve->GetTimeRange(MinX, MaxX);

		/* If true then curve either has zero points or is a vertical line and we cannot use it */
		if (MaxX == MinX)
		{
#if WITH_EDITOR
			// TODO: log something like "Curve set but not a usable curve; will not use it"
#endif
			AccelerationCurve = nullptr;
		}
	}
}

void AHomingProjectile::DoNothing()
{
}

void AHomingProjectile::Delay(FTimerHandle & TimerHandle, void(AHomingProjectile::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

void AHomingProjectile::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	Super::FireAtTarget(Firer, AttackAttributes, AttackRange, Team, MuzzleLoc, ProjectileTarget, RollRotation);

	Mesh->SetHiddenInGame(false);
	MoveComp->OnOwningProjectileFired();
	MoveComp->SetComponentTickEnabled(true);
}

void AHomingProjectile::OnHit(const FHitResult & Hit)
{
	Super::OnHit(Hit);
}

AActor * AHomingProjectile::GetTarget() const
{
	return Target;
}

const FTimerHandle & AHomingProjectile::GetLifetimeTimerHandle() const
{
	return TimerHandle_Lifetime;
}

float AHomingProjectile::GetInitialSpeed() const
{
	return InitialSpeed;
}

float AHomingProjectile::GetMaxSpeed() const
{
	return MaxSpeed;
}

UCurveFloat * AHomingProjectile::GetAccelerationCurve() const
{
	return AccelerationCurve;
}

bool AHomingProjectile::CanOnlyHitTarget() const
{
	return bCanOnlyHitTarget;
}

USphereComponent * AHomingProjectile::GetSphere() const
{
	return SphereComp;
}

bool AHomingProjectile::RegistersHitOnTimeout() const
{
	return bRegistersHitOnTimeout;
}

bool AHomingProjectile::CanHitDefeatedTargets() const
{
	return bCanHitDefeatedTargets;
}

void AHomingProjectile::OnTargetNoLongerValid()
{
	AddToPool();
}

#if !UE_BUILD_SHIPPING
bool AHomingProjectile::IsFitForEnteringObjectPool() const
{
	return Super::IsFitForEnteringObjectPool()
		&& MoveComp->IsComponentTickEnabled() == false;
}
#endif
