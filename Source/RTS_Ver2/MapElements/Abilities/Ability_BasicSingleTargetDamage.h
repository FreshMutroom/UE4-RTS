// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_BasicSingleTargetDamage.generated.h"

class URTSDamageType;


/**
 *	Deals damage to a target
 *
 *	I basically ignored what I had done with UE4.19 and implemented this from scratch. Could 
 *	look back at that version and see if there's anything useful there 
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_BasicSingleTargetDamage : public AAbilityBase
{
	GENERATED_BODY()

public:

	AAbility_BasicSingleTargetDamage();
	
	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:

	void ShowTargetParticles(ISelectable * TargetAsSelectable);
	void PlayTargetSound(AActor * Target, ETeam InstigatorsTeam);

	/* How much damage to deal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Damage;

	/* The type of damage to deal */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf <URTSDamageType> DamageType;

	/** 
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0", ClampMax = "0.999"))
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
