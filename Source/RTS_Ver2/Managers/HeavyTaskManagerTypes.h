// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class IBuildingAttackComp_Turret;


/* Data to carry out a sweep for a building turret component */
struct BuildingAttackComp_TurretData
{
public:

	/**
	 *	Constructor for a IBuildingAttackComp_Turret to call 
	 */
	explicit BuildingAttackComp_TurretData(IBuildingAttackComp_Turret * InComponent);

	IBuildingAttackComp_Turret * GetAttackComponent() const { return AttackComponent; }
	FOverlapDelegate * GetTraceDelegate() const { return TraceDelegate; }
	const FCollisionObjectQueryParams & GetQueryParams() const { return QueryParams; }
	FVector GetLocation() const { return Location; }
	float GetSweepRadius() const { return SweepRadius; }

protected:

	IBuildingAttackComp_Turret * AttackComponent;

	FOverlapDelegate * TraceDelegate;

	FCollisionObjectQueryParams QueryParams;

	/* World location where to do sweep from */
	FVector Location;

	/* Radius of sweep */
	float SweepRadius;
};


struct Array_BuildingAttackComp_TurretData
{
	TArray<BuildingAttackComp_TurretData> Array;
};


/* All the data required for an object to find itself in O(1) */
struct FTaskManagerBucketInfo
{
public:

	FTaskManagerBucketInfo()
		: BucketIndex(UINT8_MAX)
		, ArrayIndex(-1)
	{}

	explicit FTaskManagerBucketInfo(uint8 InBucketIndex, int16 InArrayIndex)
		: BucketIndex(InBucketIndex)
		, ArrayIndex(InArrayIndex)
	{}

	/* The bucket the object is in */
	uint8 BucketIndex;

	/* Index in bucket the object is in */
	int16 ArrayIndex;
};
