// Fill out your copyright notice in the Description page of Project Settings.


#include "BAbility_DealDamage.h"

#include "Statics/DamageTypes.h"


UBAbility_DealDamage::UBAbility_DealDamage()
{
	DamagePercent = 0.5001f;
}

void UBAbility_DealDamage::Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome, int16 & OutRandomNumberSeed16Bits)
{
	/* This relies on UDamageType_Default having all damage multipliers of 1. Otherwise 
	we'll deal more/less damage than we want to */
	AbilityTarget->TakeDamage(AbilityTarget->GetBuildingAttributes()->GetMaxHealth() * DamagePercent, 
		FDamageEvent(UDamageType_Default::StaticClass()), nullptr, AbilityInstigator);

	Super::Server_Execute(AbilityInfo, AbilityInstigator, AbilityTarget, OutOutcome, OutRandomNumberSeed16Bits);
}

void UBAbility_DealDamage::Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome, int32 RandomNumberSeed)
{
	Super::Client_Execute(AbilityInfo, AbilityInstigator, AbilityTarget, Outcome, RandomNumberSeed);
}
