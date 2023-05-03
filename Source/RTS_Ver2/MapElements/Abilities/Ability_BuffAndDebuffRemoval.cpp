// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_BuffAndDebuffRemoval.h"

// Might be able to get away with removing these includes 


AAbility_BuffAndDebuffRemoval::AAbility_BuffAndDebuffRemoval()
{
	bCallAoEStartFunction = EUninitializableBool::False;
	/* Because I think this ability is used by self cleansing and other target cleansing 
	type abilities it will need this true. Should create a seperate ability class like 
	Ability_SelfBuffRemoval so we can get the bandwidth benefits of not sending it when not 
	required */
	bRequiresTargetOtherThanSelf = EUninitializableBool::True;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
}

bool AAbility_BuffAndDebuffRemoval::IsUsable_TargetChecks(ISelectable * AbilityInstigator, 
	ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const
{
	/* Return true if they have the buff/debuff on them */

	FTickableBuffOrDebuffInstanceInfo * Info;
	if (Type == EBuffOrDebuffType::Buff)
	{
		Info = Target->GetBuffState(BuffOrDebuffToRemove);
	}
	else // Assuming debuff
	{
		assert(Type == EBuffOrDebuffType::Debuff);

		Info = Target->GetDebuffState(BuffOrDebuffToRemove);
	}
	
	const bool bTargetHasThisBuffOrDebuff = (Info != nullptr);

	if (bTargetHasThisBuffOrDebuff)
	{
		return true;
	}
	else
	{
		OutMissingRequirement = EAbilityRequirement::BuffOrDebuffNotPresent;
		return false;
	}
}

void AAbility_BuffAndDebuffRemoval::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;
	
	/* Remove the buff/debuff. Assmued we have already checked that it is on the target. Slight 
	possibly performance problem though: in order for this ability to be used in the first place 
	the target had to have the buff applied to them so we would have checked that before. Now 
	we have to iterate array again to remove it, would be nice if the array index was passed 
	in for us. Compiler maybe could do it */
	if (Type == EBuffOrDebuffType::Buff)
	{
		OutOutcome = static_cast<EAbilityOutcome>(TargetAsSelectable->RemoveBuff(BuffOrDebuffToRemove,
			EffectInstigator, InstigatorAsSelectable, RemovalReason));
	}
	else // Assuming debuff
	{
		assert(Type == EBuffOrDebuffType::Debuff);

		OutOutcome = static_cast<EAbilityOutcome>(TargetAsSelectable->RemoveDebuff(BuffOrDebuffToRemove,
			EffectInstigator, InstigatorAsSelectable, RemovalReason));
	}
}

void AAbility_BuffAndDebuffRemoval::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;
	
	/* Remove the buff/debuff. Assmued we have already checked that it is on the target */
	if (Type == EBuffOrDebuffType::Buff)
	{
		TargetAsSelectable->Client_RemoveBuffGivenOutcome(BuffOrDebuffToRemove, EffectInstigator,
			InstigatorAsSelectable, static_cast<EBuffOrDebuffRemovalOutcome>(Outcome));
	}
	else // Assuming debuff
	{
		assert(Type == EBuffOrDebuffType::Debuff);
			
		TargetAsSelectable->Client_RemoveDebuffGivenOutcome(BuffOrDebuffToRemove, EffectInstigator,
			InstigatorAsSelectable, static_cast<EBuffOrDebuffRemovalOutcome>(Outcome));
	}
}
