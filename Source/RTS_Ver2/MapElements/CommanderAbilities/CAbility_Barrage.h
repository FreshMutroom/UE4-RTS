// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"

#include "Statics/CoreDefinitions.h"
#include "Statics/Structs/Structs_6.h"
#include "Statics/Structs_5.h"
#include "CAbility_Barrage.generated.h"

class AProjectileBase;
class UCAbility_Barrage;
struct FActiveBarrageState;
class UCurveFloat;
class AObjectPoolingManager;
enum class ETeam : uint8;
enum class ETickableTickType : uint8;


/* How to adjust the Z axis value of a location we have chosen to fire a projectile at */
UENUM()
enum class ETargetLocationZAxisOption : uint8
{
	/* e.g. Player uses ability at location (0.f, 0.f, 0.f). 
	We choose the location (500.f, 0.f, 0.f) as one of the locations around the use location 
	where we should fire a projectile. But this location may not be on the ground. 
	Best performance */
	DoNothing, 

	// Navigation system can raycast + has GetRandomPoint. Either of those might be faster than a line trace
	//NavMeshQuery,

	/* Do a line trace so every projectile is fired at a location on the ground */
	LineTrace
};


/** 
 *	All the info about how a projectile is fired during the barrage. 
 *	
 *	A salvo is just projectiles fired in a concentrated area kinda like an airship tends to fire 
 *	many shots near the same general location before choosing another general location.  
 */
USTRUCT()
struct FBarrageProjectileInfo
{
	GENERATED_BODY()

public:

	FBarrageProjectileInfo();

	void SetDamageValues(AObjectPoolingManager * PoolingManager);
	void CheckCurveAssets(UCAbility_Barrage * OwningObject);

protected:

	/** 
	 *	Projectile to use. 
	 */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "Projectile"))
	TSubclassOf<AProjectileBase> Projectile_BP;

	/* Whether to override the damage values on the projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bOverrideProjectileDamageValues;

	/* Damage values of the projectile */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bOverrideProjectileDamageValues))
	FBasicDamageInfo ProjectileDamageValues;

	/* Time from when the ability is used to when the first projectile is fired */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinInitialDelay;

	/* Time from when the ability is used to when the first projectile is fired. 
	Set this equal to MinInitialDelay to avoid randomness */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxInitialDelay;

	/** 
	 *	How long to spawn projectiles for. Measured after InitialDelay is up. After 
	 *	this amount of time is up no more projectiles will spawn. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MinDuration;

	/* For randomness */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float MaxDuration;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinTimeBetweenSalvos;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxTimeBetweenSalvos;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float SalvoRadius;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int16 MinShotsPerSalvo;

	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
	int16 MaxShotsPerSalvo;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MinTimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float MaxTimeBetweenShots;

	/** 
	 *	This is how far in front or behind the aircraft the projectile should be fired from 
	 *	e.g. on an AC130 the weapons are at different locations along the side of it 
	 *
	 *	Positive values mean further up the front of the ship
	 *	Negative values mean further near the back of the ship
	 *
	 *	My notes: no matter the sign of RotationRate the 2 statements above stay true
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float LocationOffset;

	/** 
	 *	Curve for choosing the salvo location, not individual projectile locations.
	 *	Both axis should be normalized to the range [0, 1]. The X axis must start at 0 
	 *	and end at 1. 
	 *	X axis = a random number 
	 *	Y axis = the horizontal distance the projectile should spawn from the use location 
	 *
	 *	If no curve is used here then linear will be used 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * DistanceFromCenterCurve_Salvo;

	/**
	 *	The distance from the salvo location each projectile should be. Just like  
	 *	DistanceFromCenterCurve_Salvo except for locations around the salvo location 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	UCurveFloat * DistanceFromCenterCurve_Projectiles;

	/* Whether to raycast so locations chosen are on the ground */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (DisplayName = "Z Axis Option"))
	ETargetLocationZAxisOption ZAxisOption;

