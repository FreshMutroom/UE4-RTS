// Fill out your copyright notice in the Description page of Project Settings.

#include "NoCollisionTimedProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Engine/World.h"
#include "Public/TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Statics/DevelopmentStatics.h"
#include "Managers/ObjectPoolingManager.h"
#include "GameFramework/RTSPlayerController.h"
#include "Statics/Statics.h"
#include "MapElements/Projectiles/MovementComponents/NoCollisionProjectileMovement.h"
#include "MapElements/Abilities/AbilityBase.h"


ANoCollisionTimedProjectile::ANoCollisionTimedProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh); // Note: Previously arrow was root
	Mesh->PrimaryComponentTick.bCanEverTick = false;
	Mesh->PrimaryComponentTick.bStartWithTickEnabled = false;
	Mesh->bReceivesDecals = false;
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCollisionProfileName(FName("NoCollision"));

	/* Make the mesh engine sphere by default because this is the default projectile class */
	auto Sphere_SM = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/VREditor/TransformGizmo/SM_Sequencer_Key.SM_Sequencer_Key'"));
	if (Sphere_SM.Object != nullptr)
	{
		Mesh->SetStaticMesh(Sphere_SM.Object);
	}
	
	MoveComp = CreateDefaultSubobject<UNoCollisionProjectileMovement>(TEXT("Movement Component"));
	MoveComp->PrimaryComponentTick.bStartWithTickEnabled = false;
	MoveComp->InitialSpeed = 3500.f;
	MoveComp->bRotationFollowsVelocity = true;
	/* Fires straight */
	MoveComp->ProjectileGravityScale = 0.f;

	TrailParticles->SetupAttachment(Mesh);

	bReplicates = false;
	bReplicateMovement = false;
	bAlwaysRelevant = false;

	SetActorEnableCollision(false);

	LifetimeModifier = 1.f;
	HitSpeedModifier = 1.f;
	bHitFallenTargets = true;

#if WITH_EDITOR
	RunPostEditLogic();
#endif
}

void ANoCollisionTimedProjectile::SetupForEnteringObjectPool()
{
	assert(TimerManager->IsTimerPending(TimerHandle_Hit) == false);

	MakeInactive();

	Super::SetupForEnteringObjectPool();
}

void ANoCollisionTimedProjectile::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	assert(TimerManager->IsTimerPending(TimerHandle_Hit) == false);

	MakeInactive();

	Super::AddToPool(bActuallyAddToPool, bDisableTrailParticles);
}

float ANoCollisionTimedProjectile::CalculateLifetime(float Distance)
{
	return (Distance / MoveComp->InitialSpeed) * LifetimeModifier;
}

void ANoCollisionTimedProjectile::FireAtTarget(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
	float AttackRange, ETeam Team, const FVector & MuzzleLoc, AActor * ProjectileTarget, float RollRotation)
{
	/* For now firing at a target with an arcing projectile is not allowed */
	assert(MoveComp->GetGravityZ() == 0.f);
	
	Super::FireAtTarget(Firer, AttackAttributes, AttackRange, Team, MuzzleLoc, ProjectileTarget, RollRotation);

	Mesh->SetHiddenInGame(false);
	assert(MoveComp->IsComponentTickEnabled() == false);
	MoveComp->SetComponentTickEnabled(true);

	/* Teleport to muzzle location and rotation. Pretty sure setting rotation here is meaningful 
	and isn't overridden later. Could perhaps test it by just doing GetActorLocation */
	const bool bResult = SetActorLocationAndRotation(MuzzleLoc, FRotator(0.f, 0.f, RollRotation));
	assert(bResult);

	/* Figure out when to destroy self based on speed and distance to target */
	const FVector TargetLoc = Target->GetActorLocation();
	const float DistanceToTarget = (Target->GetActorLocation() - MuzzleLoc).Size();
	const float Lifetime = CalculateLifetime(DistanceToTarget);

	MoveComp->TargetType = ENoCollisionProjectileMode::StraightFiringAtTarget;

	/* Make projectile move towards target */
	MoveComp->Velocity = (TargetLoc - MuzzleLoc).GetSafeNormal() * MoveComp->InitialSpeed;

	if (HitSpeedModifier == 1.f)
	{
		Delay(TimerHandle_Lifetime, &ANoCollisionTimedProjectile::OnFireAtTargetComplete, Lifetime);
	}
	else
	{
		Delay(TimerHandle_Lifetime, &ANoCollisionTimedProjectile::OnMeshReachedTarget, Lifetime);

		Delay(TimerHandle_Hit, &ANoCollisionTimedProjectile::OnHitTarget, Lifetime * HitSpeedModifier);
	}
}

