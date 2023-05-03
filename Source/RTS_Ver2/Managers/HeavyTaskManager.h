// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h"
#include "Managers/HeavyTaskManagerTypes.h"
#include "HeavyTaskManager.generated.h"

enum class ETickableTickType : uint8;
class IBuildingAttackComp_Turret;
struct FTraceHandle;
struct FTraceDatum;


/**
 *	Heavy task manager is ment to carry out computer resource intense tasks as 'spread out' 
 *	as possible to avoid any hitching. e.g. every so often a capsule sweep is required to see what units are in range 
 *	of other units. Well this manager is ment to make it so each frame does approx the same number 
 *	of sweeps as the last frame.
 */
UCLASS(NotBlueprintable)
class RTS_VER2_API UHeavyTaskManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
protected:

	//~ Begin overrides for FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	//~ End overrides for FTickableGameObject

	/* Check all the actors that overlap a capsule at a world location */
	static FTraceHandle AsyncCapsuleOverlapTest(UWorld * World, const FVector & Location,
		const FCollisionObjectQueryParams & QueryParams, float Radius, FOverlapDelegate * InDelegate);

public:

	/**
	 *	Have a building attack component start doing sweeps for targets  
	 */
	void RegisterBuildingAttackComponent(IBuildingAttackComp_Turret * InComponent);
	
	/** 
	 *	Stop a building attack component from having its sweep done. If the building has 
	 *	reached zero health then that might be a time when you want to call this 
	 */
	void UnregisterBuildingAttackComponent(IBuildingAttackComp_Turret * InComponent);

protected:

	/* Return the which bucket a defense component should be placed into */
	uint8 GetOptimalBucket_BuildingAttackComponent();

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	static constexpr uint8 NUM_BUILDING_ATTACK_COMP_BUCKETS = 16;

	uint8 CurrentBuildingAttackCompBucketForTick;

	/** 
	 *	Each building attack comp that wants to find targets.
	 */
	Array_BuildingAttackComp_TurretData BuildingAttackComps[NUM_BUILDING_ATTACK_COMP_BUCKETS];
};
