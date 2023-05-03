// Fill out your copyright notice in the Description page of Project Settings.


#include "PitchChangeBuildingAttackComp_SM.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"


UPitchChangeBuildingAttackComp_SM::UPitchChangeBuildingAttackComp_SM()
{
	bReplicates = true;

	YawFacingRequirement = 10.f;
	PitchRotationRate = 20.f;
	PitchFacingRequirement = 5.f;
	MinAllowedPitch = MIN_TURRET_PITCH;
	MaxAllowedPitch = MAX_TURRET_PITCH;

#if WITH_EDITOR
	RunPostEditLogic();
#endif
}

void UPitchChangeBuildingAttackComp_SM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	// Currently only the server needs to tick. This could change if I add interpolation for pitch replication
	SERVER_CHECK;

	UStaticMeshComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RotatePitchToFaceTarget(DeltaTime);

	// Try attack target.
	if (HasAttackWarmup())
	{
		// Check if attack is ready
		if (HasAttackFullyWarmedUp())
		{
			const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
			if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
			{
				ServerDoAttack();
			}
			else
			{
				SetTarget(nullptr);
			}
		}
		else if (IsAttackWarmingUp())
		{
			TimeSpentWarmingUpAttack += DeltaTime;
		}
		else
		{
			TimeRemainingTillAttackOffCooldown -= DeltaTime;
			TimeRemainingTillAttackOffCooldown = FMath::Max(0.f, TimeRemainingTillAttackOffCooldown);
			if (TimeRemainingTillAttackOffCooldown <= 0.f)
			{
				const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
				if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
				{
					StartAttackWarmUp();
				}
				else if (TargetingStatus == ETargetTargetingCheckResult::Unaquire)
				{
					SetTarget(nullptr);
				}
			}
		}
	}
	else
	{
		TimeRemainingTillAttackOffCooldown -= DeltaTime;
		if (TimeRemainingTillAttackOffCooldown <= 0.f)
		{
			const ETargetTargetingCheckResult TargetingStatus = GetCurrentTargetTargetingStatus();
			if (TargetingStatus == ETargetTargetingCheckResult::CanAttack)
			{
				ServerDoAttack();
			}
			else if (TargetingStatus == ETargetTargetingCheckResult::Unaquire)
			{
				SetTarget(nullptr);
				TimeRemainingTillAttackOffCooldown = 0.f;
			}
			else
			{
				TimeRemainingTillAttackOffCooldown = 0.f;
			}
		}
	}
}

void UPitchChangeBuildingAttackComp_SM::RotatePitchToFaceTarget(float DeltaTime)
{
	SERVER_CHECK;

	/* Possibly not strong enough of a check. May need to do if (Statics::IsValid(Target.Get())) */
	if (Target.IsValid())
	{
		const float LookAtPitch = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Target->GetActorLocation()).Pitch;
		const float OurCurrentPitch = GetEffectiveComponentPitchRotation();
		const float AbsDifference = (FMath::Abs(OurCurrentPitch) > LookAtPitch) ? FMath::Abs(OurCurrentPitch) - LookAtPitch : LookAtPitch - FMath::Abs(OurCurrentPitch);
		/* Only bother rotating if it's a significant amount */
		if (AbsDifference > 0.0001f)
		{
			// Clamp so as to not overshoot rotation towards target
			float PitchAmountToMoveThisFrame = FMath::Min(AbsDifference, PitchRotationRate * DeltaTime);
			PitchAmountToMoveThisFrame = (LookAtPitch > OurCurrentPitch) ? PitchAmountToMoveThisFrame : PitchAmountToMoveThisFrame * -1.f;

			if (OwningBuilding->IsInsideFogOfWar() == false)
			{
				// FQaut calculations given yaw and roll are 0 
				const float DEG_TO_RAD = PI / 180.f;
				const float RADS_DIVIDED_BY_2 = DEG_TO_RAD / 2.f;
				float SP, CP;
				// No Fmod cause I already know it's in the [-180, 180) range
				const float PitchNoWinding = PitchAmountToMoveThisFrame;//FMath::Fmod(PitchAmountToMoveThisFrame, 360.0f);
				FMath::SinCos(&SP, &CP, PitchNoWinding * RADS_DIVIDED_BY_2);
				const FQuat DeltaRotation = FQuat(0.f, -SP, 0.f, CP);

				AddRelativeRotation(DeltaRotation, false);

				RelativePitchRotation = RelativeRotation.Pitch;
			}
			else
			{
				/* The building is not visible to the local player. We shouldn't update it's 
				rotation. Instead we calculate what it's rotation would be and set the replicated 
				variable to it so other human players can know it via replicated variable */
				
				/* I'm not doing all the complicated stuff AddRelativeRotation would do, I'm just 
				incrementing this. I hope it's good enough and there's no gimbal lock or 
				any other issues */
				RelativePitchRotation += PitchAmountToMoveThisFrame;
			}
		}
	}
}

