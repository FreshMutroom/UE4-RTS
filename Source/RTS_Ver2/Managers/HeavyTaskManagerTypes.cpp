// Fill out your copyright notice in the Description page of Project Settings.


#include "HeavyTaskManagerTypes.h"


BuildingAttackComp_TurretData::BuildingAttackComp_TurretData(IBuildingAttackComp_Turret * InComponent)
	: AttackComponent(InComponent)
	, TraceDelegate(InComponent->GetTargetingTraceDelegate())
	, QueryParams(InComponent->GetTargetingQueryParams())
	, Location(InComponent->GetTargetingSweepOrigin())
	, SweepRadius(InComponent->GetTargetingSweepRadius())
{
	/* If this throws in your ctor make sure to do Delegate.BindUObject(YourFunc) */
	assert(TraceDelegate->IsBoundToObject(CastChecked<UMeshComponent>(InComponent)));
}
