// Fill out your copyright notice in the Description page of Project Settings.

#include "CAbility_ResearchUpgrade.h"

#include "Managers/UpgradeManager.h"


UCAbility_ResearchUpgrade::UCAbility_ResearchUpgrade()
{
	//~ Begin UCommanderAbilityBase variables
	bHasMultipleOutcomes =			ECommanderUninitializableBool::False;
	bCallAoEStartFunction =			ECommanderUninitializableBool::False;
	bAoEHitsHaveMultipleOutcomes =	ECommanderUninitializableBool::False;
	bRequiresSelectableTarget =		ECommanderUninitializableBool::False;
	bRequiresPlayerTarget =			ECommanderUninitializableBool::False;
	bRequiresLocation =				ECommanderUninitializableBool::False;
	bHasRandomBehavior =			ECommanderUninitializableBool::False;
	bRequiresTickCount =			ECommanderUninitializableBool::False;
	//~ End UCommanderAbilityBase variables

	UpgradeType = EUpgradeType::None;
}

void UCAbility_ResearchUpgrade::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	Super::FinalSetup(GameInst, GameState, LocalPlayersPlayerState);

	UE_CLOG(UpgradeType == EUpgradeType::None, RTSLOG, Fatal, TEXT("Commander upgrade ability [%s] "
		"has it's upgrade type set to None"), *GetClass()->GetName());
}

void UCAbility_ResearchUpgrade::Server_Execute(FUNCTION_SIGNATURE_ServerExecute)
{
	SuperServerExecute;

	AbilityInstigator->GetUpgradeManager()->OnUpgradeComplete(UpgradeType, EUpgradeAquireMethod::CommanderAbility);
}

void UCAbility_ResearchUpgrade::Client_Execute(FUNCTION_SIGNATURE_ClientExecute)
{
	SuperClientExecute;

	AbilityInstigator->GetUpgradeManager()->OnUpgradeComplete(UpgradeType, EUpgradeAquireMethod::CommanderAbility);
}