UBuildingAttackComponent_SM::ETargetTargetingCheckResult UPitchChangeBuildingAttackComp_SM::GetCurrentTargetTargetingStatus() const
{
	AActor * Tar = Target.Get();
	if (Statics::IsValid(Tar) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	if (Statics::HasZeroHealth(Tar) == true)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	// Perhaps target's gained flying now so check
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Tar))
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	if (IsActorInRange_UseLeneince(Tar) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}
	// Check angle
	float OurPitch;
	FRotator LookAtRot;
	if (YawFacingRequirement < 180.f)
	{
		// Check yaw
		if (GetYawToActor(Tar, LookAtRot) > YawFacingRequirement)
		{
			return HasRotatingBase() ? ETargetTargetingCheckResult::CannotAttack : ETargetTargetingCheckResult::Unaquire;
		}

		// Check pitch
		OurPitch = GetEffectiveComponentPitchRotation();
		const ETargetTargetingCheckResult PitchCheckResult = GetPitchTargetingStatus(OurPitch, LookAtRot.Pitch);
		if (PitchCheckResult != ETargetTargetingCheckResult::CanAttack)
		{
			return PitchCheckResult;
		}
	}
	else
	{
		// Check pitch
		OurPitch = GetEffectiveComponentPitchRotation();
		LookAtRot = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Tar->GetActorLocation());
		const ETargetTargetingCheckResult PitchCheckResult = GetPitchTargetingStatus(OurPitch, LookAtRot.Pitch);
		if (PitchCheckResult != ETargetTargetingCheckResult::CanAttack)
		{
			return PitchCheckResult;
		}
	}
	if (Statics::IsSelectableVisible(Tar, TeamVisibilityInfo) == false)
	{
		return ETargetTargetingCheckResult::Unaquire;
	}

	return ETargetTargetingCheckResult::CanAttack;
}

void UPitchChangeBuildingAttackComp_SM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPitchChangeBuildingAttackComp_SM, RelativePitchRotation);
}

void UPitchChangeBuildingAttackComp_SM::PreNetReceive()
{
	// Might not even need this
	UActorComponent::PreNetReceive();

	// Might not even need this either
	bNetUpdateRelativePitchRotation = false;
}

void UPitchChangeBuildingAttackComp_SM::OnRep_RelativePitchRotation()
{
	if (OwningBuilding->IsInsideFogOfWar() == false)
	{
		bNetUpdateRelativePitchRotation = true;
	}
}

void UPitchChangeBuildingAttackComp_SM::PostRepNotifies()
{
	if (bNetUpdateRelativePitchRotation)
	{
		RelativeRotation.Pitch = RelativePitchRotation;

		/* This will most likely make the change instant. I may want to add some interpolation
		but is instant for now */
		UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate);
		bNetUpdateRelativePitchRotation = false;
	}
}

void UPitchChangeBuildingAttackComp_SM::OnParentBuildingExitFogOfWar(bool bOnServer)
{
	/* Update RelativeRotation then actually rotate component */
	RelativeRotation.Pitch = RelativePitchRotation;
	UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate);
}

bool UPitchChangeBuildingAttackComp_SM::CanSweptActorBeAquiredAsTarget(AActor * Actor) const
{
	if (Statics::IsValid(Actor) == false)
	{
		return false;
	}
	if (Statics::HasZeroHealth(Actor) == true)
	{
		return false;
	}
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Actor) == true)
	{
		return false;
	}
	if (Statics::CanTypeBeTargeted(Actor, AttackAttributes.GetAcceptableTargetTypes()) == false)
	{
		return false;
	}
	// Check angle
	if (HasRotatingBase())
	{
		// Check pitch
		if (bNeedsToCheckPitchBeforeTargetAquire)
		{
			const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Actor->GetActorLocation());
			if (CanPitchBeChangedEnoughToFace(LookAtRot.Pitch) == false)
			{
				return false;
			}
		}
	}
	else
	{
		if (bNeedsToCheckPitchBeforeTargetAquire)
		{
			// Check yaw
			FRotator LookAtRot;
			if (YawFacingRequirement < 180.f)
			{
				if (GetYawToActor(Actor, LookAtRot) > YawFacingRequirement)
				{
					return false;
				}

				// Yaw is ok. Now check pitch
				if (CanPitchBeChangedEnoughToFace(LookAtRot.Yaw) == false)
				{
					return false;
				}
			}
			else
			{
				// Check pitch
				LookAtRot = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Actor->GetActorLocation());
				if (CanPitchBeChangedEnoughToFace(LookAtRot.Pitch) == false)
				{
					return false;
				}
			}
		}
		else
		{
			// Check yaw
			if (YawFacingRequirement < 180.f && GetYawToActor(Actor) > YawFacingRequirement)
			{
				return false;
			}
		}
	}
	if (Statics::IsSelectableVisible(Actor, TeamVisibilityInfo) == false)
	{
		return false;
	}

	return true;
}

