// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingAttackCompAnimInstances.h"

#include "MapElements/BuildingComponents/BuildingAttackComponent_SK.h"


void UBuildingTurretCompAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	bIsOnServer = GetWorld()->IsServer();
}

UBuildingAttackComponent_SK * UBuildingTurretCompAnimInstance::GetOwningAttackComp() const
{
	return CastChecked<UBuildingAttackComponent_SK>(GetOwningComponent());
}

void UBuildingTurretCompAnimInstance::AnimNotify_FireWeapon()
{
	if (bIsOnServer == false)
	{
		return;
	}

	GetOwningAttackComp()->ServerAnimNotify_DoAttack();
}
