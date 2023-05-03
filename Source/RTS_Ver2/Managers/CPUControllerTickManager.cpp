// Fill out your copyright notice in the Description page of Project Settings.

#include "CPUControllerTickManager.h"

#include "Miscellaneous/CPUPlayerAIController.h"


ACPUControllerTickManager::ACPUControllerTickManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Fill with nulls
	AIControllers.Init(nullptr, NUM_CONTROLLER_ARRAY_ENTRIES);
}

void ACPUControllerTickManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACPUPlayerAIController * AICon = AIControllers[Index++ % NUM_CONTROLLER_ARRAY_ENTRIES];

	/* Null check because there will be nulls unless we are playing with the max allowed 
	number of CPU players */
	if (AICon != nullptr)
	{
		AICon->TickFromManager();
	}
}

void ACPUControllerTickManager::RegisterNewCPUPlayerAIController(ACPUPlayerAIController * InAIController)
{
	// Put new AI controller at the end
	AIControllers[NumAIControllers++] = InAIController;
}
