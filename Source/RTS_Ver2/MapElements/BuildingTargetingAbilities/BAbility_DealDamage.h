// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/BuildingTargetingAbilities/BuildingTargetingAbilityBase.h"
#include "BAbility_DealDamage.generated.h"


/**
 *	Deals damage to the building. Damage is percentage based.
 */
UCLASS(Blueprintable)
class RTS_VER2_API UBAbility_DealDamage : public UBuildingTargetingAbilityBase
{
	GENERATED_BODY()
	
public:

	UBAbility_DealDamage();

	//~ Begin UBuildingTargetingAbilityBase interface
	virtual void Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome,
		int16 & OutRandomNumberSeed16Bits) override;
	virtual void Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome,
		int32 RandomNumberSeed) override;
	//~ End UBuildingTargetingAbilityBase interface

protected:

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	/* Normalized percentage of damage to deal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float DamagePercent;
};
