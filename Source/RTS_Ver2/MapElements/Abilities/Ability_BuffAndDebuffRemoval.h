// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"

#include "Statics/OtherEnums.h"
#include "Ability_BuffAndDebuffRemoval.generated.h"


/**
 *	Removes a buff/debuff
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_BuffAndDebuffRemoval : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_BuffAndDebuffRemoval();

	//~ Begin AAbilityBase interface
	virtual bool IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target, 
		EAbilityRequirement & OutMissingRequirement) const override;
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:

	/* The buff/debuff this ability removes */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	ETickableBuffAndDebuffType BuffOrDebuffToRemove;
	
	/* Whether BuffOrDebuffToRemove is a buff or debuff. Important to get this right */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffOrDebuffType Type;

	/* What we consider the reason the buff/debuff was removed */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EBuffAndDebuffRemovalReason RemovalReason;
};
