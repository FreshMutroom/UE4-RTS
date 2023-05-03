// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BuildingAttackCompAnimInstances.generated.h"

class UBuildingAttackComponent_SK;


/**
 *	Anim instance for UBuildingAttackComponent_SK and probably the pitch altering one too
 */
UCLASS(Abstract)
class RTS_VER2_API UBuildingTurretCompAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

#if WITH_EDITOR
	/* Friend for the anim notify checking function */
	friend UBuildingAttackComponent_SK;
#endif

protected:

	virtual void NativeBeginPlay() override;

	/* GetOwningComponent() */
	UBuildingAttackComponent_SK * GetOwningAttackComp() const;

	/** 
	 *	Fire a projectile. I named this the same as the anim notify on the infantry anim instance 
	 *	so the user does not need to create another anim montage if they choose to use the 
	 *	skeletal meshes of infantry as their turret 
	 */
	UFUNCTION()
	void AnimNotify_FireWeapon();

	//----------------------------------------------------------------
	//	Data
	//----------------------------------------------------------------

	/* True if GetWorld()->IsServer() would return true */
	bool bIsOnServer;
};
