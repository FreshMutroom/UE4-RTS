// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_5.h"

#include "Statics/DamageTypes.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/Projectiles/ProjectileBase.h"


FBasicDamageInfo::FBasicDamageInfo()
	: ImpactDamage(5.f)
	, ImpactDamageType(UDamageType_Default::StaticClass())
	, ImpactRandomDamageFactor(0.f) 
	, AoEDamage(0.f) 
	, AoEDamageType(UDamageType_Default::StaticClass()) 
	, AoERandomDamageFactor(0.f)
{
}

FBasicDamageInfo::FBasicDamageInfo(float InImpactDamage, float InImpactRandomDamageFactor)
	: ImpactDamage(InImpactDamage)
	, ImpactDamageType(UDamageType_Default::StaticClass())
	, ImpactRandomDamageFactor(InImpactRandomDamageFactor)
	, AoEDamage(0.f)
	, AoEDamageType(UDamageType_Default::StaticClass())
	, AoERandomDamageFactor(0.f)
{
}

FBasicDamageInfo::FBasicDamageInfo(float InImpactDamage, float InImpactRandomDamageFactor, float InAoEDamage, float InAoERandomDamageFactor) 
	: ImpactDamage(InImpactDamage)
	, ImpactDamageType(UDamageType_Default::StaticClass())
	, ImpactRandomDamageFactor(InImpactRandomDamageFactor)
	, AoEDamage(InAoEDamage)
	, AoEDamageType(UDamageType_Default::StaticClass())
	, AoERandomDamageFactor(InAoERandomDamageFactor)
{
}

void FBasicDamageInfo::SetValuesFromProjectile(UWorld * World, const TSubclassOf<AProjectileBase>& InProjectileBP)
{
	assert(InProjectileBP != nullptr);

	/* It's fine if this is false but not expecting this to be called if so */
	// Commented because crashes otherwise
	//assert(ShouldOverrideProjectileDamageValues());

	/* Call SpawnActorDeferred so projectile isn't added to projectile pool, although perhaps
	we should just let it happen and not destroy it at the end of this func */
	AProjectileBase * Projectile = World->SpawnActorDeferred<AProjectileBase>(InProjectileBP,
		FTransform(FRotator::ZeroRotator, Statics::POOLED_ACTOR_SPAWN_LOCATION, FVector::OneVector));

	// Artillery strike for one wants this
	//Projectile_BP = InProjectileBP;

	// Take values from projectile
	ImpactDamage = Projectile->GetImpactDamage();
	ImpactDamageType = Projectile->GetImpactDamageType();
	ImpactRandomDamageFactor = Projectile->GetImpactRandomDamageFactor();
	AoEDamage = Projectile->GetAoEDamage();
	AoEDamageType = Projectile->GetAoEDamageType();
	AoERandomDamageFactor = Projectile->GetAoERandomDamageFactor();

	// Projectile no longer needed
	Projectile->Destroy();
}

void FBasicDamageInfo::SetValuesFromProjectile(AProjectileBase * Projectile)
{
	ImpactDamage = Projectile->GetImpactDamage();
	ImpactDamageType = Projectile->GetImpactDamageType();
	ImpactRandomDamageFactor = Projectile->GetImpactRandomDamageFactor();
	AoEDamage = Projectile->GetAoEDamage();
	AoEDamageType = Projectile->GetAoEDamageType();
	AoERandomDamageFactor = Projectile->GetAoERandomDamageFactor();
}

float FBasicDamageInfo::GetImpactOutgoingDamage() const
{
	return ImpactDamage * FMath::RandRange(1.f - ImpactRandomDamageFactor, 1.f + ImpactRandomDamageFactor);
}

const TSubclassOf<URTSDamageType>& FBasicDamageInfo::GetImpactDamageType() const
{
	return ImpactDamageType;
}

float FBasicDamageInfo::GetImpactRandomDamageFactor() const
{
	return ImpactRandomDamageFactor;
}
