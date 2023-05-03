// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_SingleTargetBuffOrDebuff.h"


UAbility_SingleTargetBuffOrDebuff::UAbility_SingleTargetBuffOrDebuff()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::True;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::True;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::False;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::True; // True cause the buff/debuff might manipulate mana on application
	//~ End UCommanderAbilityBase variables
}

void UAbility_SingleTargetBuffOrDebuff::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	ISelectable * TargetAsSelectable = CastChecked<ISelectable>(AbilityTarget);

	if (bUseStaticTypeBuffOrDebuff)
	{
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(AbilityInstigator,
			nullptr, TargetAsSelectable, StaticBuffOrDebuffType, TargetAsSelectable->Selectable_GetGI()));
	}
	else
	{
		OutOutcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(AbilityInstigator,
			nullptr, TargetAsSelectable, TickableBuffOrDebuffType, TargetAsSelectable->Selectable_GetGI()));
	}
}

void UAbility_SingleTargetBuffOrDebuff::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	ISelectable * TargetAsSelectable = CastChecked<ISelectable>(AbilityTarget);

	if (bUseStaticTypeBuffOrDebuff)
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(AbilityInstigator, nullptr, TargetAsSelectable,
			StaticBuffOrDebuffType, TargetAsSelectable->Selectable_GetGI(),
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
	else
	{
		Statics::Client_ApplyBuffOrDebuffGivenOutcome(AbilityInstigator, nullptr, TargetAsSelectable,
			TickableBuffOrDebuffType, TargetAsSelectable->Selectable_GetGI(),
			static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
	}
}
