// Fill out your copyright notice in the Description page of Project Settings.


#include "WarthogMovementComponent.h"

#include "MapElements/CommanderAbilities/Actors/Warthog.h"


UWarthogMovementComponent::UWarthogMovementComponent()
{
	DescentTransitionTime = 1.5f;
	GetawayTransitionTime = 1.f;
}

void UWarthogMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UWarthogMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UWarthogMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FVector OldVelocity = Velocity;
	const FQuat NewRotation = OldVelocity.ToOrientationQuat();

	AWarthog * Warthog = GetOwnerAsWarthog();
	const EWarthogAttackPhase Phase = Warthog->GetAttackPhase();
	if (Phase == EWarthogAttackPhase::Phase1_MovingToTarget)
	{
		// Velocity should be set to a straight line so no need to calculate anything
		MoveUpdatedComponent(Velocity * DeltaTime, NewRotation, false);
	}
	else if (Phase == EWarthogAttackPhase::Phase2_Descending || Phase == EWarthogAttackPhase::Phase3_Firing)
	{
		/* Angle the warthog to face downwards */ 
		const float NewPitch = FMath::InterpEaseIn<float>(0.f, -Warthog->GetPhase2Tilt(), FMath::Min(Warthog->GetTimeSpentInPhase2AndPhase3() / DescentTransitionTime, 1.f), 0.5f);
		const float ZAmount = FMath::Tan(FMath::DegreesToRadians(NewPitch)) * OriginalVelocity.Size();
		Velocity = FVector(OriginalVelocity.X, OriginalVelocity.Y, OriginalVelocity.Z - ZAmount);
		Velocity.Normalize();
		const float Speed = FMath::CubicInterp(Warthog->GetPhase1AndPhase4MoveSpeed(), 0.f, Warthog->GetPhase2AndPhase3MoveSpeed(), 0.f, FMath::Min(Warthog->GetTimeSpentInPhase2AndPhase3() / DescentTransitionTime, 1.f));
		Velocity *= Speed;

		MoveUpdatedComponent(Velocity * DeltaTime, NewRotation, false);
	}
	else
	{
		assert(Phase == EWarthogAttackPhase::Phase4_PostFired);
		
		// Fly away
		/* Start gaining altitude */
		const float NewPitch = FMath::InterpEaseIn<float>(Warthog->GetPhase2Tilt(), Warthog->GetPhase4Tilt(), FMath::Min(Warthog->GetTimeSpentInPhase4() / GetawayTransitionTime, 1.f), 0.7f);
		const float ZAmount = FMath::Tan(FMath::DegreesToRadians(NewPitch)) * OriginalVelocity.Size();
		Velocity = FVector(OriginalVelocity.X, OriginalVelocity.Y, OriginalVelocity.Z + ZAmount);
		Velocity.Normalize();
		const float Speed = FMath::InterpEaseIn<float>(Warthog->GetPhase2AndPhase3MoveSpeed(), Warthog->GetPhase1AndPhase4MoveSpeed(), FMath::Min(Warthog->GetTimeSpentInPhase4() / GetawayTransitionTime, 1.f), 0.5f);
		Velocity *= Speed;

		MoveUpdatedComponent(Velocity * DeltaTime, NewRotation, false);
	}

	UpdateComponentVelocity();
}

AWarthog * UWarthogMovementComponent::GetOwnerAsWarthog() const
{
	return CastChecked<AWarthog>(GetOwner());
}

void UWarthogMovementComponent::OnOwningAbilityUsed(const FVector & TargetLocation)
{
	AWarthog * Warthog = GetOwnerAsWarthog();

	// Set Velocity
	
	Velocity = TargetLocation - Warthog->GetActorLocation();
	// Set to 0 so fly parallel to ground
	Velocity.Z = 0.f;
	Velocity.Normalize();
	Velocity *= Warthog->GetPhase1AndPhase4MoveSpeed();
	OriginalVelocity = Velocity;
}
