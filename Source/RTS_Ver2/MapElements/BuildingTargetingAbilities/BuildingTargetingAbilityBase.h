// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h"
#include "BuildingTargetingAbilityBase.generated.h"

struct FBuildingTargetingAbilityStaticInfo;
class ABuilding;
enum class EAbilityOutcome : uint8;


/**
 *	Contains data about a building targeting ability. 
 *
 *	Some examples of building targeting abilities:
 *	- C&C engineers can capture enemy buildings or repair friendly ones
 *	- C&C spies can reveal what enemy buildings are producing
 */
UCLASS(Abstract)
class RTS_VER2_API UBuildingTargetingAbilityBase : public UObject
{
	GENERATED_BODY()

public:

	/* Called during setup of game instance */
	virtual void InitialSetup();

	/* Carries out the ability on the server */
	virtual void Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome,
		int16 & OutRandomNumberSeed16Bits);

	/* Carries out the ability on the client */
	virtual void Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome,
		int32 RandomNumberSeed);

	/* Converts the random number seed from 16bits to 32bits */
	static int32 SeedAs16BitTo32Bit(int16 Seed16Bits);
};
