// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"

#include "Ability_Scan.generated.h"

enum class EAffiliation : uint8;


/**
 *	Reveals some fog of war similar to Terran's scan or C&C generals USA scan. 
 *	Can optionally reveal stealthed units too.
 *	Also: 
 *	- plays a sound 
 *	- spawns a decal at target location 
 *	- spawns a particle emitter at target location
 */
UCLASS(Blueprintable)
class RTS_VER2_API AAbility_Scan : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_Scan();

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

protected:

	void Begin(AActor * EffectInstigator, const FVector & Location, ETeam InstigatorsTeam, bool bIsServer);

	void PlaySound(const FVector & TargetLocation, ETeam InstigatorsTeam, EAffiliation InstigatorsAffiliation);
	void DrawTargetLocationDecal(const FVector & TargetLocation, EAffiliation InstigatorsAffiliation);
	void ShowTargetLocationParticles(const FVector & TargetLocation, EAffiliation InstigatorsAffiliation);

	//-----------------------------------------------------
	//	Data
	//-----------------------------------------------------

	/* Fog revealing info for target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior")
	FTemporaryFogRevealEffectInfo RevealingInfo;

	/* Sound to play at target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * TargetLocationSound;

	/* Decal to draw at the location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	FSpawnDecalInfo DecalInfo;

	/* Particle system to show at the location. Probably don't obey fog of war */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	UParticleSystem * TargetLocationParticles;

	/** 
	 *	Worst affiliation that can hear the sound produced by the scan. 
	 *
	 *	e.g. Owned = only instigator can hear it
	 *	Allied = instigator and teammates can hear it 
	 *	Hostile = everyone can hear it 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	EAffiliation CanHearSoundAffiliation;

	/** Worst affiliation that can see the decal/particles produced by the scan. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	EAffiliation CanSeeVisualsAffiliation;
};
