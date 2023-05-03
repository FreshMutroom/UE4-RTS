// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingAttackCompStructBase_SK.h"


UBuildingAttackCompStructBase_SK::UBuildingAttackCompStructBase_SK()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	bReceivesDecals = false;
	SetCollisionProfileName(FName("BuildingMesh"));
	/* Just doing these cause this isn't animated and might save on performance */
	bNoSkeletonUpdate = true;
	bPauseAnims = true;

	bReplicates = true;

	RotationRate = 50.f;
}

void UBuildingAttackCompStructBase_SK::SetupComp(ABuilding * InOwningBuilding, UActorComponent * Turret)
{
	OwningBuilding = InOwningBuilding;
	TurretComp = CastChecked<IBuildingAttackComp_Turret>(Turret);

	/* We want this comp to tick after the turret because the turret can set it's target on tick.
	However I have commented this out because I actually get a cycle warning when doing it.
	Oh well I guess we'll tick before the turret then */
	//AddTickPrerequisiteComponent(Turret);
}

float UBuildingAttackCompStructBase_SK::GetEffectiveComponentYawRotation() const
{
	return RelativeYawRotation;
}

void UBuildingAttackCompStructBase_SK::OnParentBuildingExitFogOfWar()
{
	/* Update RelativeRotation then actually update rotation. We want this to also update the
	rotation of the turret component that is a child */
	RelativeRotation.Yaw = RelativeYawRotation;
	UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate);
}

void UBuildingAttackCompStructBase_SK::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	/* Not doing USkeletalMeshComponent because this class does not use any animations and 
	we want to save on performance */
	UMeshComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetWorld()->IsServer())
	{
		/* It is assumed the turret comp will null Target for us if they're no longer attackable
		e.g. out of range, in fog, etc */
		AActor * Target = TurretComp->GetTarget();
		if (Target != nullptr)
		{
			/* Turn to face Target */
			const float LookAtYaw = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), Target->GetActorLocation()).Yaw;
			const float OurCurrentYawRotation = GetEffectiveComponentYawRotation();
			float YawDeltaThisFrame;
			/* Choose to rotate in the direction that will get us facing the target the fastest */
			const float Difference = OurCurrentYawRotation - LookAtYaw;
			if (Difference >= 180.f || (Difference < 0.f && Difference >= -180.f))
			{
				// Go clockwise
				const float YawRotationAmountRequiredToFaceTarget = (Difference >= 0.f) ? 360.f - Difference : -Difference;

				// Use minimum i.e. do not overshoot rotation towards target
				YawDeltaThisFrame = FMath::Min<float>(YawRotationAmountRequiredToFaceTarget, RotationRate * DeltaTime);
			}
			else
			{
				// Go counterclockwise
				const float YawRotationAmountRequiredToFaceTarget = (Difference >= 0.f) ? Difference : (360.f - Difference);

				// Use minimum i.e. do not overshoot rotation towards target
				YawDeltaThisFrame = FMath::Max<float>(-YawRotationAmountRequiredToFaceTarget, -RotationRate * DeltaTime);
			}

			/* Only rotate if it is a significant amount */
			if (YawDeltaThisFrame > 0.0001f || YawDeltaThisFrame < -0.0001f)
			{
				if (OwningBuilding->IsInsideFogOfWar() == false)
				{
					// Calculations for FQuat
					const float DEG_TO_RAD = PI / 180.f;
					const float RADS_DIVIDED_BY_2 = DEG_TO_RAD / 2.f;
					// No Fmod since I know it's already in the [-180, 180) range
					const float YawNoWinding = YawDeltaThisFrame;//FMath::Fmod(YawDeltaThisFrame, 360.0f);
					float SY, CY;
					FMath::SinCos(&SY, &CY, YawNoWinding * RADS_DIVIDED_BY_2);

					/* Knowing that pitch and roll will be 0 I have sped up the calculation of this FQuat a
					bit. Maybe compiler would have done it anyway */
					AddRelativeRotation(FQuat(0.f, 0.f, SY, CY), false);

					RelativeYawRotation = RelativeRotation.Yaw;
				}
				else
				{
					// Just update replicated variable but do not actually rotate component. 
					RelativeYawRotation += YawDeltaThisFrame;
					RelativeYawRotation = FRotator::ClampAxis(RelativeYawRotation);
				}
			}
		}
	}
}

void UBuildingAttackCompStructBase_SK::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Leaving out Super cause I don't need most of what it does

	// This is all I need.
	DOREPLIFETIME(UBuildingAttackCompStructBase_SK, RelativeYawRotation);
}

void UBuildingAttackCompStructBase_SK::PreNetReceive()
{
	// Might not even need this
	UActorComponent::PreNetReceive();

	// Might not even need this either
	bNetUpdateRelativeYawRotation = false;
}

void UBuildingAttackCompStructBase_SK::OnRep_RelativeYawRotation()
{
	if (OwningBuilding->IsInsideFogOfWar() == false)
	{
		bNetUpdateRelativeYawRotation = true;
	}
}

void UBuildingAttackCompStructBase_SK::PostRepNotifies()
{
	/* Left out a lot of stuff by not calling Super. I don't think any of it is needed */

	if (bNetUpdateRelativeYawRotation)
	{
		RelativeRotation.Yaw = RelativeYawRotation;

		/* This will most likely make the change instant. I May want to add some interpolation
		but is instant for now */
		UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate);
		bNetUpdateRelativeYawRotation = false;
	}
}

#if WITH_EDITOR
void UBuildingAttackCompStructBase_SK::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	RelativeYawRotation = RelativeRotation.Yaw;
}
#endif
