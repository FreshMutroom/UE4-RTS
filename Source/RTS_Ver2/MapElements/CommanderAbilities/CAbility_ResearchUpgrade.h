// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h"
#include "CAbility_ResearchUpgrade.generated.h"

enum class EUpgradeType : uint8;


/**
 *	Instantly fully researches an upgrade for the instigator.
 *
 *	The upgrade you research you probably don't want it to be already researched when you 
 *	use this ability
 */
UCLASS(Abstract, Blueprintable)
class RTS_VER2_API UCAbility_ResearchUpgrade : public UCommanderAbilityBase
{
	GENERATED_BODY()

public:
	
	UCAbility_ResearchUpgrade();

	//~ Begin UCommanderAbilityBase interface
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState) override;
	virtual void Server_Execute(FUNCTION_SIGNATURE_ServerExecute) override;
	virtual void Client_Execute(FUNCTION_SIGNATURE_ClientExecute) override;
	//~ End UCommanderAbilityBase interface

protected:

	//----------------------------------------------------------
	//	Data
	//----------------------------------------------------------

	/* The upgrade to research. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	EUpgradeType UpgradeType;
};
