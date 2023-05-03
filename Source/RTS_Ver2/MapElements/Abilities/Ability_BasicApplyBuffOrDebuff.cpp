// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_BasicApplyBuffOrDebuff.h"

#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/Selectable.h"


AAbility_BasicApplyBuffOrDebuff::AAbility_BasicApplyBuffOrDebuff()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes = EUninitializableBool::True;
	bCallAoEStartFunction = EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::False;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
	//~ End AAbilityBase interface
}

void AAbility_BasicApplyBuffOrDebuff::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;
	
	if (bUseStaticTypeBuffOrDebuff)
	{	
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator, InstigatorAsSelectable, InstigatorAsSelectable,
			StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
	}
	else
	{
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator, InstigatorAsSelectable, InstigatorAsSelectable,
			TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
	}
}

void AAbility_BasicApplyBuffOrDebuff::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;
		
	if (bUseStaticTypeBuffOrDebuff)
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable, InstigatorAsSelectable,
			StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(), 
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
	else
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable, InstigatorAsSelectable,
			TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(), 
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
}
