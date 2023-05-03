// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_BasicApplyBuffOrDebuff.generated.h"


/**
 *	Just applies a buff or debuff to a someone. No particles or anything like that. 
 *
 *	This only works for applying buffs/debuffs to self, not to anyone else
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_BasicApplyBuffOrDebuff : public AAbilityBase
{
	GENERATED_BODY()
	
public:
	
	AAbility_BasicApplyBuffOrDebuff();

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:

	/* Whether to apply a static type buff/debuff or a tickable type buff/debuff */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bUseStaticTypeBuffOrDebuff;

	/* The type of buff/debuff to apply */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = "bUseStaticTypeBuffOrDebuff"))
	EStaticBuffAndDebuffType StaticBuffOrDebuffType;

	/* The type of buff/debuff to apply */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = "!bUseStaticTypeBuffOrDebuff"))
	ETickableBuffAndDebuffType TickableBuffOrDebuffType;
};
