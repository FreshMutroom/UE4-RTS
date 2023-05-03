// Fill out your copyright notice in the Description page of Project Settings.

#include "BuffAndDebuffManager.h"
#include "GameFramework/Actor.h" // Needed because NoExportTypes.h not included in .h 

#include "Statics/DamageTypes.h"
#include "Statics/Statics.h"
#include "GameFramework/Selectable.h"
#include "Statics/Structs_2.h"


ABuffAndDebuffManager::ABuffAndDebuffManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABuffAndDebuffManager::Tick(float DeltaTime)
{
#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
	/* We could do all this ticking logic in AInfantry::Tick and ABuilding::Tick. There's no 
	major problems with it. One minor "problem" is that if a buff/debuff is applied during 
	a tick then there's a certain chance that it will tick that tick and a chance it will tick 
	during the next tick instead. I don't know though, this doesn't really seem like much 
	of a problem and may actually help with reducing the number of buff/debuffs that tick 
	on certain frames */
	
	/* This only needs to tick on server. We replicate what happens to each client */
	SERVER_CHECK;
	
	Super::Tick(DeltaTime);

#if TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

	for (int32 i = TickableBuffsAndDebuffs.Num() - 1; i >= 0; --i)
	{
		TickSingleBuffOrDebuff(TickableBuffsAndDebuffs[i], DeltaTime);
	}
	
#endif // TICKABLE_BUFF_AND_DEBUFF_ENABLED_GAME

#endif // WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
}

#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
void ABuffAndDebuffManager::TickSingleBuffOrDebuff(FTickableBuffOrDebuffInstanceInfo & InstanceInfo, float DeltaTime)
{
	InstanceInfo.DecrementDeltaTime(DeltaTime);

	const FTickableBuffOrDebuffInfo * Info = InstanceInfo.GetInfoStruct();
	const float TickInterval = Info->GetTickInterval();

	/* Going to while statement here instead of just if statement so we can do multiple ticks
	for extremly fast tick rate buffs/debuffs */
	uint8 NumTicksToExecute = 0;
	while (InstanceInfo.GetTimeRemainingTillNextTick() <= 0.f)
	{
		NumTicksToExecute++;
		InstanceInfo.IncreaseTimeRemainingTillNextTick(TickInterval);

		/* This break means we can only tick a buff/debuff a maximum of once during this function. 
		Remove it to allow more ticks to happen, but then we also have to send 'how many ticks 
		happened' to clients too in the RPC. As long as buffs/debuffs don't tick too fast then 
		this shouldn't matter. */
		break;
	}

	const bool bExpiresOverTime = Info->ExpiresOverTime();
	bool bUpdatedUI = false;

	for (uint8 k = 0; k < NumTicksToExecute; ++k)
	{
		InstanceInfo.IncrementTickCount();

		const EBuffOrDebuffTickOutcome TickOutcome = Info->ExecuteDoTickBehavior(SomePtrToTheSelectableThisIsAppliedTo, Elem);

		/* Note down this tick happened so we can multicast it */
		a;

		/* Check if buff/debuff has 'ticked out' */
		if (bExpiresOverTime && Elem.GetTickCount() == Info->GetNumberOfTicks())
		{
			/* Will avoid RemoveAtSwap to preserve ordering since it will look strange
			if buffs/debuffs are swapping location on HUD */
			SomePtrToTheSelectableThisIsAppliedTo.;

			const EBuffOrDebuffRemovalOutcome RemovalOutcome = Info->ExecuteOnRemovedBehavior(SomePtrToTheSelectableThisIsAppliedTo,
				Elem, nullptr, nullptr, EBuffAndDebuffRemovalReason::Expired);

			/* Note down removal happened so we can multicast it. We'll have to send 1 multicast 
			for both ticks and removals to keep ordering intact */
			a;

			a;// TODO UI updating
		}
	}
}

#endif


