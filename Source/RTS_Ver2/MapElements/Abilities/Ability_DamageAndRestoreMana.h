// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_DamageAndRestoreMana.generated.h"

class URTSDamageType;


/**
 *	- Deal damage damage to a single target. 
 *	- Instigator restores some mana if they kill the target with this ability. Amount restored 
 *	is based on the resource cost of the target.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_DamageAndRestoreMana : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_DamageAndRestoreMana();

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:
	
	void ShowInstigatorParticles(AActor * AbilityInstigator, ISelectable * InstigatorAsSelectable, EAbilityOutcome Outcome);
	void ShowTargetParticles(const FContextButtonInfo & AbilityInfo, AActor * Target, ISelectable * TargetAsSelectable, EAbilityOutcome Outcome);
	void PlayTargetSound(AActor * Target, ETeam InstigatorsTeam, EAbilityOutcome Outcome);

	//===========================================================================================
	//	Selectable Resource Restoration
	//===========================================================================================

	/* The selectable resource that is restored if the target is killed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Restoration")
	ESelectableResourceType RestoredResourceType;

#if WITH_EDITORONLY_DATA
	/**
	 *	How much to multiply the resource by 
	 *	
	 *	e.g. if the mineral cost of the destroyed unit is 200 and the mineral entry in here 
	 *	equals 0.5f then 100 mana is restored 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Restoration")
	TMap <EResourceType, float> ResourceRestorationFactor;
#endif

	/* Populated on post edit. Here because it's faster iteration an array instead of a hashmap */
	UPROPERTY()
	TArray <float> ResourceRestorationFactorArray;

	//===========================================================================================
	//	Damage
	//===========================================================================================

	/* How much damage to deal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	float Damage;

	/* The type of damage to deal */
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

	/** Optional particles to spawn on the instigator */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * InstigatorParticles_Template;

	/* Where on the instigator InstigatorParticles_Template should spawn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems", meta = (EditCondition = bCanEditInstigatorParticlesProperties))
	ESelectableBodySocket InstigatorParticles_SpawnPoint;

	/* Optional particle system template to attach to target if they are not killed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetFailParticles_Template;

	/**
	 *	Where on the target selectable the particles should try attach to.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems", meta = (EditCondition = bCanEditTargetFailParticlesProperties))
	ESelectableBodySocket TargetFailParticles_AttachPoint;

	/* Particles to show on the instigator only if they successfully kill the target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * InstigatorSuccessParticles_Template;

	/* Where on instigator to spawn the success particles */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems", meta = (EditCondition = bCanEditInstigatorSuccessParticlesProperties))
	ESelectableBodySocket InstigatorSuccessParticles_SpawnPoint;

	/* Optional particle system template to attach to target if they are killed by ability */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetSuccessParticles_Template;

	/**
	 *	Where on the target selectable the particles should spawn at.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems", meta = (EditCondition = bCanEditTargetSuccessParticlesProperties))
	ESelectableBodySocket TargetSuccessParticles_SpawnPoint;

	//============================================================================================
	//	Audio
	//============================================================================================

	/* Optional sound to play at target location when the ability does not kill the target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetSound_Failure;

	/* Optional sound to play at target location when the ability kills the target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetSound_Success;


	//============================================================================================
	//	Editor Variables
	//============================================================================================

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditInstigatorParticlesProperties;

	UPROPERTY()
	bool bCanEditTargetFailParticlesProperties;

	UPROPERTY()
	bool bCanEditInstigatorSuccessParticlesProperties;

	UPROPERTY()
	bool bCanEditTargetSuccessParticlesProperties;
#endif

public:

#if WITH_EDITOR

	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

#endif
};
