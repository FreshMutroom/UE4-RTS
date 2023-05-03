// Fill out your copyright notice in the Description page of Project Settings.

#include "Ability_Scan.h"


AAbility_Scan::AAbility_Scan()
{
	//~ Begin AAbilityBase interface
	bHasMultipleOutcomes =			EUninitializableBool::False;
	bCallAoEStartFunction =			EUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	EUninitializableBool::False;
	bRequiresTargetOtherThanSelf =	EUninitializableBool::False;
	bRequiresLocation =				EUninitializableBool::True;
	bHasRandomBehavior =			EUninitializableBool::False;
	bRequiresTickCount =			EUninitializableBool::False;
	//~ End AAbilityBase interface

	CanHearSoundAffiliation = EAffiliation::Owned;
	CanSeeVisualsAffiliation = EAffiliation::Owned;
}

void AAbility_Scan::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SuperServerBegin;

	Begin(EffectInstigator, Location, InstigatorsTeam, true);
}

void AAbility_Scan::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	SuperClientBegin;

	Begin(EffectInstigator, Location, InstigatorsTeam, false);
}

void AAbility_Scan::Begin(AActor * EffectInstigator, const FVector & Location, ETeam InstigatorsTeam, bool bIsServer)
{
	ARTSPlayerState * LocalPlayerState = CastChecked<ARTSPlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState);
	const EAffiliation InstigatorsAffiliation = CastChecked<ARTSPlayerState>(EffectInstigator->GetOwner())->GetAffiliation();

	/* It's kind of becoming a recuring theme that we need to check whether InstigatorsTeam is
	the same team as the local player. Consider assigning this at some point during match setup
	to a new variable in AAbilityBase so we can query that instead of doing the whole
	GetWorld()->GetFirstPlayerController->PlayerState->GetTeam() */

	// Reveal fog/stealth
#if GAME_THREAD_FOG_OF_WAR
	RevealingInfo.CreateForTeam(GS->GetFogManager(), FVector2D(Location), InstigatorsTeam,
		bIsServer, LocalPlayerState->GetTeam() == InstigatorsTeam);
#elif MULTITHREADED_FOG_OF_WAR
	RevealingInfo.CreateForTeam(FVector2D(Location), InstigatorsTeam,
		bIsServer, LocalPlayerState->GetTeam() == InstigatorsTeam);
#endif

	// Play sound
	PlaySound(Location, InstigatorsTeam, InstigatorsAffiliation);

	// Draw decal
	DrawTargetLocationDecal(Location, InstigatorsAffiliation);

	// Show particle system
	ShowTargetLocationParticles(Location, InstigatorsAffiliation);
}

void AAbility_Scan::PlaySound(const FVector & TargetLocation, ETeam InstigatorsTeam, EAffiliation InstigatorsAffiliation)
{
	if (TargetLocationSound != nullptr)
	{
		if (InstigatorsAffiliation <= CanHearSoundAffiliation)
		{
			Statics::SpawnSoundAtLocation(GS, InstigatorsTeam, TargetLocationSound, TargetLocation, 
				ESoundFogRules::InstigatingTeamOnly);
		}
	}
}

void AAbility_Scan::DrawTargetLocationDecal(const FVector & TargetLocation, EAffiliation InstigatorsAffiliation)
{
	if (InstigatorsAffiliation <= CanSeeVisualsAffiliation)
	{
		Statics::SpawnDecalAtLocation(this, DecalInfo, TargetLocation);
	}
}

void AAbility_Scan::ShowTargetLocationParticles(const FVector & TargetLocation, EAffiliation InstigatorsAffiliation)
{
	if (TargetLocationParticles != nullptr)
	{
		if (InstigatorsAffiliation <= CanSeeVisualsAffiliation)
		{
			Statics::SpawnParticles(TargetLocationParticles, GS, TargetLocation, FRotator::ZeroRotator,
				FVector::OneVector);
		}
	}
}