//============================================================================================
//	              -------- Dash ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::Dash_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	/* Do not apply if target has zero health. Don't actually have to do this check. */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	/* Get data that tells us whether the buff is already applied */
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::Dash);
	const bool bAlreadyHasThisBuff = (Info != nullptr);
	
	if (bAlreadyHasThisBuff)
	{
		/* Reset duration back to full */
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		/* Put in the selectable's buff/debuff container and set duration */
		Target->RegisterBuff(ETickableBuffAndDebuffType::Dash, BuffOrDebuffInstigator, InstigatorAsSelectable);

		/* Apply a 50% move speed increase. Important we checked whether the buff was already
		applied above otherwise we would increase move speed by even more */
		Target->ApplyTempMoveSpeedMultiplier(1.5f);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::Dash_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::Dash_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	/* Remove the 50% speed increase */
	Target->RemoveTempMoveSpeedMultiplier(1.f / 1.5f);

	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- Haste ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::Haste_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{	
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::Haste);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		/* Reset duration back to full */
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::Haste, BuffOrDebuffInstigator, InstigatorAsSelectable);

		Target->ApplyTempMoveSpeedMultiplier(1.3f);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::Haste_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::Haste_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	/* Remove the 30% move speed increase */
	Target->RemoveTempMoveSpeedMultiplier(1.f / 1.3f);

	return EBuffOrDebuffRemovalOutcome::Success;
}


