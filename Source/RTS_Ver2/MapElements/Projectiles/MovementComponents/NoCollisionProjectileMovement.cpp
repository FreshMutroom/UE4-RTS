// Fill out your copyright notice in the Description page of Project Settings.

#include "NoCollisionProjectileMovement.h"

#include "MapElements/Projectiles/NoCollisionTimedProjectile.h"


UNoCollisionProjectileMovement::UNoCollisionProjectileMovement()
{
	ArcCalculationMethod = EArcingProjectileTrajectoryMethod::ChooseInitialVelocity;
	bUseHighArc = false;
	ArcValue = 0.5f;
	HitDistance = 10.f;
	Lifetime = 40.f;
}

void UNoCollisionProjectileMovement::BeginPlay()
{
	Super::BeginPlay();

	Projectile = CastChecked<ANoCollisionTimedProjectile>(GetOwner());

	/* Check that if the projectile is affected by gravity that its max speed is not limited */
	UE_CLOG(GetGravityZ() != 0.f && MaxSpeed != 0.f, RTSLOG, Fatal, 
		TEXT("Projectile class [%s] has gravity applied to it but it's max speed is "
		"limited. This does not work well with the arc predicting logic. Please either set its "
		"gravity scale to 0 so it fires straight or set its max speed to 0 meaning unlimited max speed"), 
		*GetClass()->GetName());
}

void UNoCollisionProjectileMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	/* This function has been overridden for one purpose and that is to check if the projectile 
	is close enough to its target for a hit to be considered. It will only do this if 
	- projectile is being fired at location and not a target since targets can move 
	- projectile is affected by gravity i.e. does not fire straight 
	
	It would be cool if I could predict how long it takes an arcing projectile to reach its 
	target. That way we can get rid of this tick function override entirely */

	if (TargetType == ENoCollisionProjectileMode::ArcedFiringAtLocation)
	{
		/* Check if the projectile has gotten close enough to the target location for us to 
		consider it hit it */
		if (IsCloseEnoughToTargetLocation())
		{
			/* This func might want to teleport the projectile exactly onto the target location */
			Projectile->OnFireAtLocationComplete();
		}
	}

	/* The logic above may need to go somewhere in the middle of Super. I'm just lazy and 
	don't want to paste it all here so I'm just calling full Super here */
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UNoCollisionProjectileMovement::IsCloseEnoughToTargetLocation() const
{
	return (TargetLocation - Projectile->GetActorLocation()).SizeSquared() <= FMath::Square(HitDistance);
}

EArcingProjectileTrajectoryMethod UNoCollisionProjectileMovement::GetArcCalculationMethod() const
{
	return ArcCalculationMethod;
}

float UNoCollisionProjectileMovement::GetProjectileLifetime() const
{
	return Lifetime;
}

bool UNoCollisionProjectileMovement::UseHighArc() const
{
	/* bUseHighArc is only relevant if using this method */
	assert(GetArcCalculationMethod() == EArcingProjectileTrajectoryMethod::ChooseInitialVelocity);
	return bUseHighArc;
}

float UNoCollisionProjectileMovement::GetArcValue() const
{
	/* ArcValue only relevant if using this method, but SuggestProjectileVelocity_CustomArc 
	will be used as a failsafe if SuggestProjectileVelocity fails so I have commented this */
	//assert(GetArcCalculationMethod() == EArcingProjectileTrajectoryMethod::ChooseArc);
	return ArcValue;
}

#if WITH_EDITOR
void UNoCollisionProjectileMovement::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditGravityRelatedProperties = (ProjectileGravityScale != 0.f);
	bCanEditInitialVelocityArcCalcProperties = (ProjectileGravityScale != 0.f) && (ArcCalculationMethod == EArcingProjectileTrajectoryMethod::ChooseInitialVelocity);
	bCanEditArcArcCalcProperties = (ProjectileGravityScale != 0.f) && (ArcCalculationMethod == EArcingProjectileTrajectoryMethod::ChooseArc);
}
#endif
