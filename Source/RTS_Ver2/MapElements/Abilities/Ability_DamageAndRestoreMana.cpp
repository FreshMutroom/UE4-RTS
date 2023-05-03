// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_DamageAndRestoreMana.h"

//#include "GameFramework/Selectable.h"


AAbility_DamageAndRestoreMana::AAbility_DamageAndRestoreMana()
{
	//~ Begin AAbilityBase interface
	bCallAoEStartFunction = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::True;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
	bHasMultipleOutcomes = EUninitializableBool::True;
	bRequiresTickCount = EUninitializableBool::True;
	//~ End AAbilityBase interface

	ResourceRestorationFactor.Reserve(Statics::NUM_RESOURCE_TYPES);
	ResourceRestorationFactorArray.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceRestorationFactor.Emplace(Statics::ArrayIndexToResourceType(i), 0.5f);
		ResourceRestorationFactorArray.Emplace(0.5f);
	}

	Damage = 50.f;
	DamageType = UDamageType_Default::StaticClass();
}

void AAbility_DamageAndRestoreMana::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	// Do I actually assign a value to the tick count at any time for the client to use?

	/* Deal damage to target */
	const float FinalOutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
	const float TargetsPreAbilityHealth = TargetAsSelectable->GetHealth();
	const float DamageDone = Target->TakeDamage(FinalOutgoingDamage, FDamageEvent(DamageType), nullptr, EffectInstigator);

	/* Check if we killed the target */
	const bool bKilledTarget = (TargetsPreAbilityHealth > 0.f) && (DamageDone >= TargetsPreAbilityHealth);
	if (bKilledTarget)
	{
		OutOutcome = EAbilityOutcome::DidKill;

		//-----------------------------------------------------
		//	Replenish mana on instigator
		//-----------------------------------------------------
		
		// TMap iterating here, might wanna change that to a TArray for performance
		const TMap <EResourceType, int32> & TargetsCostContainer = TargetAsSelectable->GetAttributes()->GetCosts();
		int32 ReplenishAmount = 0;
		for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
		{
			const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);
			ReplenishAmount += ResourceRestorationFactorArray[i] * (TargetsCostContainer.Contains(ResourceType) ? TargetsCostContainer[ResourceType] : 0);
		}
		
		if (InstigatorAsSelectable->GetAttributesModifiable()->GetSelectableResource_1().GetType() == RestoredResourceType)
		{
			InstigatorAsSelectable->GetAttributesModifiable()->GetSelectableResource_1().AdjustAmount(ReplenishAmount,
				GS->GetGameTickCounter(), GS->GetGameTickCounter(), InstigatorAsSelectable, true);
		}
	}
	else
	{
		OutOutcome = EAbilityOutcome::DidNotKill;
	}

	//-----------------------------------------------------
	//	Visuals + audio
	//-----------------------------------------------------

	ShowInstigatorParticles(EffectInstigator, InstigatorAsSelectable, OutOutcome);
	ShowTargetParticles(AbilityInfo, Target, TargetAsSelectable, OutOutcome);
	PlayTargetSound(Target, InstigatorsTeam, OutOutcome);
}

void AAbility_DamageAndRestoreMana::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	if (Outcome == EAbilityOutcome::DidKill)
	{
		//-----------------------------------------------------
		//	Replenish mana on instigator
		//-----------------------------------------------------

		const TMap <EResourceType, int32> & TargetsCostContainer = TargetAsSelectable->GetAttributes()->GetCosts();
		int32 ReplenishAmount = 0;
		for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
		{
			const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);
			ReplenishAmount += ResourceRestorationFactorArray[i] * (TargetsCostContainer.Contains(ResourceType) ? TargetsCostContainer[ResourceType] : 0);
		}

		if (InstigatorAsSelectable->GetAttributesModifiable()->GetSelectableResource_1().GetType() == RestoredResourceType)
		{
			InstigatorAsSelectable->GetAttributesModifiable()->GetSelectableResource_1().AdjustAmount(ReplenishAmount,
				GS->GetGameTickCounter(), ServerTickCountAtTimeOfAbility, InstigatorAsSelectable, true);
		}
	}

	//-----------------------------------------------------
	//	Visuals + audio
	//-----------------------------------------------------

	// Validity check because networking
	if (Statics::IsValid(EffectInstigator))
	{
		ShowInstigatorParticles(EffectInstigator, InstigatorAsSelectable, Outcome);
	}
	
	// Validity check because networking
	if (Statics::IsValid(EffectInstigator))
	{
		ShowTargetParticles(AbilityInfo, Target, TargetAsSelectable, Outcome);
		PlayTargetSound(Target, InstigatorsTeam, Outcome);
	}
}

