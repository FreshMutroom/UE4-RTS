// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "InfantryAnimInstance.generated.h"

class AInfantry;


/**
 *	Anim instance infantry should use. Using this class allows use of custom anim notifies. 
 *	Inside the BP's anim graph you will need to make a blueprint of this and add a 
 *	"Slot 'Default Slot'" node and connect its output to the final animation pose node
 */
UCLASS(Abstract)
class RTS_VER2_API UInfantryAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	friend AInfantry;

public:

	UInfantryAnimInstance();

private:

	virtual void NativeBeginPlay() override;

	/* Reference to owning pawn */
	UPROPERTY()
	AInfantry * OwningPawn;

	/* Signals unit should exit stealth mode if they support stealthing */
	UFUNCTION()
	void AnimNotify_ExitStealthMode();

	/* Starts playing a sound for attack e.g. like how oblisk of light warms up */
	UFUNCTION()
	void AnimNotify_PlayAttackPreparationSound();

	/* Called when fire animation fires weapon. */
	UFUNCTION()
	void AnimNotify_FireWeapon();

	/* Called when weapon firing animation is finished. This signals that
	the unit can move again */
	UFUNCTION()
	void AnimNotify_OnAttackAnimationFinished();

	/* Called when a context action animation reaches point where it should
	carry out the action */
	UFUNCTION()
	void AnimNotify_ExecuteContextAction();

	/* Signals that unit can move again from AI ticks */
	UFUNCTION()
	void AnimNotify_OnContextAnimationFinished();

	/* Signals that resources have been dropped off at depot */
	UFUNCTION()
	void AnimNotify_DropOffResources();

	/* Signals that we have picked up an inventory item from the ground */
	UFUNCTION()
	void AnimNotify_PickUpInventoryItemOffGround();

	/* Called by an anim notify when the zero health animation has finished */
	UFUNCTION()
	void AnimNotify_OnZeroHealthAnimationFinished();

};
