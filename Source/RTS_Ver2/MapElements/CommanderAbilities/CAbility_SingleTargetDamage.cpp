// Fill out your copyright notice in the Description page of Project Settings.

#include "CAbility_SingleTargetDamage.h"


UCAbility_SingleTargetDamage::UCAbility_SingleTargetDamage()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::True;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::False;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	DamageInfo.ImpactDamage = 150.f;
}

void UCAbility_SingleTargetDamage::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	AbilityTarget->TakeDamage(DamageInfo.GetImpactOutgoingDamage(), FDamageEvent(DamageInfo.ImpactDamageType),
		nullptr, nullptr);
}

void UCAbility_SingleTargetDamage::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;
}
