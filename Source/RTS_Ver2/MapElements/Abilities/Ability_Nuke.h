// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/Abilities/AbilityBase.h"
#include "Statics/Structs/Structs_6.h"
#include "Ability_Nuke.generated.h"

struct FPropertyChangedChainEvent;
class UCurveFloat;
enum class ESelectableBodySocket : uint8;
class ARTSPlayerState;
class ALeaveThenComeBackProjectile;
struct FAttachInfo;
class AObjectPoolingManager;


/**
 *	- Launches a ALeaveThenComeBackProjectile. 
 *	- Shows particles at launch site.
 *	- Plays sound at launch site. 
 *	- Notifies players of launch. 
 *	- Reveals fog at target location. 
 *	- Spawns decal at target location.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API AAbility_Nuke : public AAbilityBase
{
	GENERATED_BODY()
	
public:

	AAbility_Nuke();

protected:

	virtual void BeginPlay() override;

	//~ Begin AAbilityBase interface
	virtual void Server_Begin(FUNCTION_SIGNATURE_ServerBegin) override;
	virtual void Client_Begin(FUNCTION_SIGNATURE_ClientBegin) override;
	//~ End AAbilityBase interface

	void Begin(AActor * EffectInstigator, ISelectable * InstigatorAsSelectable, AActor * Target,
		ISelectable * TargetAsSelectable, ETeam InstigatorsTeam, const FVector & Location, 
		bool bIsServer);

	/** 
	 *	Tells the local player about the launch so they can show something on their HUD and 
	 *	play a sound or whatever. 
	 */
	void NotifyLocalPlayerOfLaunch(ARTSPlayerState * InstigatorsOwner, ETeam InstigatorsTeam, 
		ARTSPlayerState * LocalPlayersPlayerState);

	/* Return where the projectile should launch from. Both AActor and ISelectable versions */
	AActor * GetLaunchActor(AActor * EffectInstigator, AActor * Target) const;
	ISelectable * GetLaunchSelectable(ISelectable * InstigatorAsSelectable, ISelectable * TargetAsSelectable) const;

	void ShowLaunchLocationParticles(const FVector & LaunchLocation, const FRotator & LaunchRotation);
	void PlayLaunchLocationSound(const FVector & LaunchLocation, ETeam InstigatorsTeam);

	void LaunchProjectile(AActor * ProjectileLauncher, ETeam LaunchersTeam, const FVector & LaunchLocation, 
		const FVector & TargetLocation);

	void RevealFogAtTargetLocation(const FVector & TargetLocation, ETeam InstigatorsTeam, bool bIsServer, 
		ARTSPlayerState * LocalPlayersPlayerState);
	
	UFUNCTION()
	void SpawnDecalAtTargetLocation(const FVector & TargetLocation, ARTSPlayerState * InstigatorsOwner, 
		ETeam InstigatorsTeam, ARTSPlayerState * LocalPlayersPlayerState);

	void SpawnDecalAtTargetLocationAfterDelay(const FVector & TargetLocation, ARTSPlayerState * InstigatorsOwner,
		ETeam InstigatorsTeam, ARTSPlayerState * LocalPlayersPlayerState);

	//-------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------

	/* Reference to object pooling manager */
	TWeakObjectPtr<AObjectPoolingManager> PoolingManager;

	/* Socket that missle is launched from */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	ESelectableBodySocket LaunchBodyLocation;

	/* Projectile to use as nuke for when it is launched. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	TSubclassOf<ALeaveThenComeBackProjectile> Projectile_BP;
	
	/* Particles to play at launch location e.g. perhaps lots of smoke or something */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	UParticleSystem * LaunchLocationParticles;

	/* The sound to play in the world at the launch site */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Audio")
	USoundBase * LaunchLocationSound;

	/** 
	 *	If true then missle launches from the ability's target. If false then the missle launches 
	 *	from the ability instigator. 
	 *	
	 *	If the button for this ability resides on the silo's action bar then this can be false. 
	 *	If the button for this ability resides on a unit similar to terran's ghosts then this 
	 *	could be true and you would pass in the silo as the target for the ability to make the 
	 *	projectile launch from that silo.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals")
	bool bLaunchesFromAbilityTarget;

	/* Fog revealing info for target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Behavior")
	FTemporaryFogRevealEffectInfo TargetLocationTemporaryFogReveal;

	/* Whether to override the damage values defined on the projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage")
	bool bOverrideProjectileDamageValues;

	/* AoE damage properties of the projectile if choosing to override them */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Damage", meta = (EditCondition = bOverrideProjectileDamageValues, DisplayName = "Damage Values"))
	FBasicDamageInfo DamageInfo;

	/* Decal to spawn at the target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals", meta = (DisplayName = "Target Location Decal"))
	FSpawnDecalInfo TargetLocationDecalInfo;

	/* The delay from when ability is used to when the target location decal should spawn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Visuals", meta = (ClampMin = 0))
	float ShowDecalDelay;

	/* What about damage radius and falloff curve overrides too? The function that fires 
	projectiles will need to have them added as params if we want this */

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
