// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuildingAttackComp_Turret.generated.h"

struct FTraceHandle;
struct FTraceDatum;
class UMeshComponent;
struct FVisibilityInfo;
class AObjectPoolingManager;
class ABuilding;
class ARTSGameState;
struct FOverlapDatum;
class IBuildingAttackComp_TurretsBase;
enum class ETeam : uint8;


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBuildingAttackComp_Turret : public UInterface
{
	GENERATED_BODY()
};

/**
 *	USceneComponents implementing this interface can be used to attack. This interface 
 *	was added for building attack components. It only really exists because I want to 
 *	allow both static and skeletal meshes to be used for attack components.
 *
 *	Turrets can optionally add pitch rotation and can fire projectiles. If they are a child 
 *	of a IBuildingAttackComp_TurretBase then the base will rotate yaw. 
 */
class RTS_VER2_API IBuildingAttackComp_Turret
{
	GENERATED_BODY()

public:

	virtual UMeshComponent * GetAsMeshComponent() PURE_VIRTUAL(IBuildingAttackComp_Turret::InitialSetup, { return nullptr; });

	/** 
	 *	Set up function. Set whether the turret is a direct child of a rotating base component 
	 *	IBuildingAttackComp_TurretsBase 
	 *
	 *	@param InOwningBuilding - the building this component is attached to
	 */	
	virtual void SetupAttackComp(ABuilding * InOwningBuilding, ARTSGameState * GameState, AObjectPoolingManager * InPoolingManager, uint8 InUniqueID, IBuildingAttackComp_TurretsBase * InTurretRotatingBase) PURE_VIRTUAL(IBuildingAttackComp_Turret::InitialSetup, );

	/**
	 *	[Server]
	 *	@param InTeam - team the owner of this structure is on
	 *	@param InTeamsVisibilityInfo - visibility info struct for the team this structure belongs to
	 */
	virtual void ServerSetupAttackCompMore(ETeam InTeam, const FVisibilityInfo * InTeamsVisibilityInfo, FCollisionObjectQueryParams InQueryParams) PURE_VIRTUAL(IBuildingAttackComp_Turret::ServerSetupAttackCompMore, );

	/* [Client] */
	virtual void ClientSetupAttackCompMore(ETeam InTeam) PURE_VIRTUAL(IBuildingAttackComp_Turret::ServerSetupAttackCompMore, );

	/* Return a trace delegate. You should make sure this is bound to OnSweepForTargetsComplete */
	virtual FOverlapDelegate * GetTargetingTraceDelegate() PURE_VIRTUAL(IBuildingAttackComp_Turret::GetTargetingTraceDelegate, { return nullptr; });
	/* Return the query params to use for aquiring targets with sweep */
	virtual FCollisionObjectQueryParams GetTargetingQueryParams() const PURE_VIRTUAL(IBuildingAttackComp_Turret::GetTargetingQueryParams, { return FCollisionObjectQueryParams::DefaultObjectQueryParam; });
	/* Return world location where the sweep should be */
	virtual FVector GetTargetingSweepOrigin() const PURE_VIRTUAL(IBuildingAttackComp_Turret::GetTargetingSweepOrigin, { return FVector::ZeroVector; });
	/* Return the radius of the sweep */
	virtual float GetTargetingSweepRadius() const PURE_VIRTUAL(IBuildingAttackComp_Turret::GetTargetingSweepRadius, { return -1.f; });

	/* Set what bucket in the heavy task manager this comp is placed in */
	virtual void SetTaskManagerBucketIndices(uint8 BucketIndex, int16 ArrayIndex) PURE_VIRTUAL(IBuildingAttackComp_Turret::SetTaskManagerBucketIndices, );

	/* Just set the array index */
	virtual void SetTaskManagerArrayIndex(int16 ArrayIndex) PURE_VIRTUAL(IBuildingAttackComp_Turret::SetTaskManagerArrayIndex, );

	/** 
	 *	Get the heavy task manager indicies info. I called it grab cause calling it 
	 *	Get would make it's name only 1 letter different from the setter and I know it'll cause a 
	 *	headache in the future. 
	 */
	virtual void GrabTaskManagerBucketIndices(uint8 & OutBucketIndex, int16 & OutArrayIndex) const PURE_VIRTUAL(IBuildingAttackComp_Turret::GrabTaskManagerBucketIndices, );

	/* [Client] Called on clients when an attack is made on server */
	virtual void ClientDoAttack(AActor * AttackTarget) PURE_VIRTUAL(IBuildingAttackComp_Turret::ClientDoAttack, );

