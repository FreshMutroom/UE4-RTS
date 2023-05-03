// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Ability_AoEBuffOrDebuff.generated.h"

/**
 *	Applies a buff or debuff to selectables in an area of effect 
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_AoEBuffOrDebuff : public AAbilityBase
{
	GENERATED_BODY()
	
public:
	
	AAbility_AoEBuffOrDebuff();

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

	/* Area of effect */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float Radius;

	/* Whether it affects enemies */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanHitEnemies;

	/* Whether it affects friendlies. Note if this and bCanHitEnemies are both false then this 
	ability cannot hit anything! */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanHitFriendlies;

	/* Whether it affects flying units */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bCanHitFlying;
};
