// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"

#include "Statics/Structs_5.h"
#include "CAbility_SingleTargetDamage.generated.h"


/**
 *	Deals damage to a single target. Very basic ability. Mainly just added this for testing
 */
UCLASS(Blueprintable)
class RTS_VER2_API UCAbility_SingleTargetDamage : public UCommanderAbilityBase
{
	GENERATED_BODY()

public:
	
	UCAbility_SingleTargetDamage();

	//~ Begin UCommanderAbilityBase interface
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	/* How much damage to deal */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FBasicDamageInfo DamageInfo;
};
