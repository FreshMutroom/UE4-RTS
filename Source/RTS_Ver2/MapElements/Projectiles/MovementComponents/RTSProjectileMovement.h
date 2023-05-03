// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RTSProjectileMovement.generated.h"

/**
 *	An exact replica of ProjectileMovementComponent except StopSimulating has been overridden
 *	to not call SetUpdatedComponent which is desired behavior with pooled projectiles. This could
 *	have easily been done without needing to create another class but performance may be slightly
 *	better this way. Slightly is pushing it though.
 */
UCLASS()
class RTS_VER2_API URTSProjectileMovement : public UProjectileMovementComponent
{
	GENERATED_BODY()

protected:

	virtual void StopSimulating(const FHitResult & HitResult) override;

};
