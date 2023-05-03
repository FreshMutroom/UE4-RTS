// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"

#include "MapElements/BuildingComponents/BuildingAttackComp_TurretsBase.h"
#include "BuildingAttackCompStructBase_SM.generated.h"

class IBuildingAttackComp_Turret;


/**
 *	This is the part of a building attack component that rotates yaw e.g. in C&C this is like 
 *	the top half of a turret. This component is for a static mesh. 
 *	It is very likely a IBuildingAttackComp_Turret will be a child of this component.
 */
UCLASS(Blueprintable, ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class RTS_VER2_API UBuildingAttackCompStructBase_SM : public UStaticMeshComponent, public IBuildingAttackComp_TurretsBase
{
	GENERATED_BODY()
	
public:

	UBuildingAttackCompStructBase_SM();

	//~ Begin IBuildingAttackComp_TurretsBase overrides
	virtual void SetupComp(ABuilding * InOwningBuilding, UActorComponent * Turret) override;
	virtual float GetEffectiveComponentYawRotation() const override;
	virtual void OnParentBuildingExitFogOfWar() override;
	//~ End IBuildingAttackComp_TurretsBase overrides

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const override;

	virtual void PreNetReceive() override;

	UFUNCTION()
	void OnRep_RelativeYawRotation();

	// Called right after calling all OnRep notifies (called even when there are no notifies)
	virtual void PostRepNotifies() override;

protected:

	//-------------------------------------------------------------------
	//	Data
	//-------------------------------------------------------------------

	/* Building this comp is attached to */
	ABuilding * OwningBuilding;

	/* Turret comp that is an attach child of this */
	IBuildingAttackComp_Turret * TurretComp;

	/* Relative yaw rotation */
	UPROPERTY(ReplicatedUsing = OnRep_RelativeYawRotation)
	float RelativeYawRotation;

	uint8 bNetUpdateRelativeYawRotation : 1;

	/* The rate at which this base can rotate yaw */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = "0.0001"))
	float RotationRate;

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif
};