bool UPitchChangeBuildingAttackComp_SM::CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const
{
	if (Statics::IsValid(Actor) == false)
	{
		return false;
	}
	if (Statics::HasZeroHealth(Actor) == true)
	{
		return false;
	}
	if (AttackAttributes.CanAttackAir() == false && Statics::IsAirUnit(Actor))
	{
		return false;
	}
	if (Statics::CanTypeBeTargeted(Actor, AttackAttributes.GetAcceptableTargetTypes()) == false)
	{
		return false;
	}
	// Check angle
	if (HasRotatingBase() == false && YawFacingRequirement < 180.f)
	{
		FRotator LookAtRot;
		OutRotationRequiredToFace = GetYawToActor(Actor, LookAtRot);
		if (OutRotationRequiredToFace > YawFacingRequirement)
		{
			return false;
		}
	
		if (bNeedsToCheckPitchBeforeTargetAquire)
		{
			if (CanPitchBeChangedEnoughToFace(LookAtRot.Pitch) == false)
			{
				return false;
			}
		}
		// Add on pitch rotation required
		OutRotationRequiredToFace += FMath::Abs(LookAtRot.Pitch);
	}
	else
	{
		const float LookAtPitch = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Actor->GetActorLocation()).Pitch;
		if (bNeedsToCheckPitchBeforeTargetAquire)
		{
			if (CanPitchBeChangedEnoughToFace(LookAtPitch) == false)
			{
				return false;
			}
		}
		OutRotationRequiredToFace = FMath::Abs(LookAtPitch);
	}
	if (Statics::IsSelectableVisible(Actor, TeamVisibilityInfo) == false)
	{
		return false;
	}

	return true;
}

float UPitchChangeBuildingAttackComp_SM::GetEffectiveComponentPitchRotation() const
{
	return RelativePitchRotation;
}

bool UPitchChangeBuildingAttackComp_SM::CanPitchBeChangedEnoughToFace(float PitchWeWantToHave) const
{
	assert(bNeedsToCheckPitchBeforeTargetAquire);
	return (PitchWeWantToHave > (MinAllowedPitch - PitchFacingRequirement))
		&& (PitchWeWantToHave < (MaxAllowedPitch + PitchFacingRequirement));
}

UBuildingAttackComponent_SM::ETargetTargetingCheckResult UPitchChangeBuildingAttackComp_SM::GetPitchTargetingStatus(float OurPitch, float TargetsLookAtPitch) const
{
	/* If this throws 1 reason may be I need to clamp it in Tick for the 
	OwningBuilding->IsInsideFogOfWar() == true case */
	assert(OurPitch <= 90.f && OurPitch >= -90.f);
	assert(TargetsLookAtPitch <= 90.f && TargetsLookAtPitch >= -90.f);

	/* First check if target's look at pitch is achievable */
	if (TargetsLookAtPitch <= MinAllowedPitch - PitchFacingRequirement 
		|| TargetsLookAtPitch >= MaxAllowedPitch + PitchFacingRequirement)
	{
		/* The turret cannot achieve the pitch required to face the target */
		return ETargetTargetingCheckResult::Unaquire;
	}

	/* Check if pitch is good enough to attack the target */
	if (OurPitch + PitchFacingRequirement >= TargetsLookAtPitch
		&& OurPitch - PitchFacingRequirement <= TargetsLookAtPitch)
	{
		return ETargetTargetingCheckResult::CanAttack;
	}
	else
	{
		return ETargetTargetingCheckResult::CannotAttack;
	}
}

#if WITH_EDITOR
void UPitchChangeBuildingAttackComp_SM::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	RunPostEditLogic();
}

void UPitchChangeBuildingAttackComp_SM::RunPostEditLogic()
{
	/* UBuildingAttackComponent_SM::PostEditChangeChainProperty can set this to false, but we 
	want it always true */
	bReplicates = true;
	
	RelativePitchRotation = RelativeRotation.Pitch;

	bNeedsToCheckPitchBeforeTargetAquire =
		(MinAllowedPitch - PitchFacingRequirement >= -90.f) || (MaxAllowedPitch + PitchFacingRequirement <= 90.f);
}
#endif
