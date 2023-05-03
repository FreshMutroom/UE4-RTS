// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_FinishingBlow.h"


AAbility_FinishingBlow::AAbility_FinishingBlow()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes =			EUninitializableBool::False;
	bCallAoEStartFunction =			EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	EUninitializableBool::False;
	bRequiresTargetOtherThanSelf =	EUninitializableBool::True;
	bRequiresLocation =				EUninitializableBool::False;
	bHasRandomBehavior =			EUninitializableBool::False;
	bRequiresTickCount =			EUninitializableBool::False;
	//~ End AAbilityBase interface

	HealthPercentageThreshold = 0.5f;
	Damage = 999999.f;
	DamageType = UDamageType_Default::StaticClass();
	RandomDamageFactor = 0.f;
	TargetParticles_AttachPoint = ESelectableBodySocket::None;
}

bool AAbility_FinishingBlow::IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const
{
	const FSelectableAttributesBasic * Attributes = Target->GetAttributes();
	
	if (Attributes != nullptr)
	{
		if ((Target->GetHealth() / Attributes->GetMaxHealth()) < (HealthPercentageThreshold))
		{
			return true;
		}
		else
		{
			OutMissingRequirement = EAbilityRequirement::TargetsHealthNotLowEnough;
			return false;
		}
	}
	else
	{
		// Selectable likely does not have health in the first place. Might not even be possible to get here
		OutMissingRequirement = EAbilityRequirement::RequiresTargetWithHealth;
		return false;
	}
}

void AAbility_FinishingBlow::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	Target->TakeDamage(GetOutgoingDamage(), FDamageEvent(DamageType), nullptr, EffectInstigator);

	ShowTargetParticles(TargetAsSelectable);
	PlayTargetSound(Target, InstigatorsTeam);
}

void AAbility_FinishingBlow::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	ShowTargetParticles(TargetAsSelectable);
	PlayTargetSound(Target, InstigatorsTeam);
}

float AAbility_FinishingBlow::GetOutgoingDamage() const
{
	return Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
}

void AAbility_FinishingBlow::ShowTargetParticles(ISelectable * TargetAsSelectable)
{
	if (TargetParticles_Template != nullptr)
	{
		const FAttachInfo * AttachInfo = TargetAsSelectable->GetBodyLocationInfo(TargetParticles_AttachPoint);

		Statics::SpawnFogParticlesAttached(GS, TargetParticles_Template, AttachInfo->GetComponent(),
			AttachInfo->GetSocketName(), 0.f, AttachInfo->GetAttachTransform().GetLocation(),
			AttachInfo->GetAttachTransform().GetRotation().Rotator(),
			AttachInfo->GetAttachTransform().GetScale3D());
	}
}

void AAbility_FinishingBlow::PlayTargetSound(AActor * Target, ETeam InstigatorsTeam)
{
	if (TargetSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetSound, Target->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}
}