public:

	// Trivial getters
	const TSubclassOf<AProjectileBase> & GetProjectileBP() const { return Projectile_BP; }
	const FBasicDamageInfo & GetProjectileDamageValues() const { return ProjectileDamageValues; }
	float GetMinInitialDelay() const { return MinInitialDelay; }
	float GetMaxInitialDelay() const { return MaxInitialDelay; }
	float GetMinDuration() const { return MinDuration; }
	float GetMaxDuration() const { return MaxDuration; }
	float GetMinTimeBetweenSalvos() const { return MinTimeBetweenSalvos; }
	float GetMaxTimeBetweenSalvos() const { return MaxTimeBetweenSalvos; }
	float GetSalvoRadius() const { return SalvoRadius; }
	int16 GetMinShotsPerSalvo() const { return MinShotsPerSalvo; }
	int16 GetMaxShotsPerSalvo() const { return MaxShotsPerSalvo; }
	float GetMinTimeBetweenShots() const { return MinTimeBetweenShots; }
	float GetMaxTimeBetweenShots() const { return MaxTimeBetweenShots; }
	float GetLocationOffset() const { return LocationOffset; }
	UCurveFloat * GetDistanceFromCenterCurve_SalvoLocation() const { return DistanceFromCenterCurve_Salvo; }
	UCurveFloat * GetDistanceFromCenterCurve_Projectiles() const { return DistanceFromCenterCurve_Projectiles; }
	ETargetLocationZAxisOption GetZAxisOption() const { return ZAxisOption; }
};


/** 
 *	State for a single projectile type of the barrage. 
 *	
 *	Decided to make this a UCLASS instead of a USTRUCT because I want to call timer handles with it 
 */
UCLASS(NotBlueprintable)
class UActiveBarrageSingleSalvoTypeState : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UActiveBarrageSingleSalvoTypeState();
	
	void Init(FActiveBarrageState * InBarrageState, const FBarrageProjectileInfo & InInfo, 
		AObjectPoolingManager * InPoolingManager, UWorld * InWorld, int32 InArrayIndex,
		float InInitialDelay, float InTimeTillStop);

	//~ Begin overrides for FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	//~ End overrides for FTickableGameObject

protected:

	FActiveBarrageState * BarrageState;
	const FBarrageProjectileInfo * Info;
	TRawPtr(AObjectPoolingManager) PoolingManager;
	TRawPtr(UWorld) World;

	float TimeSpentAlive;
	float TimeTillStop;
	float TimeTillNextProjectileSpawn;

	/* The location that is the center for the current salvo */
	FVector CurrentSalvoLocation;

	int16 NumProjectilesRemainingInSalvo;

	/* If true then the next projectile we fire is the first in a salvo */
	bool bNextProjectileIsFirstOfSalvo;

public:

	/* The array index in BarrageState->States that this is */
	int32 ArrayIndex;

protected:

	FVector CalculateProjectileSpawnLocation() const;
	FVector CalculateSalvoLocation() const;
	FVector CalculateProjectileFireAtLocation() const;
	
	/* Calculate how much time should pass between firing a projectile and firing the next projectile */
	float CalculateTimeBetweenShots() const;

	void SpawnProjectile();
	void Stop();
};


/* State for a barrage */
USTRUCT()
struct FActiveBarrageState
{
	GENERATED_BODY()

public:

	FActiveBarrageState() 
		: EffectActor(nullptr)   
		, TargetLocation(FVector(-1.f))
		, FirersOriginalLocation(FVector(-1.f))
		, UniqueID(-1)
		, InstigatorsTeam(ETeam::Uninitialized)
	{ 
	} 

	// Use this ctor
	explicit FActiveBarrageState(UCAbility_Barrage * InEffectActor, AObjectPoolingManager * InPoolingManager, 
		const FVector & InTargetLocation, int32 InUniqueID, ETeam InInstigatorsTeam, int32 InRandomNumberSeed);

