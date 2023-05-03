// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_ApplyBuffDebuffToTarget.generated.h"

/**
 *	Applys a buff/debuff to a target
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_ApplyBuffDebuffToTarget : public AAbilityBase
{
	GENERATED_BODY()

public:

	AAbility_ApplyBuffDebuffToTarget();

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