void ANoCollisionTimedProjectile::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes, 
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	Super::FireAtLocation(Firer, AttackAttributes, AttackRange, Team, StartLoc, TargetLoc, RollRotation, ListeningAbility, ListeningAbilityUniqueID);

	Mesh->SetHiddenInGame(false);
	assert(MoveComp->IsComponentTickEnabled() == false);
	MoveComp->SetComponentTickEnabled(true);

	/* Teleport to start location. Do we wanna set rotation here too? */
	SetActorLocation(StartLoc);

	/* Set the projectile's initial velocity. If it has no gravity this is easy. If it does 
	have gravity then we will use an engine function to do this. From my testing these work 
	quite well provided you set the UProjectileMovementComponent::MaxSpeed to unlimited (== 0.f) 
	and make sure to pass in the projectile's gravity Z */
	const float GravityZ = MoveComp->GetGravityZ();
	if (GravityZ == 0.f)
	{
		const float DistanceToTarget = (TargetLoc - StartLoc).Size();
		const float Lifetime = CalculateLifetime(DistanceToTarget);
		
		MoveComp->TargetType = ENoCollisionProjectileMode::StraightFiringAtLocation;
		MoveComp->Velocity = (TargetLoc - StartLoc).GetSafeNormal() * MoveComp->InitialSpeed;
		MoveComp->TargetLocation = TargetLoc;

		if (HitSpeedModifier == 1.f)
		{
			Delay(TimerHandle_Lifetime, &ANoCollisionTimedProjectile::OnFireAtLocationComplete, Lifetime);
		}
		else
		{
			Delay(TimerHandle_Lifetime, &ANoCollisionTimedProjectile::OnMeshReachedTargetLocation, Lifetime);

			Delay(TimerHandle_Hit, &ANoCollisionTimedProjectile::OnHitTargetLocation, Lifetime * HitSpeedModifier);
		}
	}
	else
	{
		MoveComp->TargetType = ENoCollisionProjectileMode::ArcedFiringAtLocation;
		MoveComp->TargetLocation = TargetLoc;

		/* Set MoveComp's velocity */
		if (MoveComp->GetArcCalculationMethod() == EArcingProjectileTrajectoryMethod::ChooseInitialVelocity)
		{
			/* This can fail if MoveComp->InitialSpeed is too low */
			bool bSuccess = UGameplayStatics::SuggestProjectileVelocity(this, MoveComp->Velocity, GetActorLocation(), 
				TargetLoc, MoveComp->InitialSpeed, MoveComp->UseHighArc(), 0.f/*Irrelevant when using DoNotTrace*/, 
				GravityZ, ESuggestProjVelocityTraceOption::DoNotTrace);

			if (!bSuccess)
			{
#if WITH_EDITOR
				PoolingManager->NotifyOfSuggestProjectileVelocityFailing(this, GetActorLocation(), TargetLoc);
#endif
				/* As a failsafe we'll do this, otherwise projectile will just fall into the 
				ground. It's not cool when that happens. Ideally the user sets MoveComp->InitialSpeed 
				high enough that this never happens. The distance is a factor though e.g. if the 
				distance is 600 it can pass but at 800 it will fail, so when you set InitialSpeed 
				you have to make sure it works for the longest distance possible that the projectile 
				can be fired */
				bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, MoveComp->Velocity,
					GetActorLocation(), TargetLoc, GravityZ, MoveComp->GetArcValue());
				assert(bSuccess);
			}
		}
		else // Assumed EArcingProjectileMethod::ChooseArc
		{
			assert(MoveComp->GetArcCalculationMethod() == EArcingProjectileTrajectoryMethod::ChooseArc);
			
			bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, MoveComp->Velocity,
				GetActorLocation(), TargetLoc, GravityZ, MoveComp->GetArcValue());
			assert(bSuccess);
		}

		Delay(TimerHandle_Lifetime, &ANoCollisionTimedProjectile::OnTimedOut, MoveComp->GetProjectileLifetime());
	}
}

