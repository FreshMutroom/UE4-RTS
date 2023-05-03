// Fill out your copyright notice in the Description page of Project Settings.

#include "CAbility_ArtilleryStrike.h"


UActiveArtilleryStrikeState::UActiveArtilleryStrikeState()
	: NumProjectilesRemainingInSalvo(0)
{
}

void UActiveArtilleryStrikeState::Init(UCAbility_ArtilleryStrike * InEffectObject, 
	AObjectPoolingManager * InPoolingManager, bool bInIsServer, ETeam InInstigatorsTeam,
	int32 InRandomNumberSeed, const FVector & UseLocation)
{
	EffectObject = InEffectObject;
	PoolingManager = InPoolingManager;
	AbilityLocation = UseLocation; 
	RandomStream = FRandomStream(InRandomNumberSeed);
	LastYawRot = GetRandomFloat(0.f, 360.f);
	bIsServer = bInIsServer;
	InstigatorsTeam = InInstigatorsTeam;

	TimeTillSpawnNextProjectile = GetRandomFloat(InEffectObject->MinInitialDelay, InEffectObject->MaxInitialDelay);
	TimeTillStop = TimeTillSpawnNextProjectile + GetRandomFloat(InEffectObject->MinDuration, InEffectObject->MaxDuration);
}

void UActiveArtilleryStrikeState::Tick(float DeltaTime)
{
	// Guard against continuing after the object is going to be GCed
	if (TimeTillStop <= 0.f)
	{
		return;
	}
	
	TimeTillStop -= DeltaTime;
	if (TimeTillStop <= 0.f)
	{
		/* What's happening here? Making the code resilient to hitches. 
		Say the ability has 1 sec left and we get a 10sec hitch. So TimeTillStop will be -9. 
		We wanna run 1 sec worth of spawning projectiles before stopping the ability so that 
		is what is happening here */

		float UnusedTime = TimeTillStop + DeltaTime;
		while (TimeTillSpawnNextProjectile <= 0.f && UnusedTime > 0.f)
		{
			const float CurrentTimeTillSpawnNextProjectile = TimeTillSpawnNextProjectile;
			SpawnProjectile();
			UnusedTime -= (TimeTillSpawnNextProjectile - CurrentTimeTillSpawnNextProjectile);
		}

		Stop();
	}
	else
	{
		TimeTillSpawnNextProjectile -= DeltaTime;
		while (TimeTillSpawnNextProjectile <= 0.f)
		{
			SpawnProjectile();
		}
	}
}

TStatId UActiveArtilleryStrikeState::GetStatId() const
{
	/* I just copied what I saw in engine source */
	RETURN_QUICK_DECLARE_CYCLE_STAT(UActiveArtilleryStrikeState, STATGROUP_Tickables);
}

