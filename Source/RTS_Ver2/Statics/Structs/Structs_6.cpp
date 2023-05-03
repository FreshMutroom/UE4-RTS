// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_6.h"

#include "Managers/MultithreadedFogOfWar.h"
#include "Managers/FogOfWarManager.h"


#if GAME_THREAD_FOG_OF_WAR
void FTemporaryFogRevealEffectInfo::CreateForTeam(AFogOfWarManager * FogManager, FVector2D WorldLocation, 
	ETeam Team, bool bIsServer, bool bTeamIsLocalPlayersTeam) const
{
	if (bIsServer || bTeamIsLocalPlayersTeam)
	{
		// Only continue if the effect will actually do something
		if (FogRevealRadius > 0.f || StealthRevealRadius > 0.f)
		{
			FogManager->CreateTeamTemporaryRevealEffect(*this, WorldLocation, Team);
		}
	}
}
#elif MULTITHREADED_FOG_OF_WAR
void FTemporaryFogRevealEffectInfo::CreateForTeam(FVector2D WorldLocation, ETeam Team, bool bIsServer, bool bTeamIsLocalPlayersTeam) const
{
	if (bIsServer || bTeamIsLocalPlayersTeam)
	{
		// Only continue if the effect will actually do something
		if (FogRevealRadius > 0.f || StealthRevealRadius > 0.f)
		{
			MultithreadedFogOfWarManager::Get().RegisterTeamTemporaryRevealEffect(*this, WorldLocation, Team);
		}
	}
}
#endif

void FTemporaryFogRevealEffectInfo::Tick(float DeltaTime, float & OutFogRevealRadius, 
	float & OutStealthRevealRadius)
{
	TimeExisted += DeltaTime;

	if (RevealRateCurve != nullptr)
	{
		const float FogRevealRadiusMultiplier = RevealRateCurve->GetFloatValue(TimeExisted);

		OutFogRevealRadius = FogRevealRadius * FogRevealRadiusMultiplier;
		OutStealthRevealRadius = StealthRevealRadius * FogRevealRadiusMultiplier;
	}
	else // No curve asset set
	{
		if (RevealSpeed > 0.f)
		{
			// Use linear
			OutFogRevealRadius = FMath::Min(TimeExisted * RevealSpeed, FogRevealRadius);
			OutStealthRevealRadius = FMath::Min(TimeExisted * RevealSpeed, StealthRevealRadius);
		}
		else
		{
			OutFogRevealRadius = FogRevealRadius;
			OutStealthRevealRadius = StealthRevealRadius;
		}
	}
}

FVector2D FTemporaryFogRevealEffectInfo::GetLocation() const
{
	return Location;
}

bool FTemporaryFogRevealEffectInfo::HasCompleted() const
{
	return TimeExisted >= Duration;
}
