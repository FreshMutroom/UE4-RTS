// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_AoEDamage.h"


AAbility_AoEDamage::AAbility_AoEDamage()
{
	// Begin AAbilityBase variables
	bHasMultipleOutcomes = EUninitializableBool::False;
	bCallAoEStartFunction = EUninitializableBool::False; // Client doesn't need to know who is hit; all damage is done on server only
	bAoEHitsHaveMultipleOutcomes = EUninitializableBool::False;
	bRequiresTargetOtherThanSelf = EUninitializableBool::False;
	bRequiresLocation = EUninitializableBool::True;
	bHasRandomBehavior = EUninitializableBool::False;
	bRequiresTickCount = EUninitializableBool::False;
	// End AAbilityBase variables

	Radius = 500.f;
	BaseDamage = 50.f;
	DamageType = UDamageType_Default::StaticClass();
	bCanHitEnemies = true;
}

void AAbility_AoEDamage::BeginPlay()
{
	Super::BeginPlay();

	CheckCurves();
}

void AAbility_AoEDamage::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	if (DamageDelay > 0.f)
	{
		DealDamageAfterDelay(EffectInstigator, Location, InstigatorsTeam, DamageDelay);
	}
	else
	{
		// Deal damage now
		DealDamage(EffectInstigator, Location, InstigatorsTeam);
	}

	ShowTargetLocationParticles(EffectInstigator, Location);
	PlayTargetLocationSound(Location, InstigatorsTeam);
}

void AAbility_AoEDamage::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	ShowTargetLocationParticles(EffectInstigator, Location);
	PlayTargetLocationSound(Location, InstigatorsTeam);
}

void AAbility_AoEDamage::CheckCurves()
{
}

void AAbility_AoEDamage::DealDamage(AActor * AbilityInstigator, const FVector & Location, ETeam InstigatorsTeam)
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

	TArray <FHitResult> HitResults;
	Statics::CapsuleSweep(GetWorld(), HitResults, Location, QueryParams, Radius);

	for (const auto & Elem : HitResults)
	{
		AActor * HitActor = Elem.GetActor();

		// Validity check needed here or has engine already done it? Maybe not since it's a weak pointer

		if (bCanHitFlying || Statics::IsAirUnit(HitActor) == false)
		{
			HitActor->TakeDamage(CalculateDamage(Location, HitActor), FDamageEvent(DamageType), nullptr, AbilityInstigator);
		}
	}
}

void AAbility_AoEDamage::DealDamageAfterDelay(AActor * AbilityInstigator, const FVector & Location, ETeam InstigatorsTeam, float DelayAmount)
{
	assert(DelayAmount > 0.f);
	
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;
	static const FName FuncName = GET_FUNCTION_NAME_CHECKED(AAbility_AoEDamage, DealDamage);
	TimerDel.BindUFunction(this, FuncName, AbilityInstigator, Location, InstigatorsTeam);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, DelayAmount, false);
}

float AAbility_AoEDamage::CalculateDamage(const FVector & AbilityLocation, AActor * HitTarget) const
{	
	if (DamageFalloffCurve == nullptr)
	{
		// No damage falloff
		return BaseDamage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor);
	}
	else
	{
		const float Distance = Statics::GetDistance2D(AbilityLocation, HitTarget->GetActorLocation());
		const float DistanceFalloffMultiplier = DamageFalloffCurve->GetFloatValue(Distance / Radius);

		return BaseDamage * FMath::RandRange(1.f - RandomDamageFactor, 1.f + RandomDamageFactor) * DistanceFalloffMultiplier;
	}
}

void AAbility_AoEDamage::ShowTargetLocationParticles(AActor * AbilityInstigator, const FVector & Location)
{
	if (TargetLocationParticles != nullptr)
	{
		Statics::SpawnFogParticles(TargetLocationParticles, GS, Location, GetTargetLocationParticlesRotation(AbilityInstigator), FVector::OneVector);
	}
}

FRotator AAbility_AoEDamage::GetTargetLocationParticlesRotation(AActor * AbilityInstigator) const
{
	return AbilityInstigator->GetActorRotation();
}

void AAbility_AoEDamage::PlayTargetLocationSound(const FVector & Location, ETeam InstigatorsTeam)
{
	if (TargetLocationSound != nullptr)
	{
		Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetLocationSound, Location, ESoundFogRules::DecideOnSpawn);
	}
}
