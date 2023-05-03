// Fill out your copyright notice in the Description page of Project Settings.


#include "BAbility_StealResources.h"

#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerState.h"
#include "MapElements/Building.h"


UBAbility_StealResources::UBAbility_StealResources()
{
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

void UBAbility_StealResources::Server_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome & OutOutcome, int16 & OutRandomNumberSeed16Bits)
{
	ARTSPlayerState * InstigatorsOwner = CastChecked<ISelectable>(AbilityInstigator)->Selectable_GetPS();
	ARTSPlayerState * TargetsOwner = AbilityTarget->GetPS();

	/* Deduct the resources from target */
	int32 PreAbilityResources[Statics::NUM_RESOURCE_TYPES];
	int32 PostAbilityResources[Statics::NUM_RESOURCE_TYPES];
	TargetsOwner->AdjustResources(AmountToDeductFromTargetArray, PreAbilityResources, PostAbilityResources, true);

	/* Gain resources on instigator. Only gain what was actually stolen e.g. if this ability
	steals 1000 but the target only had 700 then we only gain 700 */
	int32 DifferenceArray[Statics::NUM_RESOURCE_TYPES];
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		DifferenceArray[i] = PreAbilityResources[i] - PostAbilityResources[i];
	}
	InstigatorsOwner->GainResources(DifferenceArray, false);

	PlaySound(InstigatorsOwner, TargetsOwner);

	Super::Server_Execute(AbilityInfo, AbilityInstigator, AbilityTarget, OutOutcome, OutRandomNumberSeed16Bits);
}

void UBAbility_StealResources::Client_Execute(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AActor * AbilityInstigator, ABuilding * AbilityTarget, EAbilityOutcome Outcome, int32 RandomNumberSeed)
{
	ARTSPlayerState * InstigatorsOwner = CastChecked<ISelectable>(AbilityInstigator)->Selectable_GetPS();
	ARTSPlayerState * TargetsOwner = AbilityTarget->GetPS();

	PlaySound(InstigatorsOwner, TargetsOwner);

	Super::Client_Execute(AbilityInfo, AbilityInstigator, AbilityTarget, Outcome, RandomNumberSeed);
}

void UBAbility_StealResources::PlaySound(ARTSPlayerState * AbilityInstigatorsOwner, ARTSPlayerState * AbilityTargetsOwner)
{
	if (AbilityInstigatorsOwner->BelongsToLocalPlayer())
	{
		if (InstigatorsSound != nullptr)
		{
			// If "this" doesn't work for world context then can use GS
			Statics::PlaySound2D(this, InstigatorsSound);
		}
	}
	else if (AbilityTargetsOwner->BelongsToLocalPlayer())
	{
		if (TargetsSound != nullptr)
		{
			Statics::PlaySound2D(this, TargetsSound);
		}
	}
}

#if WITH_EDITOR
void UBAbility_StealResources::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
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
