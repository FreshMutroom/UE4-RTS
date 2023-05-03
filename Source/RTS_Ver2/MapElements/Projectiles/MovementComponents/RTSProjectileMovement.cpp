// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSProjectileMovement.h"

void URTSProjectileMovement::StopSimulating(const FHitResult & HitResult)
{
	//SetUpdatedComponent(NULL);

	/* Instead of SetUpdateComponent(NULL) just straight nulling it and then having projectile
	reassign it before resuming ticking works. Without the null OnProjectileStop will get called
	twice. Because updated component is always the same primitive component this shouldn't have a
	any side effects right? */
	UpdatedComponent = nullptr;
	Velocity = FVector::ZeroVector;
	OnProjectileStop.Broadcast(HitResult);
}