	/* Return the actor this turret is targeting. Can return null if it is not targeting anything */
	virtual AActor * GetTarget() const PURE_VIRTUAL(IBuildingAttackComp_Turret::GetTarget, { return nullptr; });

	/** 
	 *	Called when the building this component is attached to enters fog of war for the local player 
	 *	
	 *	@param bOnServer - true if GetWorld()->IsServer() would return true 
	 */
	virtual void OnParentBuildingEnterFogOfWar(bool bOnServer) PURE_VIRTUAL(IBuildingAttackComp_Turret::OnParentBuildingEnterFogOfWar, );

	/** 
	 *	Called when the building this component is attached to exits fog of war for the local player
	 *	
	 *	@param bOnServer - true if GetWorld()->IsServer() would return true  
	 */
	virtual void OnParentBuildingExitFogOfWar(bool bOnServer) PURE_VIRTUAL(IBuildingAttackComp_Turret::OnParentBuildingExitFogOfWar, );

	//------------------------------------------------------------------------------------------

	/* Return whether an actor hit by the overlap test can be aquired as a target */
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor) const PURE_VIRTUAL(IBuildingAttackComp_Turret::CanSweptActorBeAquiredAsTarget, { return nullptr; });

	/** 
	 *	This version also outputs how much total yaw + pitch rotation is required to face Actor. 
	 *	OutRotationRequiredToFace will only be outputted a value if this func returns true.
	 *	This version should only be called if the structure takes into account how much rotation 
	 *	is required to face a target when picking a target.
	 *	
	 *	@param OutRotationRequiredToFace - how much rotation is required to face the target, taking 
	 *	into account yaw and pitch.
	 */
	virtual bool CanSweptActorBeAquiredAsTarget(AActor * Actor, float & OutRotationRequiredToFace) const PURE_VIRTUAL(IBuildingAttackComp_Turret::CanSweptActorBeAquiredAsTarget, { return nullptr; });

	/* Set the attack target */
	virtual void SetTarget(AActor * NewTarget) PURE_VIRTUAL(IBuildingAttackComp_Turret::SetTarget, );
};


//------------------------------------------------------------------------------------------------
//================================================================================================
//	------- Enums -------
//================================================================================================
//------------------------------------------------------------------------------------------------

UENUM()
enum class EDistanceCheckMethod : uint8
{
	/* Prefer targets closer to defense structure */
	Closest, 
	/* Prefer targest far away from defense structure */
	Furtherest
};


/**
 *	Enum for deciding of all the swept actors which one to aquire.
 */
UENUM()
enum class ETargetAquireMethodPriorties : uint8
{
	/**
	 *	Just choose the first actor encounted by iteration. Best performance. 
	 */
	None,
	/** 
	 *	Try and pick actors that would require the building to rotate the least to target  
	 *	If the structure does not have any facing requirement then this is irrelevant. 
	 *	
	 *	This is a good value to have to avoid enemies 'running circles' around your structure 
	 */
	LeastRotationRequired,
	/** 
	 *	Pick actors with an attack over anything else 
	 */
	HasAttack,
	/** 
	 *	Always pick the closest/furtherest target 
	 */
	Distance, 
	/** 
	 *	Pick an actor that has an attack. Pick the one that requires the least rotation to fire 
	 *	at. If no actors have an attack pick the one that requires the least rotation to fire at. 
	 */
	HasAttack_LeastRotationRequired,
	/** 
	 *	Pick the closest/furtherest actor that has an attack. If no actors have an attack then 
	 *	it will pick the closest/furtherest actor 
	 */
	HasAttack_Distance
};


namespace BuldingTurretStatics
{
	/** 
	 *	Call after an overlap test to get the optimal target. 
	 *	
	 *	@param TraceData - result from overlap test 
	 *	@param TargetAquireMethod - how to choose optimal target 
	 *	@param DistanceCheckMethod - whether to prefer closer or further away targets 
	 *	@param bCanAssignNullToTarget - whether it's ok to assign null to Target. For example if 
	 *	there are no actors hit by the overlap test then it is likely or certain OutTarget will be assigned null 
	 */
	static void SelectTargetFromOverlapTest(IBuildingAttackComp_Turret * TurretComp, FOverlapDatum & TraceData,
		ETargetAquireMethodPriorties TargetAquireMethod, EDistanceCheckMethod DistanceCheckMethod, bool bCanAssignNullToTarget = true);
}
