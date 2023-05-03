// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_FinishingBlow.generated.h"

class URTSDamageType;


/**
 *	Only usable on targets with health below a certain percentage
 *	Deals damage to a target.
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_FinishingBlow : public AAbilityBase
{
	GENERATED_BODY()

public:
	
	AAbility_FinishingBlow();

	//~ Begin AAbilityBase interface
	virtual bool IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const override;
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:

	float GetOutgoingDamage() const;

	void ShowTargetParticles(ISelectable * TargetAsSelectable);
	void PlayTargetSound(AActor * Target, ETeam InstigatorsTeam);

	//--------------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------------

	/** 
	 *	Percentage of health target must be below to be able to use ability (normalized to range 
	 *	[0, 1] of course) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting", meta = (ClampMin = 0, ClampMax = 1))
	float HealthPercentageThreshold;

	/* How much damage to deal to target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	float Damage;

	/* Type of damage to deal */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage")
	TSubclassOf <URTSDamageType> DamageType;

	/**
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999"))
	float RandomDamageFactor;

	//===========================================================================================
	//	Visuals
	//===========================================================================================

	/* Optional particle system template */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetParticles_Template;

	/**
	 *	Where on the selectable the particles should try attach to.
	 *	EditCondition = (TargetParticles_Template != nullptr)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	ESelectableBodySocket TargetParticles_AttachPoint;

	//============================================================================================
	//	Audio
	//============================================================================================

	/* Optional sound to play at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetSound;
};
