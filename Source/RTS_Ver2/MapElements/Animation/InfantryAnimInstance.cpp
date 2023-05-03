// Fill out your copyright notice in the Description page of Project Settings.

#include "InfantryAnimInstance.h"

#include "MapElements/Infantry.h"


UInfantryAnimInstance::UInfantryAnimInstance()
{

}

void UInfantryAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	
	OwningPawn = CastChecked<AInfantry>(GetOwningActor());
}

void UInfantryAnimInstance::AnimNotify_ExitStealthMode()
{
	OwningPawn->AnimNotify_ExitStealthMode();
}

void UInfantryAnimInstance::AnimNotify_PlayAttackPreparationSound()
{
	OwningPawn->AnimNotify_PlayAttackPreparationSound();
}

void UInfantryAnimInstance::AnimNotify_FireWeapon()
{
	OwningPawn->AnimNotify_FireWeapon();
}

void UInfantryAnimInstance::AnimNotify_OnAttackAnimationFinished()
{
	OwningPawn->AnimNotify_OnAttackAnimationFinished();
}

void UInfantryAnimInstance::AnimNotify_ExecuteContextAction()
{
	OwningPawn->AnimNotify_ExecuteContextAction();
}

void UInfantryAnimInstance::AnimNotify_OnContextAnimationFinished()
{
	OwningPawn->AnimNotify_OnContextAnimationFinished();
}

void UInfantryAnimInstance::AnimNotify_DropOffResources()
{
	OwningPawn->AnimNotify_DropOffResources();
}

void UInfantryAnimInstance::AnimNotify_PickUpInventoryItemOffGround()
{
	OwningPawn->AnimNotify_TryPickUpInventoryItemOffGround();
}

void UInfantryAnimInstance::AnimNotify_OnZeroHealthAnimationFinished()
{
	OwningPawn->AnimNotify_OnZeroHealthAnimationFinished();
}
