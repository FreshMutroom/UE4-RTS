// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "FogObeyingAudioComponent.generated.h"

enum class ESoundFogRules : uint8;
class ARTSGameState;
enum class ETeam : uint8;


/**
 *	A sound component that can decide who hears the sounds in relation to teams and fog of war. 
 */
UCLASS()
class RTS_VER2_API UFogObeyingAudioComponent : public UAudioComponent
{
	GENERATED_BODY()

public:

	UFogObeyingAudioComponent();
	
	virtual void Activate(bool bReset) override;

	/* This function is ment to mute the audio component while at the same time still let it play */
	void PseudoMute();
	void UnPseudoMute();

	bool IsMuted() const;

	/* Use this function to play a sound on this audio component */
	void PlaySound(ARTSGameState * GameState, USoundBase * InSound, float StartTime,
		ETeam SoundInstigatorsTeam, ESoundFogRules FogRules);

	/* Functions called by the fog of war manager */
	void OnExitFogOfWar_Dynamic(ARTSGameState * GameState);
	void OnEnterFogOfWar_Dynamic(ARTSGameState * GameState);
	void OnExitFogOfWar_DynamicExceptForInstigatorsTeam(ARTSGameState * GameState);
	void OnEnterFogOfWar_DynamicExceptForInstigatorsTeam(ARTSGameState * GameState);
	void OnExitFogOfWar_AlwaysKnownOnceHeard(ARTSGameState * GameState);

protected:
	
	// Bound to OnAudioFinished
	UFUNCTION()
	void OnAudioFinishedFunc();

public:

	//------------------------------------------------------------
	//	Data
	//------------------------------------------------------------

	/* Volume multiplier before this audio component was muted */
	float SavedVolumeMultiplier;

	bool bMuted;

	/* Whether the audio component is in a game state container. Faster to just query this 
	bool than to do a contains check on the array. Actually I use a TMap but this is still 
	probably faster */
	bool bInContainer;

	/* How audio played through this component obeys fog of war */
	ESoundFogRules FogObeyingRule;
};
