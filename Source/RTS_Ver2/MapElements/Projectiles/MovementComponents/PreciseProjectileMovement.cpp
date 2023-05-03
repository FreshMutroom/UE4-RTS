// Fill out your copyright notice in the Description page of Project Settings.

#include "PreciseProjectileMovement.h"
#include "Curves/CurveVector.h"
#include "GameFramework/WorldSettings.h"


void UPreciseProjectileMovement::BeginPlay()
{
	Super::BeginPlay();

	if (MovementMode == EPreciseProjectileMovementMode::CurveAssets)
	{
		UE_CLOG(PositionCurve == nullptr, RTSLOG, Fatal, TEXT("\"PositionCurve\" is null on precise movement "
			"component belonging to [%s] "), *GetOwner()->GetClass()->GetName());
		UE_CLOG(RotationCurve == nullptr, RTSLOG, Fatal, TEXT("\"RotationCurve\" is null on precise movement "
			"component belonging to [%s] "), *GetOwner()->GetClass()->GetName());
	}
}

void UPreciseProjectileMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{	
	TimeSpentAlive += DeltaTime;

	if (bTickMostlyPaused)
	{
		return;
	}

	// My god all these checks add up. Are they all really required?

	// skip if don't want component updated when not rendered or updated component can't move
	if (HasStoppedSimulation() || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(UpdatedComponent) == false)
	{
		return;
	}

	AActor * ActorOwner = UpdatedComponent->GetOwner();
	if (ActorOwner == nullptr || !CheckStillInWorld())
	{
		return;
	}

	FHitResult Hit(1.f); 
	
	// Move updated component
	if (MovementMode == EPreciseProjectileMovementMode::CurveAssets)
	{
		const FVector NewLocation = ReferenceLocation + PositionCurve->GetVectorValue(TimeSpentAlive);
		const FVector V = RotationCurve->GetVectorValue(TimeSpentAlive);
		const FRotator NewRotation = ReferenceRotation + FRotator(V.Y, V.Z, V.X);
		/* Compute delta between where we are now and where we want to be because that is what
		MoveUpdatedComponent takes. Kind of backwards though because I just want to set the location/rotation
		to the new location/rotation. I could do it - and just do like UpdatedComponent->SetComponentLocation
		or something but i'm not 100% sure whether overlaps will trigger if I do that so I'm just doing
		it the movement component way */
		const FVector MoveDelta = NewLocation - UpdatedComponent->GetComponentLocation();
		MoveUpdatedComponent(MoveDelta, NewRotation, true, &Hit);
	}
	else // Assumed StraightLine
	{
		assert(MovementMode == EPreciseProjectileMovementMode::StraightLine);

		const FVector MoveDelta = Velocity * DeltaTime;
		MoveUpdatedComponent(MoveDelta, UpdatedComponent->GetComponentRotation(), true, &Hit);
	}

	// If we hit a trigger that destroyed us, abort.
	if (ActorOwner->IsPendingKill() || HasStoppedSimulation())
	{
		return;
	}

	if (Hit.bBlockingHit)
	{
		// We don't support sliding or bouncing so we say we have hit something and that's that
		HandleBlockingHit(Hit);
	}
}

bool UPreciseProjectileMovement::HasStoppedSimulation() const
{
	return (UpdatedComponent == nullptr) || (bIsActive == false);
}

bool UPreciseProjectileMovement::CheckStillInWorld()
{
	if (UpdatedComponent == nullptr)
	{
		return false;
	}
	
	const UWorld * MyWorld = GetWorld();
	if (MyWorld == nullptr)
	{
		return false;
	}

	// check the variations of KillZ
	AWorldSettings * WorldSettings = MyWorld->GetWorldSettings(true);
	if (!WorldSettings->bEnableWorldBoundsChecks)
	{
		return true;
	}
	AActor * ActorOwner = UpdatedComponent->GetOwner();
	if (IsValid(ActorOwner) == false)
	{
		return false;
	}
	if (ActorOwner->GetActorLocation().Z < WorldSettings->KillZ)
	{
		UDamageType const * DmgType = WorldSettings->KillZDamageType ? WorldSettings->KillZDamageType->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
		ActorOwner->FellOutOfWorld(*DmgType);
		return false;
	}
	// Check if box has poked outside the world
	else if (UpdatedComponent != nullptr && UpdatedComponent->IsRegistered())
	{
		const FBox&	Box = UpdatedComponent->Bounds.GetBox();
		if (Box.Min.X < -HALF_WORLD_MAX || Box.Max.X > HALF_WORLD_MAX ||
			Box.Min.Y < -HALF_WORLD_MAX || Box.Max.Y > HALF_WORLD_MAX ||
			Box.Min.Z < -HALF_WORLD_MAX || Box.Max.Z > HALF_WORLD_MAX)
		{
			UE_LOG(RTSLOG, Warning, TEXT("%s is outside the world bounds!"), *ActorOwner->GetName());
			ActorOwner->OutsideWorldBounds();
			// not safe to use physics or collision at this point
			ActorOwner->SetActorEnableCollision(false);
			FHitResult Hit(1.f);
			StopSimulating(Hit, EStopSimulatingReason::OutsideWorldBounds);
			return false;
		}
	}
	return true;
}

void UPreciseProjectileMovement::SetMovementMode(EPreciseProjectileMovementMode InMovementMode)
{
	MovementMode = InMovementMode;
}

void UPreciseProjectileMovement::OnProjectileFired(AProjectileBase * InProjectile, USphereComponent * ProjectilesSphereComp)
{
	ReferenceLocation = ProjectilesSphereComp->GetComponentLocation();
	ReferenceRotation = ProjectilesSphereComp->GetComponentRotation();
	TimeSpentAlive = 0.f;
}

void UPreciseProjectileMovement::CurveMode_OnDescentStart(const FVector & DescentReferenceLocation)
{
	assert(MovementMode == EPreciseProjectileMovementMode::CurveAssets);

	ReferenceLocation = DescentReferenceLocation;
}

void UPreciseProjectileMovement::SoftSetTickEnabled(bool bTickEnabled)
{
	bTickMostlyPaused = !bTickEnabled;
}

void UPreciseProjectileMovement::HandleBlockingHit(const FHitResult & Hit)
{
	StopSimulating(Hit, EStopSimulatingReason::HitSomething);
}

void UPreciseProjectileMovement::StopSimulating(const FHitResult & HitResult, EStopSimulatingReason StopReason)
{
	UpdatedComponent = nullptr;
	Velocity = FVector::ZeroVector;
	GetProjectile()->OnProjectileStopFromMovementComp(HitResult);
}

AProjectileBase * UPreciseProjectileMovement::GetProjectile() const
{
	return CastChecked<AProjectileBase>(GetOwner());
}

#if WITH_EDITOR
void UPreciseProjectileMovement::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	//const FName ChangedPropertyName = PropertyChangedEvent.GetPropertyName();
	//if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UPreciseProjectileMovement, PositionCurve))
	//{
	//
	//}
}
#endif
