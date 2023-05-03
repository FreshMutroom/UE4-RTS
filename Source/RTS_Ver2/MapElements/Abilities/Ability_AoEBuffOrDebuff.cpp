// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_AoEBuffOrDebuff.h"

// Might be able to comment some of these headers below
#include "Statics/Statics.h"
#include "GameFramework/Selectable.h"


AAbility_AoEBuffOrDebuff::AAbility_AoEBuffOrDebuff()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes = EUninitializableBool::True; // Depending on the buff/debuff being applied this isn't always true but we'll flag it as true anyway
	bCallAoEStartFunction = EUninitializableBool::True;
	bRequiresTargetOtherThanSelf = EUninitializableBool::False;
	bRequiresLocation = EUninitializableBool::True;
	bHasRandomBehavior = EUninitializableBool::False;
	//~ End AAbilityBase interface

	Radius = 500.f;
	bCanHitEnemies = true;
}

void AAbility_AoEBuffOrDebuff::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	// Will not call Super because this ability has no state/randomness

	/* Because this is an info actor and is shared by everyone we need to set the queried
	collision channls based on the user's team.
	Optionally could create every teams collision query params struct on BeginPlay and cache them
	then if that is faster */
	FCollisionObjectQueryParams QueryParams;
	if (bCanHitEnemies)
	{
		for (const auto & EnemyChannel : GS->GetEnemyChannels(InstigatorsTeam))
		{
			QueryParams.AddObjectTypesToQuery(Statics::IntToCollisionChannel(EnemyChannel));
		}
	}
	if (bCanHitFriendlies)
	{
		QueryParams.AddObjectTypesToQuery(GS->GetTeamCollisionChannel(InstigatorsTeam));
	}

	TArray <FHitResult> HitResults;
	Statics::CapsuleSweep(GetWorld(), HitResults, Location, QueryParams, Radius);

	OutHits.Reserve(HitResults.Num()); // Worst case allocation scheme
	for (const auto & Elem : HitResults)
	{
		AActor * HitActor = Elem.GetActor();

		// Validity check needed here or has engine already done it? Maybe not since it's a weak pointer

		if (bCanHitFlying || Statics::IsAirUnit(HitActor) == false)
		{
			ISelectable * AsSelectable = CastChecked<ISelectable>(HitActor);

			EAbilityOutcome Outcome;
			if (bUseStaticTypeBuffOrDebuff)
			{
				Outcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator,
					InstigatorAsSelectable, TargetAsSelectable, StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
			} 
			else
			{
				Outcome = static_cast<EAbilityOutcome>(Statics::Server_TryApplyBuffOrDebuff(EffectInstigator,
					InstigatorAsSelectable, TargetAsSelectable, TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI()));
			}

			OutHits.Emplace(FAbilityHitWithOutcome(AsSelectable, Outcome));
		}
	}
}

void AAbility_AoEBuffOrDebuff::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;
	
	/* Very important: iterate this array in the same way the elements were added, which looking 
	at Server_BeginAoE is forwards */
	for (int32 i = 0; i < Hits.Num(); ++i)
	{
		const FHitActorAndOutcome & Elem = Hits[i];
		
		AActor * HitActor = Elem.GetSelectable();
		ISelectable * AsSelectable = CastChecked<ISelectable>(HitActor);

		if (bUseStaticTypeBuffOrDebuff)
		{
			Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable,
				AsSelectable, StaticBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(),
				static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
		}
		else
		{
			Statics::Client_ApplyBuffOrDebuffGivenOutcome(EffectInstigator, InstigatorAsSelectable,
				AsSelectable, TickableBuffOrDebuffType, InstigatorAsSelectable->Selectable_GetGI(),
				static_cast<EBuffOrDebuffApplicationOutcome>(Outcome));
		}
	}
}