//============================================================================================
//	              -------- BasicHealOverTime ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::BasicHealOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	/* Do not apply if target has zero health */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	/* Check whether target already has this buff */
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::BasicHealOverTime);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		/* If the target already has this buff applied to them then the behavior we do is tick it
		straight away and then reset the duration */
		Target->TakeDamageSelectable(10.f, FDamageEvent(UDamageType_BiologicalHeal::StaticClass()),
			nullptr, BuffOrDebuffInstigator);

		/* Reset duration back to full */
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::BasicHealOverTime, BuffOrDebuffInstigator, 
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::BasicHealOverTime_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	/* Apply 10 points of 'heal' type damage */
	Target->TakeDamageSelectable(10.f, FDamageEvent(UDamageType_BiologicalHeal::StaticClass()), 
		nullptr, StateInfo.GetInstigator());

	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::BasicHealOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- IncreasingHealOverTime ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::IncreasingHealOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	/* Do not apply if target has zero health */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	/* Check whether target already has this buff */
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::IncreasingHealOverTime);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		const uint8 NumTicksAlreadyCompleted = Info->GetTickCount();

		/* Heal target right now for what the next tick would have healed for */
		Target->TakeDamageSelectable(5.f * (NumTicksAlreadyCompleted + 1), FDamageEvent(UDamageType_BiologicalHeal::StaticClass()),
			nullptr, BuffOrDebuffInstigator);

		/* Reset duration */
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::IncreasingHealOverTime, BuffOrDebuffInstigator, 
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::IncreasingHealOverTime_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	/* Heal target. Amount increases each tick */
	Target->TakeDamageSelectable(5.f * StateInfo.GetTickCount(), FDamageEvent(UDamageType_BiologicalHeal::StaticClass()),
		nullptr, StateInfo.GetInstigator());

	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::IncreasingHealOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- ThePlague ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::ThePlague_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Static)
{
	/* Do not apply if target has zero health */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	FStaticBuffOrDebuffInstanceInfo * Info = Target->GetDebuffState(EStaticBuffAndDebuffType::ThePlague);
	const bool bAlreadyHasThisDebuff = (Info != nullptr);

	if (bAlreadyHasThisDebuff)
	{
		return EBuffOrDebuffApplicationOutcome::AlreadyHasIt;
	}
	else
	{
		Target->RegisterDebuff(EStaticBuffAndDebuffType::ThePlague, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		/* Increase damage taken by 20% */
		Target->GetAttributesModifiable()->ApplyTempDefenseMultiplierViaMultiplier(1.2f, Target);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::ThePlague_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Static)
{
	/* Remove the extra 20% received damage effect */
	Target->GetAttributesModifiable()->RemoveTempDefenseMultiplierViaMultiplier(1.f / 1.2f, Target);

	/* If removed via the cleanse spell then we apply a temporary buff to the unit that 
	removed it */
	if (RemovalReason == EBuffAndDebuffRemovalReason::CleanseSpell)
	{
		if (Statics::IsValid(Remover))
		{
			const EBuffOrDebuffApplicationOutcome ApplicationResult = Statics::Server_TryApplyBuffOrDebuff(Remover,
				RemoverAsSelectable, RemoverAsSelectable, ETickableBuffAndDebuffType::CleansersMight, 
				RemoverAsSelectable->Selectable_GetGI());

			switch (ApplicationResult)
			{
			case EBuffOrDebuffApplicationOutcome::ResetDuration:
				return EBuffOrDebuffRemovalOutcome::Cleanse_ResetDurationOfCleansersMight;
				
			case EBuffOrDebuffApplicationOutcome::Success:
				return EBuffOrDebuffRemovalOutcome::Cleanse_FreshApplicationOfCleansersMight;
			}
		}
	}

	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- CleansersMightOrWhatever ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::CleansersMightOrWhatever_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	/* Do not apply if target has zero health */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::CleansersMight);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		/* Reset buff duration */
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		/* Increase attack speed by 50%. Here we have assumed the remover has attack attributes */
		Target->GetAttackAttributesModifiable()->ApplyTempAttackRateModifierViaMultiplier(1.f / 1.5f, Target);

		/* Increase move speed by 50% */
		Target->ApplyTempMoveSpeedMultiplier(1.5f);

		/* Put in the selectable's buff/debuff container and set duration */
		Target->RegisterBuff(ETickableBuffAndDebuffType::CleansersMight, BuffOrDebuffInstigator, 
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::CleansersMightOrWhatever_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::CleansersMightOrWhatever_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	/* Remove the 50% attack speed increase */
	Target->GetAttackAttributesModifiable()->RemoveTempAttackRateModifierViaMultiplier(1.5f, Target);

	/* Remove the 50% move speed increase */
	Target->RemoveTempMoveSpeedMultiplier(1.f / 1.5f);

	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- PainOverTime ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::PainOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	/* Do not apply if target has zero health */
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetDebuffState(ETickableBuffAndDebuffType::PainOverTime);
	const bool bAlreadyHasThisDebuff = (Info != nullptr);

	if (bAlreadyHasThisDebuff)
	{
		/* If the target already has pain debuff then apply a tick's worth of damage instantly */
		Target->TakeDamageSelectable(15.f, FDamageEvent(UDamageType_Shadow::StaticClass()), nullptr,
			BuffOrDebuffInstigator);

		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		/* Put in the selectable's buff/debuff container and set duration */
		Target->RegisterDebuff(ETickableBuffAndDebuffType::PainOverTime, BuffOrDebuffInstigator, 
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::PainOverTime_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	/* Deal 15 shadow damage */
	Target->TakeDamageSelectable(15.f, FDamageEvent(UDamageType_Shadow::StaticClass()), nullptr, 
		StateInfo.GetInstigator());

	/* If target died from that then Pain_OnRemoved will be called here with the param 
	"RemovalReason" equal to EBuffAndDebuffRemovalReason::TargetDied */

	/* Check if we killed target with that tick. 
	If the logic you want is "do something when this debuff kills target" then do what I have 
	done here. But if you want the logic "do something when target dies with this debuff on them 
	regardless of whether it was the reason they died or not" then don't put logic here and instead 
	put logic in Pain_OnRemoved and branch on ReasonForRemoval == TargetDied */
	if (Target->HasZeroHealth())
	{
		AActor * DebuffInstigator = StateInfo.GetInstigator();
		if (Statics::IsValid(DebuffInstigator) && !Statics::HasZeroHealth(DebuffInstigator))
		{
			/* Heal the debuff instigator */
			DebuffInstigator->TakeDamage(50.f, FDamageEvent(UDamageType_BiologicalHeal::StaticClass()),
				nullptr, DebuffInstigator);

			return EBuffOrDebuffTickOutcome::PainOverTime_KilledTargetAndGotHeal;
		}

		return EBuffOrDebuffTickOutcome::PainOverTime_KilledTargetButNoHeal;
	}

	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::PainOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- Corruption ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::Corruption_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetDebuffState(ETickableBuffAndDebuffType::Corruption);
	const bool bAlreadyHasThisDebuff = (Info != nullptr);

	if (bAlreadyHasThisDebuff)
	{
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterDebuff(ETickableBuffAndDebuffType::Corruption, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::Corruption_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	/* Deal 5 shadow damage */
	Target->TakeDamageSelectable(5.f, FDamageEvent(UDamageType_Shadow::StaticClass()), nullptr,
		StateInfo.GetInstigator());

	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::Corruption_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- NearInvulnerability ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::NearInvulnerability_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::NearInvulnerability);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::NearInvulnerability, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		/* Reduce all incoming damage by 99% */
		Target->GetAttributesModifiable()->ApplyTempDefenseMultiplierViaMultiplier(0.01f, Target);

		Target->AttachParticles(BuffOrDebuffInfo);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::NearInvulnerability_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::NearInvulnerability_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	/* Remove the 99% damage reduction */
	Target->GetAttributesModifiable()->RemoveTempDefenseMultiplierViaMultiplier(1.f / 0.01f, Target);

	Target->RemoveAttachedParticles(BuffOrDebuffInfo);

	return EBuffOrDebuffRemovalOutcome::Success;
}

//============================================================================================
//	              -------- TempStealth ---------

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::TempStealth_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}
	
	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::TempStealth);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::TempStealth, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		Target->ApplyTempStealthModeEffect();

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::TempStealth_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::TempStealth_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	Target->RemoveTempStealthModeEffect();

	return EBuffOrDebuffRemovalOutcome::Success;
}

EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::EatPumpkinEffect_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::RottenPumpkinEatEffect);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::RottenPumpkinEatEffect, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::EatPumpkinEffect_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	// Heal 10% of max health
	const float DamageAmount = Target->GetAttributes()->GetMaxHealth() / 10.f; 
	Target->TakeDamageSelectable(DamageAmount, FDamageEvent(UDamageType_BiologicalHeal::StaticClass()),
		nullptr, StateInfo.GetInstigator());

	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::EatPumpkinEffect_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	return EBuffOrDebuffRemovalOutcome::Success;
}