	/* Calling this after the ctor cause perhaps things are getting messed up doing it in ctor. 
	Yes they were. I spent 8+ hours debugging this. The offending action was passing the 'this' 
	pointer to another struct while in our ctor */
	void MoreSetup(AObjectPoolingManager * InPoolingManager);

	FVector CalculateFirersOriginalLocation(UCAbility_Barrage * InEffectActor) const;

	/* Get a random integer between Min and Max */
	int32 GetRandomInt(int32 Min, int32 Max) const;

	/* Get a random float between Min and Max */
	float GetRandomFloat(float Min, float Max) const;

	/* Get the world location where the ability was used */
	FVector GetUseLocation() const;
	FVector GetFirersOriginalLocation() const;

	float GetRadius() const;
	float GetRotationRate() const;
	ETeam GetInstigatorsTeam() const;

	void OnActiveProjectileTypeFinished(UActiveBarrageSingleSalvoTypeState * Finished);

protected:

	TRawPtr(UCAbility_Barrage) EffectActor;

	FRandomStream RandomStream;

	/* World location where the ability was used */
	FVector TargetLocation;

	/* Whatever is firing the projectiles: this is its location when the ability was used */
	FVector FirersOriginalLocation;

	float Radius;
	float RotationRate;

	int32 UniqueID;

	/* Team of the player that instigated the ability */
	ETeam InstigatorsTeam;

	/* All the states for each projectile type */
	UPROPERTY()
	TArray<UActiveBarrageSingleSalvoTypeState *> States;

	friend bool operator==(const FActiveBarrageState & Elem_1, const FActiveBarrageState & Elem_2)
	{
		return Elem_1.UniqueID == Elem_2.UniqueID;
	}

	friend uint32 GetTypeHash(const FActiveBarrageState & Elem)
	{
		return Elem.UniqueID;
	}
};


/**
 *	Barrage launches projectiles at a location. Many different types of projectiles can be used. 
 *	It is perfect to simulate an aircraft such as an AC130 
 *	firing on a location. There is no mesh for the aircraft though.
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UCAbility_Barrage : public UCommanderAbilityBase
{
	GENERATED_BODY()
	
public:

	UCAbility_Barrage();

	//~ Begin UCommanderAbilityBase interface
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState) override;
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	void RevealFogAtTargetLocation(const FVector & AbilityLocation, ETeam InstigatorsTeam, bool bIsServer);
	void PlaySound(ARTSPlayerState * AbilityInsitgator);

public:

	void OnEffectInstanceExpired(FActiveBarrageState & Expired);

	int32 GetNumProjectileTypes() const;
	const FBarrageProjectileInfo & GetProjectileInfo(int32 Index) const;
	float GetRadius() const;
	float GetFirerDistance_Length() const;
	float GetFirerDistance_Height() const;
	float GetRotationRate() const;

protected:

	//----------------------------------------------------
	//	Data
	//----------------------------------------------------

	/* Holds all the active barrage's state info */
	UPROPERTY()
	TSet<FActiveBarrageState> ActiveBarrages;

	/** The area the ability affects */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float Radius;

	/** Horizontal distance firer is away from target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float FirerDistance_Length;

	/** Vertical distance firer is away from target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float FirerDistance_Height;

	/** 
	 *	How fast the location the projectiles are being fired from rotates around the target 
	 *	location. I think this is how many degrees per second.
	 *
	 *	Use negative values to move in the opposite direction  
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	float FirerRotationRate;

	/** Info about all the projectiles that are fired */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<FBarrageProjectileInfo> ProjectileInfo;

	/** Fog revealing info for target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTemporaryFogRevealEffectInfo TargetLocationTemporaryFogReveal;

	/* Optional sound to play for the ability instigator */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	USoundBase * UseSound;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
