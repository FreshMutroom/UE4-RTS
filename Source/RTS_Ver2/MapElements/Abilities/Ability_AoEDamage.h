// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_AoEDamage.generated.h"

class URTSDamageType;
class UCurveFloat;


/**
 *	Deals damage in an area of effect. 
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_AoEDamage : public AAbilityBase
{
	GENERATED_BODY()

public:

	AAbility_AoEDamage();

protected:

	virtual void BeginPlay() override;

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

	// Checks the curve assets are usable
	void CheckCurves();

	UFUNCTION()
	void DealDamage(AActor * AbilityInstigator, const FVector & Location, ETeam InstigatorsTeam);

	// Call DealDamage after a delay
	void DealDamageAfterDelay(AActor * AbilityInstigator, const FVector & Location, ETeam InstigatorsTeam, float DelayAmount);

	/* Calculate how much damage to pass into HitTarget->TakeDamage */
	float CalculateDamage(const FVector & AbilityLocation, AActor * HitTarget) const;

	/* Show particle system at location */
	void ShowTargetLocationParticles(AActor * AbilityInstigator, const FVector & Location);

	FRotator GetTargetLocationParticlesRotation(AActor * AbilityInstigator) const;

	void PlayTargetLocationSound(const FVector & Location, ETeam InstigatorsTeam);

	//------------------------------------------------------------------
	//	Data
	//------------------------------------------------------------------

	/** Radius */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0.001"))
	float Radius;

	/** Base amount of damage to deal before taking into account falloff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	float BaseDamage;

	/** Type of damage to deal */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS|Damage")
	TSubclassOf<URTSDamageType> DamageType;

	/**
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = "0", ClampMax = "0.999"))
	float RandomDamageFactor;

	/** 
	 *	The curve to use for damage falloff. If no curve is specified then there will be no damage
	 *	falloff - every selectable hit will take full damage. 
	 *	
	 *	X axis = range from center. Larger implies further from ability center. Axis range: [0, 1] 
	 *	Y axis = normalized percentage of BaseDamage to deal. Probably want range [0, 1] 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	UCurveFloat * DamageFalloffCurve;

	/**
	 *	The delay from when the ability is used to when the damage happens. 
	 *	0 = instant 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (ClampMin = 0))
	float DamageDelay;

	/** Whether it can hit enemies */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitEnemies;

	/**
	 *	Whether it can hit friendlies. Note if this and bCanHitEnemies are both false then this
	 *	ability cannot hit anything!
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitFriendlies;

	/** Whether it affects flying units */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Targeting")
	bool bCanHitFlying;

	/** Optional particle system to show at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Particle Systems")
	UParticleSystem * TargetLocationParticles;

	/** Optional sound to play at the target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetLocationSound;
};
