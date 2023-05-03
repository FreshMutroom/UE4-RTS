// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_CullingBladeClone.h"
#include "Statics/DamageTypes.h"


AAbility_CullingBladeClone::AAbility_CullingBladeClone()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes =			EUninitializableBool::True;
	bCallAoEStartFunction =			EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	EUninitializableBool::False;
	bRequiresTargetOtherThanSelf =	EUninitializableBool::True;
	bRequiresLocation =				EUninitializableBool::False;
	bHasRandomBehavior =			EUninitializableBool::False;
	bRequiresTickCount =			EUninitializableBool::False;
	//~ End AAbilityBase interface

	HealthThreshold = 300.f;
	BelowThresholdTargetParticles_AttachPoint = ESelectableBodySocket::None;
	AboveThresholdTargetParticles_AttachPoint = ESelectableBodySocket::None;
}

void AAbility_CullingBladeClone::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	OutOutcome = TargetAsSelectable->GetHealth() < HealthThreshold
		? EAbilityOutcome::CullingBlade_BelowThreshold : EAbilityOutcome::CullingBlade_AboveThreshold;

	if (OutOutcome == EAbilityOutcome::CullingBlade_BelowThreshold)
	{
		Target->TakeDamage(BelowThresholdDamage.GetImpactOutgoingDamage(), FDamageEvent(BelowThresholdDamage.GetImpactDamageType()), nullptr, EffectInstigator);

		ShowTargetParticles(TargetAsSelectable, BelowThresholdTargetParticles_Template, BelowThresholdTargetParticles_AttachPoint);
		PlayTargetSound(Target, InstigatorsTeam, BelowThresholdTargetSound);
	}
	else
	{
		Target->TakeDamage(AboveThresholdDamage.GetImpactOutgoingDamage(), FDamageEvent(AboveThresholdDamage.GetImpactDamageType()), nullptr, EffectInstigator);

		ShowTargetParticles(TargetAsSelectable, AboveThresholdTargetParticles_Template, AboveThresholdTargetParticles_AttachPoint);
		PlayTargetSound(Target, InstigatorsTeam, AboveThresholdTargetSound);
	}
}

void AAbility_CullingBladeClone::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	if (Outcome == EAbilityOutcome::CullingBlade_BelowThreshold)
	{
		ShowTargetParticles(TargetAsSelectable, BelowThresholdTargetParticles_Template, BelowThresholdTargetParticles_AttachPoint);
		PlayTargetSound(Target, InstigatorsTeam, BelowThresholdTargetSound);
	}
	else // Assumed CullingBlade_AboveThreshold
	{
		assert(Outcome == EAbilityOutcome::CullingBlade_AboveThreshold);

		ShowTargetParticles(TargetAsSelectable, AboveThresholdTargetParticles_Template, AboveThresholdTargetParticles_AttachPoint);
		PlayTargetSound(Target, InstigatorsTeam, AboveThresholdTargetSound);
	}
}

void AAbility_CullingBladeClone::ShowTargetParticles(ISelectable * TargetAsSelectable, UParticleSystem * Template, ESelectableBodySocket BodyLocaton)
{
	if (Template != nullptr)
	{
		const FAttachInfo * AttachInfo = TargetAsSelectable->GetBodyLocationInfo(BodyLocaton);

		Statics::SpawnFogParticlesAttached(GS, Template, AttachInfo->GetComponent(),
			AttachInfo->GetSocketName(), 0.f, AttachInfo->GetAttachTransform().GetLocation(),
			AttachInfo->GetAttachTransform().GetRotation().Rotator(),
			AttachInfo->GetAttachTransform().GetScale3D());
	}
}

void AAbility_CullingBladeClone::PlayTargetSound(AActor * Target, ETeam InstigatorsTeam, USoundBase * Sound)
{
	if (Sound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, Sound, Target->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}
}
