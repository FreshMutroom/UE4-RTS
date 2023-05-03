// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs_5.generated.h"

class URTSDamageType;
class AProjectileBase;


USTRUCT()
struct FBasicDamageInfo
{
	GENERATED_BODY()

public:

	/** How much damage to deal to target */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float ImpactDamage;

	/** Type of damage to deal */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS")
	TSubclassOf <URTSDamageType> ImpactDamageType;

	/**
	 *	Setting this value > 0 adds some randomness to damage
	 *	OutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0", ClampMax = "0.999"))
	float ImpactRandomDamageFactor;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "AoE Damage"))
	float AoEDamage;

	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "AoE Damage Type"))
	TSubclassOf<URTSDamageType> AoEDamageType;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0", ClampMax = "0.999", DisplayName = "AoE Random Damage Factor"))
	float AoERandomDamageFactor;

public:

	FBasicDamageInfo();

	FBasicDamageInfo(float InImpactDamage, float InImpactRandomDamageFactor);

	FBasicDamageInfo(float InImpactDamage, float InImpactRandomDamageFactor, float InAoEDamage, float InAoERandomDamageFactor);

	void SetValues(float InImpactDamage, const TSubclassOf<URTSDamageType> & InImpactDamageType, float InImpactRandomDamageFactor, 
		float InAoEDamage, const TSubclassOf<URTSDamageType> & InAoEDamageType, float InAoERandomDamageFactor)
	{
		ImpactDamage = InImpactDamage;
		ImpactDamageType = InImpactDamageType;
		ImpactRandomDamageFactor = InImpactRandomDamageFactor;
		AoEDamage = InAoEDamage;
		AoEDamageType = InAoEDamageType;
		AoERandomDamageFactor = InAoERandomDamageFactor;
	}

	/**
	 *	Given a projectile blueprint, spawn that projectile and set the damage values on this
	 *	struct using the damage values of the projectile, then destroy the projectile
	 */
	void SetValuesFromProjectile(UWorld * World, const TSubclassOf<AProjectileBase> & InProjectileBP);

	/* Set damage values from a projectile. This version uses an already spawned projectile actor */
	void SetValuesFromProjectile(AProjectileBase * Projectile);

	float GetImpactOutgoingDamage() const;
	const TSubclassOf<URTSDamageType> & GetImpactDamageType() const;
	float GetImpactRandomDamageFactor() const;
};
