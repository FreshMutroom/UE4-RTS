// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_OngoingAoEDamage.h"


AAbility_OngoingAoEDamage::AAbility_OngoingAoEDamage()
{
	// Begin AAbilityBase variables
	bHasMultipleOutcomes =			EUninitializableBool::False;
	bCallAoEStartFunction =			EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	EUninitializableBool::False;
	bRequiresTargetOtherThanSelf =	EUninitializableBool::False;
	bRequiresLocation =				EUninitializableBool::False;
	bHasRandomBehavior =			EUninitializableBool::False;
	bRequiresTickCount =			EUninitializableBool::False;
	// End AAbilityBase variables

	NumTicks = 5;
	TimeBetweenTicks = 1.f;
	Radius = 500.f;
	BaseDamagePerTick = 15.f;
	DamageType = UDamageType_Default::StaticClass();
	bCanHitEnemies = true;
	BodySpawnLocation = ESelectableBodySocket::None;
}

void AAbility_OngoingAoEDamage::BeginPlay()
{
	Super::BeginPlay();

	CheckCurves();

	// Reserve a good chunk
	ActiveEffectInstances.Reserve(32);
}

void AAbility_OngoingAoEDamage::CheckCurves()
{
}

void AAbility_OngoingAoEDamage::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	/* It's ok if this overflows at some point as long as previous IDs are not still in use. 
	Highly unlikely 4 billion+ of these abilities will be going on at the same time so should 
	be fine. */
	static uint32 UniqueID = 0;

	SpawnEpicenterParticles(EffectInstigator, InstigatorAsSelectable);
	PlayEpicenterSound(EffectInstigator, InstigatorsTeam);

	if (NumTicks > 1)
	{
		ActiveEffectInstances.Emplace(++UniqueID, FOngoingAoEDamageInstance(EffectInstigator, NumTicks, InstigatorsTeam));
	}

	if (FirstTickDelay > 0.f)
	{
		DoInitialDamageTickAfterDelay(EffectInstigator, InstigatorsTeam, UniqueID);
	}
	else
	{
		// Do first damage tick now
		DoDamageTick(EffectInstigator, InstigatorsTeam, UniqueID);
	}
}

void AAbility_OngoingAoEDamage::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	SpawnEpicenterParticles(EffectInstigator, InstigatorAsSelectable);
	PlayEpicenterSound(EffectInstigator, InstigatorsTeam);
}

void AAbility_OngoingAoEDamage::SpawnEpicenterParticles(AActor * AbilityInstigator, ISelectable * InstigatorAsSelectable)
{
	if (TargetLocationParticles != nullptr)
	{
		if (bAttachesToInstigator)
		{
			const FAttachInfo * AttachInfo = InstigatorAsSelectable->GetBodyLocationInfo(BodySpawnLocation);
			
			Statics::SpawnFogParticlesAttached(GS, TargetLocationParticles, AttachInfo->GetComponent(),
				AttachInfo->GetSocketName(), 0.f, AttachInfo->GetAttachTransform().GetLocation(),
				AttachInfo->GetAttachTransform().GetRotation().Rotator(),
				AttachInfo->GetAttachTransform().GetScale3D());
		}
		else
		{
			Statics::SpawnFogParticles(TargetLocationParticles, GS, AbilityInstigator->GetActorLocation(), AbilityInstigator->GetActorRotation(), FVector::OneVector);
		}
	}
}

void AAbility_OngoingAoEDamage::PlayEpicenterSound(AActor * AbilityInstigator, ETeam InstigatorsTeam)
{
	if (TargetLocationSound != nullptr)
	{
		if (bAttachesToInstigator)
		{
			Statics::SpawnSoundAttached(TargetLocationSound, AbilityInstigator->GetRootComponent(),
				ESoundFogRules::Dynamic);
		}
		else
		{
			Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetLocationSound, AbilityInstigator->GetActorLocation(),
				ESoundFogRules::DecideOnSpawn);
		}
	}
}

void AAbility_OngoingAoEDamage::DoDamageTick(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID)
{
	SERVER_CHECK;

	// Build query params. Because this actor is shared by many teams we can't just create it once and forget about it
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

	FVector Location;
	if (!bAttachesToInstigator && NumTicks > 1)
	{
		Location = ActiveEffectInstances[UniqueID].GetAbilityUseLocation();
	}
	else
	{
		Location = AbilityInstigator->GetActorLocation();
	}

	TArray <FHitResult> HitResults;
	Statics::CapsuleSweep(GetWorld(), HitResults, Location, QueryParams, Radius);

	const bool bCheckIfSelf = (bCanHitFriendlies && !bCanHitSelf);

	for (const auto & Elem : HitResults)
	{
		AActor * HitActor = Elem.GetActor();

		// Validity check needed here or has engine already done it? Maybe not since it's a weak pointer

		if (bCanHitFlying || Statics::IsAirUnit(HitActor) == false)
		{
			if (!bCheckIfSelf || HitActor != AbilityInstigator)
			{
				HitActor->TakeDamage(CalculateDamage(Location, HitActor), FDamageEvent(DamageType), nullptr, AbilityInstigator);
			}
		}
	}

	if (NumTicks > 1)
	{
		FOngoingAoEDamageInstance & AbilityStateInfo = ActiveEffectInstances[UniqueID];
		AbilityStateInfo.DecrementNumTicksRemaining();
		if (AbilityStateInfo.GetNumTicksRemaining() == 0)
		{
			// Do this remove operation because it actually removes the pair, not just the value, so memory usage stays low, or does Remove do that anyway?
			ActiveEffectInstances.FindAndRemoveChecked(UniqueID);

			// Clear timer because it's looping and would otherwise call DoDamageTick again soon
			GetWorldTimerManager().ClearTimer(AbilityStateInfo.GetTimerHandle());
		}
		else
		{
			DoDamageTickAfterDelay(AbilityInstigator, InstigatorsTeam, UniqueID, TimeBetweenTicks);
		}
	}
}

void AAbility_OngoingAoEDamage::DoInitialDamageTickAfterDelay(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID)
{
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;
	static const FName FuncName = GET_FUNCTION_NAME_CHECKED(AAbility_OngoingAoEDamage, DoDamageTick);
	TimerDel.BindUFunction(this, FuncName, AbilityInstigator, InstigatorsTeam, UniqueID);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, FirstTickDelay, false);
}

void AAbility_OngoingAoEDamage::DoDamageTickAfterDelay(AActor * AbilityInstigator, ETeam InstigatorsTeam, uint32 UniqueID, float DelayAmount)
{
	assert(DelayAmount > 0.f);

	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;
	static const FName FuncName = GET_FUNCTION_NAME_CHECKED(AAbility_OngoingAoEDamage, DoDamageTick);
	TimerDel.BindUFunction(this, FuncName, AbilityInstigator, InstigatorsTeam, UniqueID);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, DelayAmount, false);

	// Timer handle not looping, perhaps it could be to make it more accurate and be resilient to large hitches
}

float AAbility_OngoingAoEDamage::CalculateDamage(const FVector & Location, AActor * HitActor) const
{
	if (DamageFalloffCurve == nullptr)
	{
		// No damage falloff
		return BaseDamagePerTick * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
	}
	else
	{
		const float Distance = Statics::GetDistance2D(Location, HitActor->GetActorLocation());
		const float DistanceFalloffMultiplier = DamageFalloffCurve->GetFloatValue(Distance / Radius);

		return BaseDamagePerTick * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor) * DistanceFalloffMultiplier;
	}
}
