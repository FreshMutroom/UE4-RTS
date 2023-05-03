// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_BasicSingleTargetDamage.h"


AAbility_BasicSingleTargetDamage::AAbility_BasicSingleTargetDamage()
{
	//~ Begin AAbilityBase interface
	bCallAoEStartFunction = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::True;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
	bHasMultipleOutcomes = EUninitializableBool::False;
	//~ End AAbilityBase interface

	Damage = 120.f;
	DamageType = UDamageType_Default::StaticClass();
	RandomDamageFactor = 0.f;
	TargetParticles_AttachPoint = ESelectableBodySocket::None;
}

void AAbility_BasicSingleTargetDamage::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;
	
	const float FinalOutgoingDamage = Damage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
	
	Target->TakeDamage(FinalOutgoingDamage, FDamageEvent(DamageType), nullptr, EffectInstigator);

	ShowTargetParticles(TargetAsSelectable);
	PlayTargetSound(Target, InstigatorsTeam);
}

void AAbility_BasicSingleTargetDamage::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;
	
	/* Damage is just done on the server and will replicate so nothing needed to be done */

	// Check because networking
	if (Statics::IsValid(Target))
	{
		ShowTargetParticles(TargetAsSelectable);
		PlayTargetSound(Target, InstigatorsTeam);
	}
}

void AAbility_BasicSingleTargetDamage::ShowTargetParticles(ISelectable * TargetAsSelectable)
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

void AAbility_BasicSingleTargetDamage::PlayTargetSound(AActor * Target, ETeam InstigatorsTeam)
{
	if (TargetSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetSound, Target->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}
}
