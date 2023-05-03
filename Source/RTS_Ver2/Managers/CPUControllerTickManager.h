// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Settings/ProjectSettings.h"
#include "CPUControllerTickManager.generated.h"

class ACPUPlayerAIController;


/**
 *	Tick manager responsible for ticking the CPU player AI controllers.
 *
 *	The tick manager is here so at most one CPU player ticks each tick. CPU player tick behavior
 *	can range from doing next to nothing to doing many sweeps and large amounts of iteration.
 *	By using the tick manager this guarantees at most one AI controller ticks each engine tick.
 *
 *	CPU player tick rate is dependent on engine tick rate i.e. at 120fps twice as many AI controller 
 *	ticks will happen than if at 60fps. This can be easily changed but isn't that big of a deal. 
 *	A good thing to note though is that the number of CPU players in the match does not affect 
 *	how often they will tick i.e. in a 1 CPU player match, that CPU player ticks just as often as 
 *	if they were in a 7 CPU player match.
 */
UCLASS(NotBlueprintable)
class RTS_VER2_API ACPUControllerTickManager : public AInfo
{
	GENERATED_BODY()

	/* How many entries to put in AIControllers array */
	static constexpr uint8 NUM_CONTROLLER_ARRAY_ENTRIES = ProjectSettings::MAX_NUM_PLAYERS - 1;

public:

	ACPUControllerTickManager();

	/* During this function tick a single CPU player AI controller */
	virtual void Tick(float DeltaTime) override;
	
	void RegisterNewCPUPlayerAIController(ACPUPlayerAIController * InAIController);

protected:

	/* Index for AIControllers */
	int32 Index;

	/* CPU Player AI controllers this manager is in charge if ticking */
	TArray < ACPUPlayerAIController *, TFixedAllocator<NUM_CONTROLLER_ARRAY_ENTRIES> > AIControllers;

	/* How many CPU AI controllers have been registered */
	int32 NumAIControllers;
};