ETickableTickType UActiveArtilleryStrikeState::GetTickableTickType() const
{
	// Stop CDO from ever ticking cause it will otherwise
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

int32 UActiveArtilleryStrikeState::GetRandomInt(int32 Min, int32 Max) const
{
	return UKismetMathLibrary::RandomIntegerInRangeFromStream(Min, Max, RandomStream);
}

float UActiveArtilleryStrikeState::GetRandomFloat(float Min, float Max) const
{
	return UKismetMathLibrary::RandomFloatInRangeFromStream(Min, Max, RandomStream);
}

void UActiveArtilleryStrikeState::SpawnProjectile()
{
	if (NumProjectilesRemainingInSalvo == 0)
	{
		// -1 to deduct the projectile we're about to spawn
		NumProjectilesRemainingInSalvo = GetRandomInt(EffectObject->MinShotsPerSalvo - 1, EffectObject->MaxShotsPerSalvo - 1);
	}
	else
	{
		NumProjectilesRemainingInSalvo--;
	}

	const FVector ProjectileSpawnLocation = CalculateProjectileSpawnLocation();

	// Give projectile a random roll
	const float ProjectileRoll = GetRandomFloat(0.f, 360.f);
	if (bIsServer)
	{
		PoolingManager->Server_FireProjectileAtLocation(nullptr, EffectObject->Projectile_BP, 
			EffectObject->ProjectileDamageValues, 0.f, InstigatorsTeam, ProjectileSpawnLocation,
			ProjectileSpawnLocation + FVector(0.f, 0.f, -EffectObject->ProjectileSpawnHeight), ProjectileRoll);
	}
	else
	{
		PoolingManager->Client_FireProjectileAtLocation(EffectObject->Projectile_BP,
			EffectObject->ProjectileDamageValues, 0.f, InstigatorsTeam, ProjectileSpawnLocation,
			ProjectileSpawnLocation + FVector(0.f, 0.f, -EffectObject->ProjectileSpawnHeight), ProjectileRoll);
	}

	TimeTillSpawnNextProjectile += CalculateTimeBetweenShots();
}

FVector UActiveArtilleryStrikeState::CalculateProjectileSpawnLocation()
{
	float DistanceFromCenter;
	if (EffectObject->DistanceFromCenterCurve == nullptr)
	{
		// Use linear
		DistanceFromCenter = GetRandomFloat(0.f, EffectObject->Radius);
	}
	else
	{
		DistanceFromCenter = EffectObject->DistanceFromCenterCurve->GetFloatValue(GetRandomFloat(0.f, 1.f)) * EffectObject->Radius;
	}

	float YawRotation;
	const float RandomRot = GetRandomFloat(EffectObject->MinRotation, 360.f - EffectObject->MinRotation);
	YawRotation = FMath::Fmod(RandomRot + LastYawRot, 360.f);

	// Remember this for next time. Note range is [0, 360] not [-180, 180]. Engine is ok with that right?
	LastYawRot = YawRotation;

	FVector Vector = FVector(1.f, 0.f, 0.f);
	Vector = Vector.RotateAngleAxis(YawRotation, FVector(0.f, 0.f, 1.f));
	Vector *= DistanceFromCenter;
	Vector += AbilityLocation;
	Vector.Z += EffectObject->ProjectileSpawnHeight;
	return Vector;
}

float UActiveArtilleryStrikeState::CalculateTimeBetweenShots() const
{
	if (NumProjectilesRemainingInSalvo == 0)
	{	
		// Last projectile fired was the last one of the salvo
		return GetRandomFloat(EffectObject->MinTimeBetweenSalvos, EffectObject->MaxTimeBetweenSalvos);
	}
	else
	{
		return GetRandomFloat(EffectObject->MinTimeBetweenShots, EffectObject->MaxTimeBetweenShots);
	}
}

void UActiveArtilleryStrikeState::Stop()
{
	EffectObject->OnActiveStrikeExpired(this);
}


//------------------------------------------------------------------------------------------
//==========================================================================================
//	------- UCAbility_ArtilleryStrike -------
//==========================================================================================
//------------------------------------------------------------------------------------------

UCAbility_ArtilleryStrike::UCAbility_ArtilleryStrike()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::True;
	bHasRandomBehavior =			ECommanderUninitializableBool::True;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	Radius = 1200.f;
	ProjectileSpawnHeight = 8000.f;
	ProjectileDamageValues.AoEDamage = 50.f;
	MinInitialDelay = 0.f;
	MaxInitialDelay = 0.f;
	MinDuration = 10.f;
	MaxDuration = 10.f;
	MinTimeBetweenSalvos = 0.5f;
	MaxTimeBetweenSalvos = 1.f;
	MinShotsPerSalvo = 2;
	MaxShotsPerSalvo = 4;
	MinTimeBetweenShots = 0.3f;
	MaxTimeBetweenShots = 0.6f;
	MinRotation = 30.f;
	ShowDecalDelay = 1.f;
}

void UCAbility_ArtilleryStrike::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	Super::FinalSetup(GameInst, GameState, LocalPlayersPlayerState);

	CheckCurveAssets();

	if (bOverrideProjectileDamageValues == false)
	{
		AProjectileBase * Projectile = GameState->GetObjectPoolingManager()->GetProjectileReference(Projectile_BP);
		ProjectileDamageValues.SetValuesFromProjectile(Projectile);
	}
}

