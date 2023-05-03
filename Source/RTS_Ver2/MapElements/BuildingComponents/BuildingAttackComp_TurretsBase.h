// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuildingAttackComp_TurretsBase.generated.h"

class UActorComponent;
class ABuilding;


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBuildingAttackComp_TurretsBase : public UInterface
{
	GENERATED_BODY()
};

/**
 *	The part of a turret that rotates yaw. It will have a UBuildingAttackComp_Turret 
 *	as it's child.
 *
 *	Base is all I could think of but perhaps think of a different name cause base is 
 *	commonly used in C++
 */
class RTS_VER2_API IBuildingAttackComp_TurretsBase
{
	GENERATED_BODY()

public:

	/** 
	 *	Setupy function 
	 *	
	 *	@param InOwningBuilding - the building this comp is attached to
	 *	@param Turret - the IBuildingAttackComp_Turret component that os a child of this comp. 
	 *	Will probably always be non-null 
	 */
	virtual void SetupComp(ABuilding * InOwningBuilding, UActorComponent * Turret) PURE_VIRTUAL(IBuildingAttackComp_TurretsBase::SetupComp, );

	/** 
	 *	Get in world space the component's yaw rotation for targeting purposes, but it might 
	 *	not be the actual value returned by GetComponentRotation().Yaw 
	 */
	virtual float GetEffectiveComponentYawRotation() const PURE_VIRTUAL(IBuildingAttackComp_TurretsBase::GetEffectiveComponentYawRotation, { return -1.f; });

	/**
	 *	Called when the building this component is attached to enters fog of war for the local player
	 *
	 *	@param bOnServer - true if GetWorld()->IsServer() would return true
	 */
	virtual void OnParentBuildingEnterFogOfWar(bool bOnServer) PURE_VIRTUAL(IBuildingAttackComp_TurretsBase::OnParentBuildingEnterFogOfWar, );

	/* Called when the building this component is attached to exits fog of war */
	virtual void OnParentBuildingExitFogOfWar() PURE_VIRTUAL(IBuildingAttackComp_TurretsBase::OnParentBuildingExitFogOfWar, );
};
