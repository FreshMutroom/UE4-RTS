// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"

#include "Statics/Structs/Structs_6.h"
#include "CAbility_ArtilleryStrike.generated.h"

class AProjectileBase;
class UCurveFloat;
enum class ETickableTickType : uint8;
enum class ETeam : uint8;
class UCAbility_ArtilleryStrike;
class AObjectPoolingManager;


/* A single instance of an artillery strike */
UCLASS(NotBlueprintable)
class UActiveArtilleryStrikeState : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UActiveArtilleryStrikeState();

	void Init(UCAbility_ArtilleryStrike * InEffectObject, AObjectPoolingManager * InPoolingManager, 
		bool bInIsServer, ETeam InInstigatorsTeam, int32 InRandomNumberSeed, const FVector & UseLocation);

	//~ Begin overrides for FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	//~ End overrides for FTickableGameObject

protected:

	/* Get random integer in range */
	int32 GetRandomInt(int32 Min, int32 Max) const;
	/* Get random float in range */
	float GetRandomFloat(float Min, float Max) const;

	void SpawnProjectile();

	FVector CalculateProjectileSpawnLocation();
	float CalculateTimeBetweenShots() const;

	/* Call when the duration of the strike is up */
	void Stop();

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	UCAbility_ArtilleryStrike * EffectObject;
	AObjectPoolingManager * PoolingManager;

	float TimeTillStop;
	float TimeTillSpawnNextProjectile;

	/* Where the ability was used */
	FVector AbilityLocation;

	/* How much yaw rotation around the AbilityLocation that was applied to the last projectile 
	that was fired */
	float LastYawRot;

	FRandomStream RandomStream;

	/* Whether GetWorld()->IsServer() would return true */
	bool bIsServer;
	ETeam InstigatorsTeam;

	int16 NumProjectilesRemainingInSalvo;
};


/**
 *	Fires multiple projectiles down towards the ground
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UCAbility_ArtilleryStrike : public UCommanderAbilityBase
{
	GENERATED_BODY()
	
public:

	UCAbility_ArtilleryStrike();

	//~ Begin UCommanderAbilityBase interface
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState) override;
	//~ End UCommanderAbilityBase interface

protected:

	void CheckCurveAssets();

public:

	//~ Begin UCommanderAbilityBase interface
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	/** 
	 *	Reveal fog of war at the location this ability was used at 
	 *	
	 *	@param AbilityLocation - where this ability was used 
	 *	@param bIsServer - whether we are running code on the server 
	 */
	void RevealFogAtUseLocation(const FVector & AbilityLocation, ETeam InstigatorsTeam, bool bIsServer);

	void HandleSpawningOfDecal(const FVector & AbilityLocation, ARTSPlayerState * AbilityInstigator);
	
	/* Return true if the decal that gets burned into the ground should be seen */
	bool ShouldSeeDecal(ARTSPlayerState * AbilityInstigator) const;
	void SpawnAtUseLocationAfterDelay(const FVector & AbilityLocation);
	UFUNCTION()
	void SpawnDecalAtUseLocation(const FVector & AbilityLocation);

public:

	void OnActiveStrikeExpired(UActiveArtilleryStrikeState * Expired);

protected:

	// Write this because cbf writting lots of getters
	friend UActiveArtilleryStrikeState;

	UPROPERTY()
	TSet<UActiveArtilleryStrikeState*> ActiveStrikes;

	/** Area where projectiles are fired at */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Radius;

	/** The height projectiles spawn at above the use location's Z axis value */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float ProjectileSpawnHeight;

	/**
	 *	Projectile to use.
	 */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "Projectile"))
	TSubclassOf<AProjectileBase> Projectile_BP;

	/** Whether to override the damage values on the projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOverrideProjectileDamageValues;

	/** Damage values of the projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideProjectileDamageValues))
	FBasicDamageInfo ProjectileDamageValues;

	/** Time from when the ability is used to when the first projectile is fired */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinInitialDelay;

	/** Time from when the ability is used to when the first projectile is fired.
	Set this equal to MinInitialDelay to avoid randomness */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxInitialDelay;

	/**
	 *	How long to spawn projectiles for. Measured after InitialDelay is up. After
	 *	this amount of time is up no more projectiles will spawn.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MinDuration;

	/** For randomness */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MaxDuration;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinTimeBetweenSalvos;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxTimeBetweenSalvos;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int16 MinShotsPerSalvo;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int16 MaxShotsPerSalvo;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinTimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxTimeBetweenShots;
	
	/** 
	 *	The distance from the center that the projectiles should be shot at 
	 *	X axis = a random number. Should start at 0 and end at 1 
	 *	Y axis = normalized distance from center. Values can be anywhere from 0 to 1
	 *
	 *	If this is not set then linear will be used 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * DistanceFromCenterCurve;

	/** 
	 *	How much different the rotation the next projectile to spawn from the last one 
	 *	in degrees. Set this greater to 0 if you want your projectiles to try not to clump 
	 *	up in one spot. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MinRotation;

	/** Fog revealing info for target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTemporaryFogRevealEffectInfo TargetLocationTemporaryFogReveal;

	/* Decal to spawn at the target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Target Location Decal"))
	FSpawnDecalInfo TargetLocationDecalInfo;

	/* The delay from when ability is used to when the target location decal should spawn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float ShowDecalDelay;
};
