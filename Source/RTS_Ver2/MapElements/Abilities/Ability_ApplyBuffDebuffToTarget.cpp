// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_ApplyBuffDebuffToTarget.h"


AAbility_ApplyBuffDebuffToTarget::AAbility_ApplyBuffDebuffToTarget()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes = EUninitializableBool::True;
	bCallAoEStartFunction = EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::True;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
	bRequiresTickCount = EUninitializableBool::False;
	//~ End AAbilityBase interface
}

void AAbility_ApplyBuffDebuffToTarget::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	if (bUseStaticTypeBuffOrDebuff)
	{
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator,
			InstigatorAsSelectable, TargetAsSelectable, StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
	}
	else
	{
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator, InstigatorAsSelectable,
			TargetAsSelectable, TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
	}
}

void AAbility_ApplyBuffDebuffToTarget::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	if (bUseStaticTypeBuffOrDebuff)
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable, TargetAsSelectable,
			StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(),
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
	else
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable, TargetAsSelectable,
			TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(), 
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
}
