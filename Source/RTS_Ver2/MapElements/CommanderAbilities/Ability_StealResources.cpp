// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_StealResources.h"


UAbility_StealResources::UAbility_StealResources()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::True;
	bRequiresLocation =				ECommanderUninitializableBool::False;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	/* Default fill containers that decide how much is stolen */
	AmountToDeductFromTarget.Reserve(Statics::NUM_RESOURCE_TYPES);
	AmountToDeductFromTargetArray.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);

		AmountToDeductFromTarget.Emplace(ResourceType, 1000);
		AmountToDeductFromTargetArray.Emplace(-1000);
	}
}

void UAbility_StealResources::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	ARTSPlayerState * PlayerTarget = CastChecked<ARTSPlayerState>(AbilityTarget);

	/* Deduct the resources from target */
	int32 PreAbilityResources[Statics::NUM_RESOURCE_TYPES];
	int32 PostAbilityResources[Statics::NUM_RESOURCE_TYPES];
	PlayerTarget->AdjustResources(AmountToDeductFromTargetArray, PreAbilityResources, PostAbilityResources, true);

	/* Gain resources on instigator. Only gain what was actually stolen e.g. if this ability 
	steals 1000 but the target only had 700 then we only gain 700 */
	int32 DifferenceArray[Statics::NUM_RESOURCE_TYPES];
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		DifferenceArray[i] = PreAbilityResources[i] - PostAbilityResources[i];
	}
	AbilityInstigator->GainResources(DifferenceArray, false);

	PlaySound(AbilityInstigator, PlayerTarget);
}

void UAbility_StealResources::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	ARTSPlayerState * PlayerTarget = CastChecked<ARTSPlayerState>(AbilityTarget);

	PlaySound(AbilityInstigator, PlayerTarget);
}

void UAbility_StealResources::PlaySound(ARTSPlayerState * AbilityInstigator, ARTSPlayerState * AbilityTarget)
{
	if (AbilityInstigator->BelongsToLocalPlayer())
	{
		if (InstigatorsSound != nullptr)
		{
			// If "this" doesn't work for world context then can use GS
			Statics::PlaySound2D(this, InstigatorsSound);
		}
	}
	else if (AbilityTarget->BelongsToLocalPlayer())
	{
		if (TargetsSound != nullptr)
		{
			Statics::PlaySound2D(this, TargetsSound);
		}
	}
}

#if WITH_EDITOR
void UAbility_StealResources::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	/* Populate resource array */
	AmountToDeductFromTargetArray.Init(0, Statics::NUM_RESOURCE_TYPES);
	for (auto & Pair : AmountToDeductFromTarget)
	{
		/* Cannot allow negative values (values that would actually give the target resources). 
		If you want some kind of 'donate resources' ability then create a new class for that */
		Pair.Value = FMath::Max<int32>(0, Pair.Value);

		const int32 ArrayIndex = Statics::ResourceTypeToArrayIndex(Pair.Key);
		AmountToDeductFromTargetArray[ArrayIndex] = -Pair.Value;
	}
}
#endif