EBuffOrDebuffApplicationOutcome ABuffAndDebuffManager::Beserk_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable)
{
	if (Target->HasZeroHealth())
	{
		return EBuffOrDebuffApplicationOutcome::Failure;
	}

	FTickableBuffOrDebuffInstanceInfo * Info = Target->GetBuffState(ETickableBuffAndDebuffType::Beserk);
	const bool bAlreadyHasThisBuff = (Info != nullptr);

	if (bAlreadyHasThisBuff)
	{
		Info->ResetDuration();

		return EBuffOrDebuffApplicationOutcome::ResetDuration;
	}
	else
	{
		Target->RegisterBuff(ETickableBuffAndDebuffType::Beserk, BuffOrDebuffInstigator,
			InstigatorAsSelectable);

		FAttackAttributes * AttackAttributes = Target->GetAttackAttributesModifiable();
		// Increase outgoing damage by 50%
		AttackAttributes->ApplyTempImpactDamageModifierViaMultiplier(1.5f, Target);

		// Increase attack speed by 50%
		AttackAttributes->ApplyTempAttackRateModifierViaMultiplier(1.f / 1.5f, Target);

		// Increase movement speed by 30%
		Target->ApplyTempMoveSpeedMultiplier(1.3f);

		// Show particles
		Target->AttachParticles(BuffOrDebuffInfo);
		
		return EBuffOrDebuffApplicationOutcome::Success;
	}
}

EBuffOrDebuffTickOutcome ABuffAndDebuffManager::Beserk_DoTick(FUNCTION_SIGNATURE_DoTick)
{
	return EBuffOrDebuffTickOutcome::StandardOutcome;
}

EBuffOrDebuffRemovalOutcome ABuffAndDebuffManager::Beserk_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable)
{
	// Remove attribute changes
	FAttackAttributes * AttackAttributes = Target->GetAttackAttributesModifiable();
	AttackAttributes->RemoveTempImpactDamageModifierViaMultiplier(1.f / 1.5f, Target);
	AttackAttributes->RemoveTempAttackRateModifierViaMultiplier(1.5f, Target);
	Target->RemoveTempMoveSpeedMultiplier(1.f / 1.3f);

	// Hide particles
	Target->RemoveAttachedParticles(BuffOrDebuffInfo);

	return EBuffOrDebuffRemovalOutcome::Success;
}
