// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildingAnimInstance.h"

#include "MapElements/Building.h"


void UBuildingAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	OwningBuilding = CastChecked<ABuilding>(GetOwningActor());
}

void UBuildingAnimInstance::AnimNotify_OnDoorFullyOpen()
{
	if (!GetWorld()->IsServer())
	{
		return;
	}
	
	OwningBuilding->AnimNotify_OnDoorFullyOpen();
}

void UBuildingAnimInstance::AnimNotify_OnZeroHealthAnimationFinished()
{
	/* This is now a NOOP */
	
	//if (!GetWorld()->IsServer())
	//{
	//	return;
	//}
	//
	//OwningBuilding->AnimNotify_OnZeroHealthAnimationFinished();
}
