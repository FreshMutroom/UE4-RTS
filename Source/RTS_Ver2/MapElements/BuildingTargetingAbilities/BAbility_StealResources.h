// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/BuildingTargetingAbilities/BuildingTargetingAbilityBase.h"
#include "BAbility_StealResources.generated.h"

class USoundBase;
enum class EResourceType : uint8;
class ARTSPlayerState;


/**
 *	Takes some resources from the owner of the building, just like Black Lotus' cash
 *	grab ability in C&C generals
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UBAbility_StealResources : public UBuildingTargetingAbilityBase
{
	GENERATED_BODY()
	
public:

	UBAbility_StealResources();

	//~ Begin UBuildingTargetingAbilityBase interface
	virtual void Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome,
		int16 & OutRandomNumberSeed16Bits) override;
	virtual void Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo,
		AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome,
		int32 RandomNumberSeed) override;
	//~ End UBuildingTargetingAbilityBase interface 

protected:

	/* Play the ability's sound */
	void PlaySound(ARTSPlayerState * AbilityInstigatorsOwner, ARTSPlayerState * AbilityTargetsOwner);

	//--------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------

#if WITH_EDITORONLY_DATA
	/** How much resources to steal from player */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TMap<EResourceType, int32> AmountToDeductFromTarget;
#endif

	/* Contents of AmountToDeductFromTarget but it they need to be multiplied by -1 */
	UPROPERTY()
	TArray<int32> AmountToDeductFromTargetArray;

	/** UI sound to play for the instigator when they use this */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * InstigatorsSound;

	/** UI sound to play for the target when this is used on them */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * TargetsSound;

public:

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
