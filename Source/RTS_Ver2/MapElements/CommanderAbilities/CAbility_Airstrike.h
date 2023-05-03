// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"

#include "Statics/Structs/Structs_6.h"
#include "CAbility_Airstrike.generated.h"

class AWarthog;


/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UCAbility_Airstrike : public UCommanderAbilityBase
{
	GENERATED_BODY()
	
public:

	UCAbility_Airstrike();

	//~ Begin UCommanderAbilityBase interface
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState) override;
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	/* 
	 *	@param Warthog - warthog that was spawned for this ability 
	 *	@param SpawnLocation - world location where warthog was spawned 
	 *	@param TargetLocation - world location ability was used 
	 */
	void RevealFogAtTargetLocation(AWarthog * Warthog, const FVector & SpawnLocation, 
		const FVector & TargetLocation, ETeam InstigatorsTeam, bool bIsServer, ARTSPlayerState * LocalPlayersPlayerState);

	//-----------------------------------------------------------
	//	Data
	//-----------------------------------------------------------

	/* Warthog to spawn */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TSubclassOf<AWarthog> Warthog_BP;

	/* Fog revealing info for target location */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FTemporaryFogRevealEffectInfo TargetLocationTemporaryFogReveal;

	/** 
	 *	This overrides TargetLocationTemporaryFogReveal.Duration. This is the time after the warthog 
	 *	has fired it's final shot to when the fog should stop being revealed. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
	float FogRevealDurationAfterFinalShot;
};