void AAbility_DamageAndRestoreMana::ShowInstigatorParticles(AActor * AbilityInstigator, 
	ISelectable * InstigatorAsSelectable, EAbilityOutcome Outcome)
{
	// Always spawn these ones no matter the outcome
	if (InstigatorParticles_Template != nullptr)
	{
		const FAttachInfo * AttachInfo = InstigatorAsSelectable->GetBodyLocationInfo(InstigatorParticles_SpawnPoint);
		
		Statics::SpawnFogParticles(InstigatorParticles_Template, GS,
			AbilityInstigator->GetActorLocation() + AttachInfo->GetAttachTransform().GetLocation(),
			AbilityInstigator->GetActorRotation() + AttachInfo->GetAttachTransform().GetRotation().Rotator(),
			AttachInfo->GetAttachTransform().GetScale3D());
	}

	if (TargetSuccessParticles_Template != nullptr)
	{
		// Spawn success particles if target was killed
		if (Outcome == EAbilityOutcome::DidKill)
		{
			const FAttachInfo * AttachInfo = InstigatorAsSelectable->GetBodyLocationInfo(InstigatorSuccessParticles_SpawnPoint);
			
			Statics::SpawnFogParticles(InstigatorParticles_Template, GS,
				AbilityInstigator->GetActorLocation() + AttachInfo->GetAttachTransform().GetLocation(),
				AbilityInstigator->GetActorRotation() + AttachInfo->GetAttachTransform().GetRotation().Rotator(),
				AttachInfo->GetAttachTransform().GetScale3D());
		}
	}
}

void AAbility_DamageAndRestoreMana::ShowTargetParticles(const FContextButtonInfo & AbilityInfo, 
	AActor * Target, ISelectable * TargetAsSelectable, EAbilityOutcome Outcome)
{
	if (Outcome == EAbilityOutcome::DidKill)
	{
		if (TargetSuccessParticles_Template != nullptr)
		{
			const FAttachInfo * AttachInfo = TargetAsSelectable->GetBodyLocationInfo(TargetSuccessParticles_SpawnPoint);

			Statics::SpawnFogParticles(TargetSuccessParticles_Template, GS,
				Target->GetActorLocation() + AttachInfo->GetAttachTransform().GetLocation(),
				Target->GetActorRotation() + AttachInfo->GetAttachTransform().GetRotation().Rotator(),
				AttachInfo->GetAttachTransform().GetScale3D());
		}
	}
	else // Assumed DidNotKill
	{
		assert(Outcome == EAbilityOutcome::DidNotKill);

		if (TargetFailParticles_Template != nullptr)
		{
			TargetAsSelectable->AttachParticles(AbilityInfo, TargetFailParticles_Template, TargetFailParticles_AttachPoint, 0);
		}
	}
}

void AAbility_DamageAndRestoreMana::PlayTargetSound(AActor * Target, ETeam InstigatorsTeam, EAbilityOutcome Outcome)
{
	if (Outcome == EAbilityOutcome::DidKill)
	{
		if (TargetSound_Success != nullptr)
		{
			Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetSound_Success, Target->GetActorLocation(),
				ESoundFogRules::AlwaysKnownOnceHeard);
		}
	}
	else // Assume DidNotKill
	{
		if (TargetSound_Failure != nullptr)
		{
			Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetSound_Failure, Target->GetActorLocation(),
				ESoundFogRules::AlwaysKnownOnceHeard);
		}
	}
}

#if WITH_EDITOR
void AAbility_DamageAndRestoreMana::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Populate ResourceRestorationFactorArray
	ResourceRestorationFactorArray.Reset(); // Reset() because Empty() doesn't work
	ResourceRestorationFactorArray.Init(0, Statics::NUM_RESOURCE_TYPES);
	for (const auto & Pair : ResourceRestorationFactor)
	{
		const int32 ArrayIndex = Statics::ResourceTypeToArrayIndex(Pair.Key);
		ResourceRestorationFactorArray[ArrayIndex] = Pair.Value;
	}

	bCanEditInstigatorParticlesProperties = (InstigatorParticles_Template != nullptr);
	bCanEditTargetFailParticlesProperties = (TargetFailParticles_Template != nullptr);
	bCanEditInstigatorSuccessParticlesProperties = (InstigatorSuccessParticles_Template != nullptr);
	bCanEditTargetSuccessParticlesProperties = (TargetSuccessParticles_Template != nullptr);
}
#endif // WITH_EDITOR
