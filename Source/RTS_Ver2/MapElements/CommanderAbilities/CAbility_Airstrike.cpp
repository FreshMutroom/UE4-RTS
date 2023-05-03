// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbility_Airstrike.h"

#include "MapElements/CommanderAbilities/Actors/Warthog.h"


UCAbility_Airstrike::UCAbility_Airstrike()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::True;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	bRequiresDirection =			ECommanderUninitializableBool::True;
	//~ End UCommanderAbilityBase variables

	FogRevealDurationAfterFinalShot = 5.f;
}

void UCAbility_Airstrike::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	Super::FinalSetup(GameInst, GameState, LocalPlayersPlayerState);

	// Set damage values of warthog
	AWarthog * Warthog = GetWorld()->SpawnActor<AWarthog>(Warthog_BP, Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
	// Projectile type 1
	if (Warthog->GetAttackAttributes().bOverrideProjectileType1DamageValues == false)
	{
		AProjectileBase * Projectile = GameInst->GetPoolingManager()->GetProjectileReference(Warthog->GetAttackAttributes().ProjectileType1_BP);
		Warthog->GetAttackAttributes().ProjectileType1_DamageAttributes.SetValuesFromProjectile(Projectile);
	}
	// Projectile type 2
	if (Warthog->GetAttackAttributes().bOverrideProjectileType2DamageValues == false)
	{
		AProjectileBase * Projectile = GameInst->GetPoolingManager()->GetProjectileReference(Warthog->GetAttackAttributes().ProjectileType2_BP);
		Warthog->GetAttackAttributes().ProjectileType2_DamageAttributes.SetValuesFromProjectile(Projectile);
	}

	Warthog->Destroy();
}

void UCAbility_Airstrike::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	OutRandomNumberSeed = GenerateInitialRandomSeed();
	const int32 Seed32Bit = SeedAs16BitTo32Bit(OutRandomNumberSeed);

	// Sending in warthog
	// Spawn in the middle of nowhere, then move it to desired location
	AWarthog * Warthog = GetWorld()->SpawnActor<AWarthog>(Warthog_BP, Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
	const FVector SpawnLocation = Warthog->GetStartingLocation(AbilityInstigator, Location, OutDirection);
	const FRotator SpawnRotation = AWarthog::GetStartingRotation(SpawnLocation, OutDirection);
	Warthog->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	Warthog->InitialSetup(GI, GI->GetPoolingManager());
	Warthog->OnOwningAbilityUsed(GS, AbilityInstigator, Location, Seed32Bit);

	// Reveal fog of war at use location
	RevealFogAtTargetLocation(Warthog, SpawnLocation, Location, InstigatorsTeam, true, AbilityInstigator);
}

void UCAbility_Airstrike::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	// Sending in warthog
	// Spawn in the middle of nowhere, then move it to desired location
	AWarthog * Warthog = GetWorld()->SpawnActor<AWarthog>(Warthog_BP, Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);
	const FVector SpawnLocation = Warthog->GetStartingLocation(Location, Direction);
	const FRotator SpawnRotation = AWarthog::GetStartingRotation(SpawnLocation, Direction);
	Warthog->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	Warthog->InitialSetup(GI, GI->GetPoolingManager());
	Warthog->OnOwningAbilityUsed(GS, AbilityInstigator, Location, RandomNumberSeed);

	// Reveal fog of war at use location
	RevealFogAtTargetLocation(Warthog, SpawnLocation, Location, InstigatorsTeam, false, AbilityInstigator);
}

void UCAbility_Airstrike::RevealFogAtTargetLocation(AWarthog * Warthog, const FVector & SpawnLocation, const FVector & TargetLocation, 
	ETeam InstigatorsTeam, bool bIsServer, ARTSPlayerState * LocalPlayersPlayerState)
{
#if GAME_THREAD_FOG_OF_WAR || MULTITHREADED_FOG_OF_WAR
	/* The fog reveal time is calculated based on how far the warthog has to travel so 
	calculate and set that now.
	This is a rough calculation - it won't be exact. It for one doesn't take into account the 
	phase2 and phase3 move speed */
	const float WarthogsSpeed = Warthog->GetPhase1AndPhase4MoveSpeed();
	const float DistanceToTarget = (TargetLocation - SpawnLocation).Size();
	const float ApproxTimeToTarget = DistanceToTarget / WarthogsSpeed;
	const float FogRevealDuation = ApproxTimeToTarget + FogRevealDurationAfterFinalShot;

	TargetLocationTemporaryFogReveal.SetDuration(FogRevealDuation);
#endif

#if GAME_THREAD_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(GS->GetFogManager(), FVector2D(TargetLocation),
		InstigatorsTeam, bIsServer, LocalPlayersPlayerState->GetTeam() == InstigatorsTeam);

#elif MULTITHREADED_FOG_OF_WAR

	TargetLocationTemporaryFogReveal.CreateForTeam(FVector2D(TargetLocation),
		InstigatorsTeam, bIsServer, LocalPlayersPlayerState->GetTeam() == InstigatorsTeam);

#endif
}
