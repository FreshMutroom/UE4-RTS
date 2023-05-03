// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
/* This include includes a EUnitType definition which interferes with my definition.
Commenting it seems to cause no harm */
//#include "UObject/NoExportTypes.h"
#include "UpgradeEffect.generated.h"

class AInfantry;
class ABuilding;


/**
 *	Inside this file are definitions for different types of upgrades. Each must
 *	override ApplyEffect_*. When these ApplyEffect functions are called
 *	the selectables they should not affect have already been filtered out. You do
 *	not need to do any type of checks to see whether to apply the upgrade or not.
 *
 *	Define upgrades in each faction info. There you can choose what selectables
 *	the upgrades affect, how long to research etc. This file is only for defining
 *	the effects of upgrades.
 */


/**
 *	An abstract class that decides what effect upgrades have.
 *	Derive from this class and override its ApplyEffect_* functions to
 *	customize what upgrades do.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UUpgradeEffect : public UObject
{
	GENERATED_BODY()

public:

	/** 
	 *	Apply effects of upgrade to a building. By the time this function is called we have 
	 *	already filtered out which buildings are affected. You can stil branch on their type 
	 *	though if you would like your upgrade to do different things to different types 
	 *	e.g. if (Building->GetAttributesBase().GetBuildingType() == EBuildingType::PowerPlant) 
	 *			 Building->DoSomething();
	 *		 else
	 *			 Building->DoSomethingDifferent();		
	 *--------------------------------------------------------------------------------------------
	 *
	 *	@param Building - the building to apply the upgrade to
	 *	@param bIsFreshSpawn - if true then we are applying this upgrade on the building's spawn. 
	 *	@param UpgradeLevel - the level of the upgrade. If the upgrade can only be researched
	 *	once then this will be 1 
	 */
	virtual void ApplyEffect_Building(ABuilding * Building, bool bIsFreshSpawn, uint8 UpgradeLevel = 1);

	/** 
	 *	Apply the effects of this upgrade to an infantry.
	 *	
	 *	@param Infantry - the infantry to apply the upgrade to
	 *	@param bIsFreshSpawn - if true then we are applying this upgrade on the unit's spawn
	 *	@param UpgradeLevel - the level of the upgrade. If the upgrade can only be researched
	 *	once then this will be 1 
	 */
	virtual void ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel = 1);
};


/** A basic upgrade that can change how much damage a selectable deals and receives */
UCLASS()
class RTS_VER2_API UUpgrade_DamageAndDefense : public UUpgradeEffect
{
	GENERATED_BODY()

protected:

	UUpgrade_DamageAndDefense();

	/* Outgoing damage multiplier multiplier e.g. 1.1 = 10% more damage */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float DamageMultiplier;

	/* Incoming damage multiplier multiplier e.g. 0.9 = 10% less incoming damage */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float DefenseMultiplier;

public:

	virtual void ApplyEffect_Building(ABuilding * Building, bool bIsFreshSpawn, uint8 UpgradeLevel = 1) override;

	virtual void ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel = 1) override;
};


/* An upgrade that increases how much resources a collector can carry */
UCLASS()
class RTS_VER2_API UUpgrade_IncreaseResourceCapacity : public UUpgradeEffect
{
	GENERATED_BODY()

public:

	UUpgrade_IncreaseResourceCapacity();

	virtual void ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel = 1) override;

	//---------------------------------------------------
	//	Data
	//---------------------------------------------------

#if WITH_EDITORONLY_DATA
	/* Multipliers for resource capacity */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap <EResourceType, float> ResourceCapacityMultipliers;
#endif

	UPROPERTY()
	TArray <float> ResourceCapacityMultipliersArray;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};


/* An upgrade that modifies attack speed */
UCLASS(Abstract)
class RTS_VER2_API UUpgrade_AttackSpeed : public UUpgradeEffect
{
	GENERATED_BODY()

public:

	UUpgrade_AttackSpeed();

	virtual void ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel = 1) override;

protected:

#if WITH_EDITORONLY_DATA
	/** 
	 *	Normalized value for how much to increase attack speed by. Use negative values to decrease 
	 *	attack speed.
	 *
	 *	e.g.
	 *	0.25 = increase attack speed by 25%
	 *	-1 = reduce attack speed by 100%
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Attack Speed Increase"))
	float Multiplier_EDITOR;
#endif

	// Actual mutliplier to pass into SetAttackRateViaMultiplier
	UPROPERTY()
	float Multiplier;

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;

	void CalculateActualMultiplier();
#endif
};
