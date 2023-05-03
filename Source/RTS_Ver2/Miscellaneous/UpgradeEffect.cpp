// Fill out your copyright notice in the Description page of Project Settings.

#include "UpgradeEffect.h"

#include "MapElements/Building.h"
#include "MapElements/Infantry.h"

void UUpgradeEffect::ApplyEffect_Building(ABuilding * Building, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	/* Add any default functionality to upgrades here and call it with
	Super::ApplyEffect_Building(Building, UpgradeLevel) in derived classes.
	e.g. Perhaps you want the upgraded building to flash for a second or two */
}

void UUpgradeEffect::ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	/* Add any default functionality to upgrades here and call it with
	Super::ApplyEffect_Infantry(Infantry, UpgradeLevel) in derived classes
	e.g. Perhaps you want the upgraded infantry to flash for a second or two */
}


//=============================================================================================
// ---------- UUpgrade_DamageAndDefense ----------
//=============================================================================================

UUpgrade_DamageAndDefense::UUpgrade_DamageAndDefense()
{
	DamageMultiplier = 1.2f;
	DefenseMultiplier = 1.f;
}

void UUpgrade_DamageAndDefense::ApplyEffect_Building(ABuilding * Building, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	// TODO: modifying defense components attack damage

	FBuildingAttributes * BuildingAttributes = Building->GetBuildingAttributesModifiable();

	BuildingAttributes->SetDefenseMultiplierViaMultiplier(DefenseMultiplier, Building);
}

void UUpgrade_DamageAndDefense::ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	FAttackAttributes * AttackAttributes = Infantry->GetAttackAttributesModifiable();
	
	/* Increase damage they dish out. 
	If DamageMultiplier == 1.2f then this line just increased damage by 20% */
	AttackAttributes->SetImpactDamageViaMultiplier(DamageMultiplier, Infantry);

	FInfantryAttributes * InfantryAttributes = Infantry->GetInfantryAttributesModifiable();

	/* Modify incoming damage. 
	If DefenseMultiplier == 0.9f then this line just reduced damage taken by 10% */
	InfantryAttributes->SetDefenseMultiplierViaMultiplier(DefenseMultiplier, Infantry);
}


//=============================================================================================
// ---------- UUpgrade_IncreaseResourceCapacity ----------
//=============================================================================================

UUpgrade_IncreaseResourceCapacity::UUpgrade_IncreaseResourceCapacity()
{
	ResourceCapacityMultipliers.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceCapacityMultipliers.Emplace(Statics::ArrayIndexToResourceType(i), 1.5f);
	}

	ResourceCapacityMultipliersArray.Init(1.5f, Statics::NUM_RESOURCE_TYPES);
}

void UUpgrade_IncreaseResourceCapacity::ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	FInfantryAttributes * Attributes = Infantry->GetInfantryAttributesModifiable();
	TMap<EResourceType, FResourceCollectionAttribute> & ResourceProperties = Attributes->ResourceGatheringProperties.GetAllAttributesModifiable();

	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);
		ResourceProperties[ResourceType].SetCapacity(ResourceProperties[ResourceType].GetCapacity() * ResourceCapacityMultipliersArray[i]);
	}
}

#if WITH_EDITOR
void UUpgrade_IncreaseResourceCapacity::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Populate ResourceCapacityMultipliersArray
	for (const auto & Pair : ResourceCapacityMultipliers)
	{
		ResourceCapacityMultipliersArray[Statics::ResourceTypeToArrayIndex(Pair.Key)] = Pair.Value;
	}
}
#endif


//=============================================================================================
// ---------- UUpgrade_AttackSpeed ----------
//=============================================================================================

UUpgrade_AttackSpeed::UUpgrade_AttackSpeed()
{
#if WITH_EDITOR
	Multiplier_EDITOR = 0.25f;
	CalculateActualMultiplier();
#endif
}

void UUpgrade_AttackSpeed::ApplyEffect_Infantry(AInfantry * Infantry, bool bIsFreshSpawn, uint8 UpgradeLevel)
{
	FAttackAttributes * AttackAttributes = Infantry->GetAttackAttributesModifiable();
	AttackAttributes->SetAttackRateViaMultiplier(Multiplier, Infantry);
}

#if WITH_EDITOR
void UUpgrade_AttackSpeed::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	CalculateActualMultiplier();
}

void UUpgrade_AttackSpeed::CalculateActualMultiplier()
{
	if (Multiplier_EDITOR >= 0.f)
	{
		Multiplier = 1.f / (1.f + Multiplier_EDITOR);
	}
	else
	{
		Multiplier = 1.f - Multiplier_EDITOR;
	}
}
#endif
