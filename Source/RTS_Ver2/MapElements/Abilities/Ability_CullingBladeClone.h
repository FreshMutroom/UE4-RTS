// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"

#include "Statics/Structs_5.h"
#include "Ability_CullingBladeClone.generated.h"

class URTSDamageType;


/**
 *	Deals damage to target. Amount varies based on how much health the target has.
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_CullingBladeClone : public AAbilityBase
{
	GENERATED_BODY()

public:
	
	AAbility_CullingBladeClone();

	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;

protected:

	void ShowTargetParticles(ISelectable * TargetAsSelectable, UParticleSystem * Template, ESelectableBodySocket BodyLocation);
	void PlayTargetSound(AActor * Target, ETeam InstigatorsTeam, USoundBase * Sound);

	/* Absolute value of health where behavior varies */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior", meta = (ClampMin = 0))
	float HealthThreshold;

	//===========================================================================================
	//	Below Threshold Variables
	//===========================================================================================

	/* Damage when target is below threshold */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	FBasicDamageInfo BelowThresholdDamage;

	/* Optional particle system template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * BelowThresholdTargetParticles_Template;

	/**
	 *	Where on the selectable the particles should try attach to.
	 *	EditCondition = (TargetParticles_Template != nullptr)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	ESelectableBodySocket BelowThresholdTargetParticles_AttachPoint;

	/* Optional sound to play at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * BelowThresholdTargetSound;

	//===========================================================================================
	//	Above Threshold Variables
	//===========================================================================================

	/* Damage when target is above threshold */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	FBasicDamageInfo AboveThresholdDamage;

	/* Optional particle system template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * AboveThresholdTargetParticles_Template;

	/**
	 *	Where on the selectable the particles should try attach to.
	 *	EditCondition = (TargetParticles_Template != nullptr)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	ESelectableBodySocket AboveThresholdTargetParticles_AttachPoint;

	/* Optional sound to play at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * AboveThresholdTargetSound;
};