void UCAbility_ArtilleryStrike::CheckCurveAssets()
{
	if (DistanceFromCenterCurve != nullptr)
	{
		float Min_X, Max_X;
		DistanceFromCenterCurve->GetTimeRange(Min_X, Max_X);
		if (Min_X != 0.f || Max_X != 1.f)
		{
			// Null it so it won't be used 
			DistanceFromCenterCurve = nullptr;

			UE_LOG(RTSLOG, Warning, TEXT("DistanceFromCenterCurve for artillery strike object [%s] "
				"is not usable because the X axis does not start at 0 and end at 1. It will not be used "
				"this play session"), *GetClass()->GetName());
		}
	}
}

void UCAbility_ArtilleryStrike::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;
	
	OutRandomNumberSeed = GenerateInitialRandomSeed();

	UActiveArtilleryStrikeState * NewStrike = NewObject<UActiveArtilleryStrikeState>();
	NewStrike->Init(this, GS->GetObjectPoolingManager(), true, InstigatorsTeam, 
		SeedAs16BitTo32Bit(OutRandomNumberSeed), Location);
	ActiveStrikes.Emplace(NewStrike);

	RevealFogAtUseLocation(Location, InstigatorsTeam, true);
	HandleSpawningOfDecal(Location, AbilityInstigator);
}

void UCAbility_ArtilleryStrike::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	UActiveArtilleryStrikeState * NewStrike = NewObject<UActiveArtilleryStrikeState>();
	NewStrike->Init(this, GS->GetObjectPoolingManager(), false, InstigatorsTeam,
		RandomNumberSeed, Location);
	ActiveStrikes.Emplace(NewStrike);

	RevealFogAtUseLocation(Location, InstigatorsTeam, false);
	HandleSpawningOfDecal(Location, AbilityInstigator);
}

void UCAbility_ArtilleryStrike::RevealFogAtUseLocation(const FVector & AbilityLocation, ETeam InstigatorsTeam, bool bIsServer)
{
#if GAME_THREAD_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(GS->GetFogManager(), FVector2D(AbilityLocation),
		InstigatorsTeam, bIsServer, PS->GetTeam() == InstigatorsTeam);

#elif MULTITHREADED_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(FVector2D(TargetLocation),
		InstigatorsTeam, bIsServer, PS->GetTeam() == InstigatorsTeam);

#endif
}

void UCAbility_ArtilleryStrike::HandleSpawningOfDecal(const FVector & AbilityLocation, ARTSPlayerState * AbilityInstigator)
{
	if (TargetLocationDecalInfo.GetDecal() != nullptr)
	{
		if (ShouldSeeDecal(AbilityInstigator))
		{
			if (ShowDecalDelay > 0.f)
			{
				SpawnAtUseLocationAfterDelay(AbilityLocation);
			}
			else
			{
				SpawnDecalAtUseLocation(AbilityLocation);
			}
		}
	}
}

bool UCAbility_ArtilleryStrike::ShouldSeeDecal(ARTSPlayerState * AbilityInstigator) const
{
	/* The player who instigated it is the only one who sees it */
	return (AbilityInstigator == PS);
}

void UCAbility_ArtilleryStrike::SpawnAtUseLocationAfterDelay(const FVector & AbilityLocation)
{
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCAbility_ArtilleryStrike, SpawnDecalAtUseLocation), AbilityLocation);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, ShowDecalDelay, false);
}

void UCAbility_ArtilleryStrike::SpawnDecalAtUseLocation(const FVector & AbilityLocation)
{
	Statics::SpawnDecalAtLocation(this, TargetLocationDecalInfo, AbilityLocation);
}

void UCAbility_ArtilleryStrike::OnActiveStrikeExpired(UActiveArtilleryStrikeState * Expired)
{
	int32 NumRemoved = ActiveStrikes.Remove(Expired);
	assert(NumRemoved == 1);
}



// Just thinking about upgrades. Well need a virtual like GetPlayersInfoState which 
// returns the struct that holds all the damage, radius, etc for that player. 
// So will also need to initialize these data structs in FinalSetup or something.
// To reduce work seriously considering doing this automatically by probably iterating 
// UPROPERTY using all the properties for this class. Would reduce the amount of work 
// users have to do. 

// Alternative is just to spawn a UCommanderAbilityBase object for each player in the match 
// That is probably the simplest way without requiring users to define the structs too.
