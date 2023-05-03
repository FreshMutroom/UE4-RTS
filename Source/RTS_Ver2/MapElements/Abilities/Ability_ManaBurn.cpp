// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_ManaBurn.h"


AAbility_ManaBurn::AAbility_ManaBurn()
{
	//~ Begin AAbilityBase interface
	bCallAoEStartFunction = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::True;
	bRequiresLocation = EUninitializableBool::False;
	bHasRandomBehavior = EUninitializableBool::False;
	bHasMultipleOutcomes = EUninitializableBool::False;
	bRequiresTickCount = EUninitializableBool::True;
	//~ End AAbilityBase interface

	SelectableResourceType = ESelectableResourceType::None;
	ResourceDrainAmount = 50;
	DamageDealingRule = EDamageDealingRule::PercentageOfSelectableResourceAmountDrained;
	DamagePercentage = 0.5f;
	AbsoluteDamageAmount = 50.f;
	DamageType = UDamageType_Default::StaticClass();
	RandomDamageFactor = 0.f;
	TargetParticles_AttachPoint = ESelectableBodySocket::None;
}

bool AAbility_ManaBurn::IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const
{
	/* Do not allow ability to be used on a target that does not have the selectable resource 
	e.g. if this burns mana then it cannot be used on selectables that don't have mana */
	if (Target->GetAttributes() != nullptr 
		&& Target->GetAttributes()->GetSelectableResource_1().GetType() == SelectableResourceType)
	{
		return true;
	}
	else
	{
		OutMissingRequirement = MissingResourceMessageType;
		return false;
	}
}

void AAbility_ManaBurn::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	// Already checked in IsUsable_TargetChecks that GetAttributes() returns non-null so do not need to check again here
	FSelectableResourceInfo & ResourceInfo = TargetAsSelectable->GetAttributesModifiable()->GetSelectableResource_1();

	const int32 PreChangeResourceAmount = ResourceInfo.GetAmount();

	// Drain selectable resource
	ResourceInfo.AdjustAmount(-ResourceDrainAmount, GS->GetGameTickCounter(), GS->GetGameTickCounter(),
		TargetAsSelectable, true);

	const int32 PostChangeResourceAmount = ResourceInfo.GetAmount();

	// Deal damage
	Target->TakeDamage(GetOutgoingDamage(PreChangeResourceAmount - PostChangeResourceAmount),
		FDamageEvent(DamageType), nullptr, EffectInstigator);

	// Visuals and audio
	ShowTargetParticles(TargetAsSelectable);
	PlayTargetSound(Target, InstigatorsTeam);
}

void AAbility_ManaBurn::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	FSelectableResourceInfo & ResourceInfo = TargetAsSelectable->GetAttributesModifiable()->GetSelectableResource_1();

	// Drain resource amount
	ResourceInfo.AdjustAmount(-ResourceDrainAmount, GS->GetGameTickCounter(), ServerTickCountAtTimeOfAbility,
		TargetAsSelectable, true);

	// Visuals and audio
	ShowTargetParticles(TargetAsSelectable);
	PlayTargetSound(Target, InstigatorsTeam);
}

float AAbility_ManaBurn::GetOutgoingDamage(int32 ResourceAmountDrained) const
{
	if (DamageDealingRule == EDamageDealingRule::PercentageOfSelectableResourceAmountDrained)
	{
		return FMath::RoundToFloat(ResourceAmountDrained * DamagePercentage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor));
	}
	else // Assumed AbsoluteAmount
	{
		assert(DamageDealingRule == EDamageDealingRule::AbsoluteAmount);

		return AbsoluteDamageAmount * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
	}
}

void AAbility_ManaBurn::ShowTargetParticles(ISelectable * TargetAsSelectable)
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

void AAbility_ManaBurn::PlayTargetSound(AActor * Target, ETeam InstigatorsTeam)
{
	if (TargetSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetSound, Target->GetActorLocation(),
			ESoundFogRules::DecideOnSpawn);
	}
}

#if WITH_EDITOR
void AAbility_ManaBurn::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	bCanEditMostProperties = (SelectableResourceType != ESelectableResourceType::None);
	bCanEditPercentageDamageProperties = (bCanEditMostProperties && DamageDealingRule == EDamageDealingRule::PercentageOfSelectableResourceAmountDrained);
	bCanEditAbsoluteDamageProperties = (DamageDealingRule == EDamageDealingRule::AbsoluteAmount);
}
#endif
