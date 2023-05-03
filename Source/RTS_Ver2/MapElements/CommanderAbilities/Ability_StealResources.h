// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"
#include "Ability_StealResources.generated.h"

enum class EResourceType : uint8;


/**
 *	Steals some resources from a player.
 */
UCLASS(Blueprintable)
class RTS_VER2_API UAbility_StealResources : public UCommanderAbilityBase
{
	GENERATED_BODY()
	
public:

	UAbility_StealResources();

	//~ Begin UCommanderAbilityBase interface
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	void PlaySound(ARTSPlayerState * AbilityInstigator, ARTSPlayerState * AbilityTarget);

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

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
