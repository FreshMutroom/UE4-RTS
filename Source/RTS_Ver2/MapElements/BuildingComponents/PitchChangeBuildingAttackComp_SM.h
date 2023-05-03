// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/BuildingComponents/BuildingAttackComponent_SM.h"
#include "PitchChangeBuildingAttackComp_SM.generated.h"


/**
 *	A building attack component that can change it's pitch rotation. 
 *
 *	Examples of these:
 *	- in C&C generals: top part of a partiot missle system. To get it rotataing yaw make sure 
 *	to add a IBuildingAttackComp_TurretsBase as it's attach parent. 
 *
 *	If you need animation make sure to choose UPitchChangeBuildingAttackComp_SK instead.
 */
UCLASS()
class RTS_VER2_API UPitchChangeBuildingAttackComp_SM : public UBuildingAttackComponent_SM
{
	GENERATED_BODY()

public:

	UPitchChangeBuildingAttackComp_SM();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

protected:

	/* Rotate pitch towards Target */
	void RotatePitchToFaceTarget(float DeltaTime);

	ETargetTargetingCheckResult GetCurrentTargetTargetingStatus() const;

public:

	// TODO add a OurYaw variable and replicate only it in here
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const override;

	virtual void PreNetReceive() override;

	UFUNCTION()
	void OnRep_RelativePitchRotation();

	// Called right after calling all OnRep notifies (called even when there are no notifies)
	virtual void PostRepNotifies() override;

	//~ Begin IBuildingAttackComp_Turret interface
	virtual void OnParentBuildingExitFogOfWar(bool bOnServer) override;
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor) const override;
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const override;
	//~ End IBuildingAttackComp_Turret interface

protected:

	/** 
	 *	Get in world space the pitch of the component. Note this might not be what 
	 *	GetComponentRotation().Yaw returns 
	 */
	float GetEffectiveComponentPitchRotation() const;

	/* For a given pitch return whether it is possible for the structure to acheive that pitch */
	bool CanPitchBeChangedEnoughToFace(float PitchWeWantToHave) const;

	/** 
	 *	Return whether the pitch of this structure is good enough to attack an actor 
	 *	
	 *	@param OurPitch - pitch of this component
	 *	@param LookAtPitchToTarget - look at pitch of target. This is not a delta. This is 
	 *	whatever UKismetMathLibrary::FindLookAtRotation(OurLoc, TargetsLoc) would return 
	 *	@return - whether we can attack the target, or if we can't then whether we should unaquire 
	 *	the target or not
	 */
	ETargetTargetingCheckResult GetPitchTargetingStatus(float OurPitch, float TargetsLookAtPitch) const;

	//-----------------------------------------------------------
	//	Data
	//-----------------------------------------------------------
	
	/* Relative pitch rotation */
	UPROPERTY(ReplicatedUsing = OnRep_RelativePitchRotation)
	float RelativePitchRotation;

	/* The rate at which the turret can change it's pitch */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float PitchRotationRate;

	/**
	 *	How much pitch can be off by for structure to fire at target. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0", ClampMax = "180.0"))
	float PitchFacingRequirement;

	/* Make sure to update the ClampMin below if you change this */
	static constexpr float MIN_TURRET_PITCH = -89.9f;
	/** 
	 *	Minimum amount of pitch the turret can have. The closer to 0 this is the less the turret 
	 *	can look down and therefore the less chance it can attack actors below it 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "-89.9", ClampMax = "0"))
	float MinAllowedPitch;

	/* Make sure to update the ClampMax below if you change this */
	static constexpr float MAX_TURRET_PITCH = 89.9f;
	/**
	 *	Maximum amount of pitch the turret can have. The closer to 0 this is the less the turret 
	 *	can look up and therefore the less chance it can attack actors above it.  
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0", ClampMax = "89.9"))
	float MaxAllowedPitch;

	/** 
	 *	If true then the turret needs to check whether the pitch required to face an actor before 
	 *	aquiring them is achievable. If MinAllowedPitch, MaxAllowedPitch and PitchFacingRequirement 
	 *	are too restrictive then it's possible the target cannot be faced 
	 */
	UPROPERTY()
	uint8 bNeedsToCheckPitchBeforeTargetAquire : 1;

	uint8 bNetUpdateRelativePitchRotation : 1;

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
	void RunPostEditLogic();
#endif
};
