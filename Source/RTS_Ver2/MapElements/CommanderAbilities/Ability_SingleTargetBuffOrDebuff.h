// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"
#include "Ability_SingleTargetBuffOrDebuff.generated.h"

/**
 *	Applies a buff or debuff to a selectable
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UAbility_SingleTargetBuffOrDebuff : public UCommanderAbilityBase
{
	GENERATED_BODY()
	
public:

	UAbility_SingleTargetBuffOrDebuff();

	//~ Begin UCommanderAbilityBase
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase

protected:

	//------------------------------------------------------
	//	Data
	//------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bUseStaticTypeBuffOrDebuff;

	/* Buff/debuff to apply */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bUseStaticTypeBuffOrDebuff))
	EStaticBuffAndDebuffType StaticBuffOrDebuffType;

	/* Buff/debuff to apply */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = "!bUseStaticTypeBuffOrDebuff"))
	ETickableBuffAndDebuffType TickableBuffOrDebuffType;
};
