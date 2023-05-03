// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BuildingAnimInstance.generated.h"

class ABuilding;


/**
 *	Anim instance to use for buildings
 */
UCLASS(Abstract)
class RTS_VER2_API UBuildingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend ABuilding;

	virtual void NativeBeginPlay() override;
	
protected:

	/* Owner */
	UPROPERTY()
	ABuilding * OwningBuilding;

	/* This will spawn a unit */
	UFUNCTION()
	void AnimNotify_OnDoorFullyOpen();

	UFUNCTION()
	void AnimNotify_OnZeroHealthAnimationFinished();
};
