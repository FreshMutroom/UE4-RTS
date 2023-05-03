// Fill out your copyright notice in the Description page of Project Settings.


#include "HeavyTaskManager.h"
#include "Engine/World.h"

#include "MapElements/BuildingComponents/BuildingAttackComp_Turret.h"
#include "Statics/Statics.h"
#include "Statics/DevelopmentStatics.h"


void UHeavyTaskManager::Tick(float DeltaTime)
{
	UWorld * World = GetWorld();

	//---------------------------------------------------------------
	//	Building attack components
	//---------------------------------------------------------------

	// Advance bucket index
	CurrentBuildingAttackCompBucketForTick = (CurrentBuildingAttackCompBucketForTick + 1) % NUM_BUILDING_ATTACK_COMP_BUCKETS;
	
	for (const BuildingAttackComp_TurretData & CompData : BuildingAttackComps[CurrentBuildingAttackCompBucketForTick].Array)
	{ 
		AsyncCapsuleOverlapTest(World, CompData.GetLocation(), CompData.GetQueryParams(), CompData.GetSweepRadius(), CompData.GetTraceDelegate());
	}
}

TStatId UHeavyTaskManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UHeavyTaskManager, STATGROUP_Tickables);
}

ETickableTickType UHeavyTaskManager::GetTickableTickType() const
{
	// Stop CDO from ever ticking cause it will otherwise
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

FTraceHandle UHeavyTaskManager::AsyncCapsuleOverlapTest(UWorld * World, const FVector & Location, const FCollisionObjectQueryParams & QueryParams, float Radius, FOverlapDelegate * InDelegate)
{
	assert(InDelegate->IsBound());

	return World->AsyncOverlapByObjectType(Location, FQuat::Identity, QueryParams, FCollisionShape::MakeCapsule(Radius, Statics::SWEEP_HEIGHT), FCollisionQueryParams::DefaultQueryParam, InDelegate);
}

void UHeavyTaskManager::RegisterBuildingAttackComponent(IBuildingAttackComp_Turret * InComponent)
{
	const uint8 BucketIndex = GetOptimalBucket_BuildingAttackComponent();
	const int16 ArrayIndex = BuildingAttackComps[BucketIndex].Array.Emplace(BuildingAttackComp_TurretData(InComponent));
	InComponent->SetTaskManagerBucketIndices(BucketIndex, ArrayIndex);
}

void UHeavyTaskManager::UnregisterBuildingAttackComponent(IBuildingAttackComp_Turret * InComponent)
{
	uint8 BucketIndex;
	int16 ArrayIndex;
	InComponent->GrabTaskManagerBucketIndices(BucketIndex, ArrayIndex);

	TArray<BuildingAttackComp_TurretData> & BucketArray = BuildingAttackComps[BucketIndex].Array;
	
	assert(BucketArray[ArrayIndex].GetAttackComponent() == InComponent);

	/* Remove swap the element, making sure to update the index for the swapped element */
	if (BucketArray.Num() > 1)
	{
		if (ArrayIndex == BucketArray.Num() - 1)
		{
			// If it's the last element just remove it. Done
			BucketArray.RemoveAt(ArrayIndex, 1, false);
		}
		else
		{
			BucketArray.RemoveAtSwap(ArrayIndex, 1, false);

			// Update the array index for the swapped element
			BucketArray[ArrayIndex].GetAttackComponent()->SetTaskManagerArrayIndex(ArrayIndex);
		}
	}
	else
	{
		assert(ArrayIndex == 0);
		BucketArray.RemoveAt(ArrayIndex, 1, false);
	}
}

uint8 UHeavyTaskManager::GetOptimalBucket_BuildingAttackComponent()
{
	uint8 BestIndex = 0;
	int32 SmallestBucketSize = INT32_MAX;
	uint8 Index = 0;
	/* O(n) where n is number of buckets */
	for (const auto & Bucket : BuildingAttackComps)
	{
		if (Bucket.Array.Num() < SmallestBucketSize)
		{
			SmallestBucketSize = Bucket.Array.Num();
			BestIndex = Index;
		}
		Index++;
	}

	return BestIndex;
}
