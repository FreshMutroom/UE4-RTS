// Fill out your copyright notice in the Description page of Project Settings.

#include "LeaveThenComeBackProjectile.h"
#include "Components/SphereComponent.h"

#include "MapElements/Projectiles/MovementComponents/PreciseProjectileMovement.h"
#include "GameFramework/RTSGameState.h"


ALeaveThenComeBackProjectile::ALeaveThenComeBackProjectile()
{
	MoveComp = CreateDefaultSubobject<UPreciseProjectileMovement>(TEXT("Movement Comp"));
	MoveComp->PrimaryComponentTick.bStartWithTickEnabled = false;

	/* Make this large by default. These projectiles are good to use as nukes and we don't want it 
	disappearing suddenly */
	Lifetime = 40.f;  
	
	AscentDuration = 2.5f;
	DescentDelay = 1.f;
	MovementMode = EPreciseProjectileMovementMode::StraightLine;
	AscentSpeed = 2000.f;
	AscentRelativeAngle = FRotator(90.f, 0.f, 0.f);
	DescentStartingHeight = 5000.f;
	DescentSpeed = 2500.f;
	DescentRelativeAngle = FRotator(-90.f, 0.f, 0.f);
}

void ALeaveThenComeBackProjectile::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	if (MovementMode == EPreciseProjectileMovementMode::StraightLine)
	{
		/* Check if the projectile 
		- starts above target loc but is facing upwards OR 
		- starts below target loc and faces downwards */
		if ((DescentStartingHeight > 0.f && DescentStartingRotation.Pitch < 0.f) || (DescentStartingHeight < 0.f && DescentStartingRotation.Pitch > 0.f))
		{
			UE_LOG(RTSLOG, Fatal, TEXT("LeaveThenComeBackProjectile [%s] is set up in such a way that it can "
				"never hit the target location. Change the sign on either DescentStartingHeight or "
				"DescentStartRotation.Pitch"), *GetClass()->GetName());
		}
	}
#endif	
}

void ALeaveThenComeBackProjectile::FireAtLocation(AActor * Firer, const FBasicDamageInfo & AttackAttributes,
	float AttackRange, ETeam Team, const FVector & StartLoc, const FVector & TargetLoc, float RollRotation,
	AAbilityBase * ListeningAbility, int32 ListeningAbilityUniqueID)
{
	Super::FireAtLocation(Firer, AttackAttributes, AttackRange, Team, StartLoc, TargetLoc, RollRotation, ListeningAbility, ListeningAbilityUniqueID);

	if (MovementMode == EPreciseProjectileMovementMode::StraightLine)
	{
		// Calculate these now. We will use them for descent later
		DescentStartingRotation = GetDescentStartRotation(StartLoc, TargetLoc);
		DescentStartingLocation = GetDescentStartLocation(StartLoc, TargetLoc, DescentStartingRotation);

		MoveComp->Velocity = GetInitialVelocity(StartLoc, TargetLoc);
	}
	else // Assumed CurveAssets
	{
		/* Store TargetLoc in this variable. Will tell move comp about it later */
		DescentStartingLocation = TargetLoc;
	}

	MoveComp->UpdatedComponent = SphereComp;
	MoveComp->OnProjectileFired(this, SphereComp);
	MoveComp->SetComponentTickEnabled(true);

	FTimerHandle TimerHandle_EndAscent;
	Delay(TimerHandle_EndAscent, &ALeaveThenComeBackProjectile::OnAscentCompleted, AscentDuration);
}

void ALeaveThenComeBackProjectile::SetupCollisionChannels(ETeam Team)
{
	/* Set no collision. This is called during ACollidingProjectileBase::FireAtLocation and we do 
	not want collision for the ascent */
}

FRotator ALeaveThenComeBackProjectile::GetStartingRotation(const FVector & StartLoc, const FVector & TargetLoc, float RollRotation) const
{
	if (MovementMode == EPreciseProjectileMovementMode::StraightLine)
	{
		//-----------------------------------------------------------------------
		// May be able to calculate this slightly faster by something like:
		//FVector FacingVector = (TargetLocation - StartLocation);
		//FacingVector.Z = 0.f;
		//FacingVector.Normalize();
		//return AscentRelativeAngle.RotateVector(FacingVector) * AscentSpeed;
		// Actually above code does not work
		//-----------------------------------------------------------------------
		
		FVector FacingVector = (TargetLoc - StartLoc);
		FRotator Rotation = FacingVector.Rotation();
		Rotation.Pitch = AscentRelativeAngle.Pitch;
		Rotation += FRotator(0.f, AscentRelativeAngle.Yaw, AscentRelativeAngle.Roll);

		return Rotation;//Rotation.Vector();
	}
	else
	{
		return Super::GetStartingRotation(StartLoc, TargetLoc, RollRotation);
	}
}

FVector ALeaveThenComeBackProjectile::GetInitialVelocity(const FVector & StartLocation, const FVector & TargetLocation) const
{
	/* We're already facing where we want to go - GetStartingRotation did that. Just multiply 
	by speed */
	return GetActorForwardVector() * AscentSpeed;
}