void ANoCollisionTimedProjectile::OnTimedOut()
{
	if (TrailParticlesExtraTime > 0.f)
	{
		AddToPool(false, false);
		Delay(TimerHandle_TrailParticles, &ANoCollisionTimedProjectile::DisableTrailsAndTryAddToPool,
			TrailParticlesExtraTime);
	}
	else
	{
		AddToPool();
	}
}

void ANoCollisionTimedProjectile::OnFireAtTargetComplete()
{
	assert(HitSpeedModifier == 1.f);
	
	if (RegisterHitDelay > 0.f)
	{
		// Stop move comp from moving
		MoveComp->SetComponentTickEnabled(false);

		Delay(TimerHandle_Hit, &ANoCollisionTimedProjectile::RegisterHitOnTarget, RegisterHitDelay);
	}
	else
	{
		RegisterHitOnTarget();
	}
}

void ANoCollisionTimedProjectile::RegisterHitOnTarget()
{
	if (Statics::IsValid(Target))
	{
		if (Statics::HasZeroHealth(Target) && !bHitFallenTargets)
		{
			if (TrailParticlesExtraTime > 0.f)
			{
				AddToPool(false, false);
				Delay(TimerHandle_TrailParticles, &ANoCollisionTimedProjectile::DisableTrailsAndTryAddToPool,
					TrailParticlesExtraTime);
			}
			else
			{
				AddToPool();
			}

			return;
		}

		HitResult.Actor = Target;
		HitResult.ImpactPoint = Target->GetActorLocation();
		OnHit(HitResult);
	}
	else
	{
		if (TrailParticlesExtraTime > 0.f)
		{
			AddToPool(false, false);
			Delay(TimerHandle_TrailParticles, &ANoCollisionTimedProjectile::DisableTrailsAndTryAddToPool,
				TrailParticlesExtraTime);
		}
		else
		{
			AddToPool();
		}
	}
}

void ANoCollisionTimedProjectile::OnFireAtLocationComplete()
{
	assert(HitSpeedModifier == 1.f);
	
	if (RegisterHitDelay > 0.f)
	{
		/* This is something I want specifically for my project. We notify the listen ability now because 
		this is what I want for artillery beacon behavior. But this isn't a generalized solution. 
		I need to add some kind of variable like TimeProjectileSticksAroundAfterReachingDestination. 
		The projectile should register a hit the moment it reaches it's destination, then 
		stay still there for TimeProjectileSticksAroundAfterReachingDestination, then add itself 
		to pool then. 
		Also for consistency I should put this same if block into OnFireAtTargetComplete */
		if (ListeningForOnHit != nullptr)
		{
			ListeningForOnHit->OnListenedProjectileHit(ListeningForOnHitData);
			// Null it so OnListenedProjectileHit isn't called again by AProjectileBase::OnHit
			ListeningForOnHit = nullptr;
		}

		const FVector TargetLoc = MoveComp->TargetLocation;
		
		/* Teleport the projectile to the exact location it was fired at. This is optional.
		If it looks bad then leave it out. */
		SetActorLocation(TargetLoc);

		// Stop move comp from moving
		MoveComp->SetComponentTickEnabled(false);
		
		Delay(TimerHandle_Hit, &ANoCollisionTimedProjectile::RegisterHitAtTargetLocation, RegisterHitDelay);
	}
	else
	{
		RegisterHitAtTargetLocation();
	}
}

void ANoCollisionTimedProjectile::RegisterHitAtTargetLocation()
{
	const FVector TargetLoc = MoveComp->TargetLocation;
	
	HitResult.Actor = nullptr;
	HitResult.ImpactPoint = TargetLoc;
	OnHit(HitResult);
}

