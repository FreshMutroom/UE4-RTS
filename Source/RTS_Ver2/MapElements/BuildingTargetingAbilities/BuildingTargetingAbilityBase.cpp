// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingTargetingAbilityBase.h"

#include "Statics/Structs_1.h"


void UBuildingTargetingAbilityBase::InitialSetup()
{
}

void UBuildingTargetingAbilityBase::Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome, int16 & OutRandomNumberSeed16Bits)
{
	if (AbilityInfo.ConsumesInstigator())
	{
		CastChecked<ISelectable>(AbilityInstigator)->Consume_BuildingTargetingAbilityInstigator();
	}
}

void UBuildingTargetingAbilityBase::Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome, int32 RandomNumberSeed)
{
}

int32 UBuildingTargetingAbilityBase::SeedAs16BitTo32Bit(int16 Seed16Bits)
{
	return (int32)(Seed16Bits) * (int32)(INT16_MAX);
}