FRotator ALeaveThenComeBackProjectile::GetDescentStartRotation(const FVector & StartLocation, const FVector & TargetLocation) const
{
	// The default facing is from StartLocation to TargetLocation. Then we need to apply DescentRelativeAngle
	// If our actor's rotation is already facing the TargetLocation then we do not need to do 
	// the first step 

	// Rotation that goes from start location to target location 
	const FRotator FacingRot = (TargetLocation - StartLocation).Rotation();

	// Pitch is exactly whatever user wants it to be
	return FRotator(DescentRelativeAngle.Pitch, FacingRot.Yaw + DescentRelativeAngle.Yaw, GetActorRotation().Roll + DescentRelativeAngle.Roll);
}

FVector ALeaveThenComeBackProjectile::GetDescentStartLocation(const FVector & StartLocation, const FVector & TargetLocation, const FRotator & DescentStartRotation) const
{
	FVector V = DescentStartRotation.Vector();
	const float Hypotenuse = DescentStartingHeight / FMath::Sin(FMath::DegreesToRadians(-DescentStartRotation.Pitch));
	V *= -Hypotenuse;

	return TargetLocation + V;
}

void ALeaveThenComeBackProjectile::OnAscentCompleted()
{
	if (DescentDelay > 0.f)
	{
		// Hide projectile. Should we even do this? It's kind of expected that it's already out of 
		// camera view anyway
		SetProjectileVisibility(false);

		/* We will not stop the projectile from moving now. Users can create a drop to 0 in their 
		float curve for the move comp to stop projectile if they wish. The only reason you 
		would want to stop it is if it is going so fast there is a risk it will go above the world
		Z limit (if one exists) between now and when BeginDescent() is called */

		// Currently I do not think there is a 'travel sound' for projectiles, but if one is ever 
		// added we might want to stop it now and resume it when descent begins

		FTimerHandle TimerHandle_BeginDescent;
		Delay(TimerHandle_BeginDescent, &ALeaveThenComeBackProjectile::BeginDescent, DescentDelay);

		// Stops move comp's tick from doing anything. Could just do SetComponentTickEnabled(false) 
		// but I dunno, toggling ticks could be a slightly heavy operation. It's only going to 
		// be off for DescentDelay amount of time so will just do this
		MoveComp->SoftSetTickEnabled(false);
	}
	else
	{
		BeginDescent();
	}
}

void ALeaveThenComeBackProjectile::SetProjectileVisibility(bool bVisibility)
{
	// Does not work! Odd. Not overridden by any parent classes from what I can tell
	//SetActorHiddenInGame(true);

	Mesh->SetVisibility(bVisibility, false);
	// Do nothing with trail particles. I guess trail particles can stay visible
}

void ALeaveThenComeBackProjectile::BeginDescent()
{
	if (MovementMode == EPreciseProjectileMovementMode::StraightLine)
	{
		// Teleport to desired location
		SetActorLocationAndRotation(DescentStartingLocation, DescentStartingRotation);
		
		MoveComp->Velocity = DescentStartingRotation.Vector() * DescentSpeed;
	}
	else // Assumed CurveAssets
	{
		assert(MovementMode == EPreciseProjectileMovementMode::CurveAssets);

		MoveComp->CurveMode_OnDescentStart(DescentStartingLocation);
	}

	// Enable collision because we did not have collision enabled for the ascent
	SetupCollisionForDescent(OwningTeam);

	// Re-enable most tick logic running
	MoveComp->SoftSetTickEnabled(true);

	SetProjectileVisibility(true);
}

void ALeaveThenComeBackProjectile::SetupCollisionForDescent(ETeam Team)
{
	if (bCanHitEnemies)
	{
		for (const auto & Elem : GS->GetEnemyChannels(Team))
		{
			SphereComp->SetCollisionResponseToChannel(static_cast<ECollisionChannel>(Elem), ECR_Block);
		}
	}
	if (bCanHitFriendlies)
	{
		const ECollisionChannel Channel = GS->GetTeamCollisionChannel(Team);
		SphereComp->SetCollisionResponseToChannel(Channel, ECR_Block);
	}

	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

// Assuming this isn't called anytime in between the switch from ascending to descending
void ALeaveThenComeBackProjectile::OnProjectileStop(const FHitResult & Hit)
{
	OnHit(Hit);
}

void ALeaveThenComeBackProjectile::OnProjectileStopFromMovementComp(const FHitResult & Hit)
{
	OnProjectileStop(Hit);
}

void ALeaveThenComeBackProjectile::SetupForEnteringObjectPool()
{
	MoveComp->Velocity = FVector::ZeroVector;
	MoveComp->SetComponentTickEnabled(false);
	
	Super::SetupForEnteringObjectPool();
}

void ALeaveThenComeBackProjectile::AddToPool(bool bActuallyAddToPool, bool bDisableTrailParticles)
{
	MoveComp->Velocity = FVector::ZeroVector;
	MoveComp->SetComponentTickEnabled(false);

	Super::AddToPool(bActuallyAddToPool, bDisableTrailParticles);
}

#if !UE_BUILD_SHIPPING
bool ALeaveThenComeBackProjectile::IsFitForEnteringObjectPool() const
{
	return Super::IsFitForEnteringObjectPool()
		&& MoveComp->IsComponentTickEnabled() == false;
}
#endif

void ALeaveThenComeBackProjectile::Delay(FTimerHandle & TimerHandle, void(ALeaveThenComeBackProjectile::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	TimerManager->SetTimer(TimerHandle, this, Function, Delay, false);
}

#if WITH_EDITOR
void ALeaveThenComeBackProjectile::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditStraightLineMoveModeProperties = (MovementMode == EPreciseProjectileMovementMode::StraightLine);
	
	MoveComp->SetMovementMode(MovementMode);
}
#endif