void ANoCollisionTimedProjectile::OnMeshReachedTarget()
{
	MakeInactive();

	if (TrailParticlesExtraTime > 0.f)
	{
		Delay(TimerHandle_TrailParticles, &ANoCollisionTimedProjectile::HideTrails, TrailParticlesExtraTime);
	}
	else
	{
		HideTrails();
	}
}

void ANoCollisionTimedProjectile::OnHitTarget()
{
	if (Statics::IsValid(Target))
	{
		if (Statics::HasZeroHealth(Target) && !bHitFallenTargets)
		{
			/* Do not do damage or spawn particles etc */

			if (TimerManager->IsTimerActive(TimerHandle_Lifetime) == false
				&& TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false)
			{
				Super::AddToPool();
			}
		}
		else
		{
			HitResult.Actor = Target;
			HitResult.ImpactPoint = Target->GetActorLocation();
			/* ImpactNormal is used for rotation in OnHit. Kind of silly here though because 
			we really just want to pass GetActorRotation() or some variant of it. Instead 
			we pass in a vector which gets converted to a rotator eventually. GetActorForwardVector 
			isn't even really a good value to use for impact normal */
			HitResult.ImpactNormal = -GetActorForwardVector();

			OnHit(HitResult);
		}
	}
	else
	{
		/* If lifetime handle and trail handle aren't running then can add back to pool.
		Might want to consider TimerHandle_DealDamage too */
		if (TimerManager->IsTimerActive(TimerHandle_Lifetime) == false
			&& TimerManager->IsTimerActive(TimerHandle_TrailParticles) == false)
		{
			Super::AddToPool();
		}
	}
}

void ANoCollisionTimedProjectile::OnMeshReachedTargetLocation()
{
	MakeInactive();

	if (TrailParticlesExtraTime > 0.f)
	{
		Delay(TimerHandle_TrailParticles, &ANoCollisionTimedProjectile::HideTrails, TrailParticlesExtraTime);
	}
	else
	{
		HideTrails();
	}
}

void ANoCollisionTimedProjectile::OnHitTargetLocation()
{
	HitResult.Actor = nullptr;
	HitResult.ImpactPoint = MoveComp->TargetLocation;
	HitResult.ImpactNormal = -GetActorForwardVector(); // Probably not a good value to use

	OnHit(HitResult);
}

void ANoCollisionTimedProjectile::MakeInactive()
{
	Mesh->SetHiddenInGame(true);
	
	/* It looks to me like SetComponentTickEnabled actually does quite a bit if ticking isn't 
	being toggled (I could be wrong about that though) and I sometimes disable tick before 
	reaching here so adding this if for performance reasons */
	if (MoveComp->IsComponentTickEnabled() == true)
	{
		MoveComp->SetComponentTickEnabled(false);
	}
	
	MoveComp->SetVelocityInLocalSpace(FVector::ZeroVector);
}

void ANoCollisionTimedProjectile::HideTrails()
{
	TrailParticles->SetComponentTickEnabled(false);
	TrailParticles->SetHiddenInGame(true);

	/* If hit has also been registered then can add back to pool. Super is enough */
	if (TimerManager->IsTimerActive(TimerHandle_Hit) == false)
	{
		Super::AddToPool(true, false);
	}
}

void ANoCollisionTimedProjectile::Delay(FTimerHandle & TimerHandle, void(ANoCollisionTimedProjectile::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

#if !UE_BUILD_SHIPPING
bool ANoCollisionTimedProjectile::IsFitForEnteringObjectPool() const
{
	return Super::IsFitForEnteringObjectPool()
		&& Mesh->IsComponentTickEnabled() == false
		&& Mesh->bHiddenInGame == true
		&& Mesh->IsCollisionEnabled() == false
		&& MoveComp->IsComponentTickEnabled() == false;
}

bool ANoCollisionTimedProjectile::AreAllTimerHandlesCleared() const
{
	return Super::AreAllTimerHandlesCleared() 
		&& TimerManager->IsTimerPending(TimerHandle_Hit) == false;
}
#endif

#if WITH_EDITOR
void ANoCollisionTimedProjectile::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	RunPostEditLogic();
}

void ANoCollisionTimedProjectile::RunPostEditLogic()
{
	bCanEditRegisterHitDelay = (HitSpeedModifier == 1.f);
}
#endif
