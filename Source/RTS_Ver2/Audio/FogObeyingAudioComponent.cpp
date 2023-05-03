// Fill out your copyright notice in the Description page of Project Settings.

#include "FogObeyingAudioComponent.h"

#include "Settings/ProjectSettings.h"
#include "GameFramework/RTSGameState.h"
#include "Managers/FogOfWarManager.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"


UFogObeyingAudioComponent::UFogObeyingAudioComponent()
{
	OnAudioFinished.AddDynamic(this, &UFogObeyingAudioComponent::OnAudioFinishedFunc);
}

void UFogObeyingAudioComponent::Activate(bool bReset)
{
	if (bReset || ShouldActivate() == true)
	{
		// No Play() call here so if you want your audio comp to play the sound that was 
		// set in editor call PlaySound at some point
		if (bIsActive)
		{
			OnComponentActivated.Broadcast(this, bReset);
		}
	}
}

void UFogObeyingAudioComponent::PseudoMute()
{
	bMuted = true;

	// Cache current volume multiplier so it can be restored later
	SavedVolumeMultiplier = VolumeMultiplier;
	
	/* Set volume so low that the sound sounds muted. I wish UE4 had a way to actually mute sounds 
	but let them keep playing. 
	From my experience a number here "too low" like 0.0001f actually causes the sound to stop 
	which is odd */
	SetVolumeMultiplier(0.001f);
}

void UFogObeyingAudioComponent::UnPseudoMute()
{
	SetVolumeMultiplier(SavedVolumeMultiplier);
	bMuted = false;
}

bool UFogObeyingAudioComponent::IsMuted() const
{
	return bMuted;
}

void UFogObeyingAudioComponent::PlaySound(ARTSGameState * GameState, USoundBase * InSound,
	float StartTime, ETeam SoundInstigatorsTeam, ESoundFogRules FogRules)
{	
#if GAME_THREAD_FOG_OF_WAR
	if (InSound == nullptr)
	{
		return;
	}
	
	const ETeam LocalPlayersTeam = GameState->GetLocalPlayersTeam();
	const FVector Location = GetComponentLocation();
	
	// Quick case
	if (FogRules == ESoundFogRules::InstigatingTeamOnly)
	{
		if (SoundInstigatorsTeam != LocalPlayersTeam)
		{
			return;
		}
	}
	else if (FogRules == ESoundFogRules::DecideOnSpawn)
	{
		const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
		if (!bOutsideFog)
		{
			return;
		}
	}

	/* Add audio component to container and possibly mute it if it shouldn't be heard right now */
	if (FogRules == ESoundFogRules::AlwaysKnownOnceHeard)
	{
		const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
		if (!bOutsideFog)
		{
			/* If the component is already playing a sound then it will already be in the 
			container and we shouldn't add it again */
			if (!bInContainer)
			{
				GameState->FogSoundsContainer_AlwaysKnownOnceHeard.Add(this);
				bInContainer = true;
			}

			PseudoMute();
		}
	}
	else if (FogRules == ESoundFogRules::DynamicExceptForInstigatorsTeam)
	{
		if (SoundInstigatorsTeam != LocalPlayersTeam)
		{
			if (!bInContainer)
			{
				GameState->FogSoundsContainer_DynamicExceptForInstigatorsTeam.Add(this);
				bInContainer = true;
			}
			
			const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
			if (!bOutsideFog)
			{
				PseudoMute();
			}
		}
	}
	else if (FogRules == ESoundFogRules::Dynamic)
	{
		if (!bInContainer)
		{
			GameState->FogSoundsContainer_Dynamic.Add(this);
			bInContainer = true;
		}

		const bool bOutsideFog = GameState->GetFogManager()->IsLocationVisibleNotChecked(Location, LocalPlayersTeam);
		if (!bOutsideFog)
		{
			PseudoMute();
		}
	}

	// Note: UAudioComponent::SetSound() calls Stop()
	SetSound(InSound);
	Play(StartTime);
#elif MULTITHREADED_FOG_OF_WAR
	//TODO
#else
	SetSound(InSound);
	Play(StartTime);
#endif
}

void UFogObeyingAudioComponent::OnExitFogOfWar_Dynamic(ARTSGameState * GameState)
{
	UnPseudoMute();
}

void UFogObeyingAudioComponent::OnEnterFogOfWar_Dynamic(ARTSGameState * GameState)
{
	PseudoMute();
}

void UFogObeyingAudioComponent::OnExitFogOfWar_DynamicExceptForInstigatorsTeam(ARTSGameState * GameState)
{
	UnPseudoMute();
}

void UFogObeyingAudioComponent::OnEnterFogOfWar_DynamicExceptForInstigatorsTeam(ARTSGameState * GameState)
{
	PseudoMute();
}

void UFogObeyingAudioComponent::OnExitFogOfWar_AlwaysKnownOnceHeard(ARTSGameState * GameState)
{
	UnPseudoMute();

	GameState->FogSoundsContainer_AlwaysKnownOnceHeard.RemoveChecked(this);
	bInContainer = false;
}

void UFogObeyingAudioComponent::OnAudioFinishedFunc()
{
#if GAME_THREAD_FOG_OF_WAR
	ARTSGameState * GameState = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	// Remove from relevant container
	if (FogObeyingRule == ESoundFogRules::AlwaysKnownOnceHeard)
	{
		if (bInContainer)
		{
			bInContainer = false;
			GameState->FogSoundsContainer_AlwaysKnownOnceHeard.RemoveChecked(this);
		}
	}
	else if (FogObeyingRule == ESoundFogRules::DynamicExceptForInstigatorsTeam)
	{
		if (bInContainer)
		{
			bInContainer = false;
			GameState->FogSoundsContainer_DynamicExceptForInstigatorsTeam.RemoveChecked(this);
		}
	}
	else if (FogObeyingRule == ESoundFogRules::Dynamic)
	{
		if (bInContainer)
		{
			bInContainer = false;
			GameState->FogSoundsContainer_Dynamic.RemoveChecked(this);
		}
	}
	else {} // Method where there is no container for it 

#elif MULTITHREADED_FOG_OF_WAR

	// TODO

#else

#endif

	// So at this point is the audio comp going to be GCed since it's now stopped? Because 
	// this is what I want. And we're assuming it isn't a UPROPERTY anywhere. 
	// Ok Good I have tested this in a test project. Calling stop will make the audio comp 
	// be GCed but only if it is not a UPROPERTY which is exactly what I expected/wanted. 
	// So TWeakObjectPtr will be made null, while raw ptr will still return non-null (even 
	// though it has been GCed).
}
