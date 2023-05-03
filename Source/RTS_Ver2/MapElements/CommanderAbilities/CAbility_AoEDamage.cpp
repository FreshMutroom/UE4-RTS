// Fill out your copyright notice in the Description page of Project Settings.

#include "CAbility_AoEDamage.h"

#include "Statics/DamageTypes.h"


UCAbility_AoEDamage::UCAbility_AoEDamage()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::True;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::True;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	Radius = 1000.f;
	DealDamageDelay = 0.f;
	bCanHitEnemies = true;
	bCanHitFriendlies = true;
	bCanHitFlying = false;
	TargetLocationParticles = nullptr;
	TargetLocationParticlesScale = 1.f;
}

void UCAbility_AoEDamage::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	Super::FinalSetup(GameInst, GameState, LocalPlayersPlayerState);

	CheckCurveAssets();
}

void UCAbility_AoEDamage::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	ShowTargetLocationParticles(Location);

	if (DealDamageDelay > 0.f)
	{
		DealDamageAfterDelay(Location, InstigatorsTeam);
	}
	else
	{
		DealDamage(Location, InstigatorsTeam);
	}
}

void UCAbility_AoEDamage::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	ShowTargetLocationParticles(Location);
}

void UCAbility_AoEDamage::DealDamageAfterDelay(const FVector & Location, ETeam InstigatorsTeam)
{
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCAbility_AoEDamage, DealDamage), Location, InstigatorsTeam);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, DealDamageDelay, false);
}

void UCAbility_AoEDamage::DealDamage(const FVector & TargetLocation, ETeam InstigatorsTeam)
{
	/* Build the query params. Because this object is used by many different players it 
	cannot be created once and used forever */
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
	Statics::CapsuleSweep(GetWorld(), HitResults, TargetLocation, QueryParams, Radius);

	for (const auto & Elem : HitResults)
	{
		AActor * HitActor = Elem.GetActor();

		// Validity check needed here or has engine already done it? Maybe not since it's a weak pointer

		if (bCanHitFlying || Statics::IsAirUnit(HitActor) == false)
		{
			HitActor->TakeDamage(CalculateDamage(TargetLocation, HitActor), FDamageEvent(DamageInfo.AoEDamageType), nullptr, nullptr);
		}
	}
}

float UCAbility_AoEDamage::CalculateDamage(const FVector & TargetLocation, AActor * HitActor) const
{
	float OutgoingDamage = DamageInfo.AoEDamage;

	if (DamageFalloffCurve != nullptr)
	{
		// Maybe want to consider measuring to bounds instead of actor's location?
		const float DistanceFromTargetLocation = Statics::GetDistance2D(TargetLocation, HitActor->GetActorLocation());

		// Apply distance falloff
		OutgoingDamage *= DamageFalloffCurve->GetFloatValue(DistanceFromTargetLocation / Radius);
	}

	// Apply randomness
	OutgoingDamage *= FMath::RandRange(1.f - DamageInfo.AoERandomDamageFactor, 1.f + DamageInfo.AoERandomDamageFactor);

	return OutgoingDamage;
}

void UCAbility_AoEDamage::ShowTargetLocationParticles(const FVector & TargetLocation)
{
	if (TargetLocationParticles != nullptr)
	{
		Statics::SpawnFogParticles(TargetLocationParticles, GS, TargetLocation, FRotator::ZeroRotator, 
			FVector(TargetLocationParticlesScale));
	}
}

void UCAbility_AoEDamage::CheckCurveAssets()
{
	if (DamageFalloffCurve != nullptr)
	{
		float Min_X, Max_X, Min_Y, Max_Y;
		DamageFalloffCurve->GetTimeRange(Min_X, Max_X);
		DamageFalloffCurve->GetValueRange(Min_Y, Max_Y);

		// I guess we could let Max_Y go higher than 1 meaning the ability deals more damage the further the hit actors are from the use location 
		if (Min_X != 0.f || Max_X != 1.f || Min_Y != 0.f || Max_Y != 1.f)
		{
			// Curve is not usable. Null it so it won't be used
			DamageFalloffCurve = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("Commander ability object [%s]'s DamageFalloffCurve is not "
				"usable because each axis does not start at 0 and end at 1. It will not be used this "
				"play session"), *GetClass()->GetName());
		}
	}
}

void UCAbility_AoEDamage::Delay(FTimerHandle & TimerHandle, void(UCAbility_AoEDamage::* Function)(), float DelayAmount)
{
	assert(DelayAmount > 0.f);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, Function, DelayAmount, false);
}
