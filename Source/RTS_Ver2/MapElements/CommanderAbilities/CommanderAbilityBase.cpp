// Fill out your copyright notice in the Description page of Project Settings.

#include "CommanderAbilityBase.h"

#include "GameFramework/RTSGameInstance.h"


UCommanderAbilityBase::UCommanderAbilityBase()
{
	bHasMultipleOutcomes = ECommanderUninitializableBool::Uninitialized;
	bCallAoEStartFunction = ECommanderUninitializableBool::Uninitialized;
	bAoEHitsHaveMultipleOutcomes = ECommanderUninitializableBool::Uninitialized;
	bRequiresSelectableTarget = ECommanderUninitializableBool::Uninitialized;
	bRequiresPlayerTarget = ECommanderUninitializableBool::Uninitialized;
	bRequiresLocation = ECommanderUninitializableBool::Uninitialized;
	bHasRandomBehavior = ECommanderUninitializableBool::Uninitialized;
	bRequiresTickCount = ECommanderUninitializableBool::Uninitialized;
	bRequiresDirection = ECommanderUninitializableBool::Uninitialized;
}

void UCommanderAbilityBase::FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState)
{
	GI = GameInst;
	GS = GameState;
	PS = LocalPlayersPlayerState;
}

void UCommanderAbilityBase::Server_Execute(const FCommanderAbilityInfo & AbilityInfo,
	ARTSPlayerState * AbilityInstigator, ETeam InstigatorsTeam, const FVector & Location,
	AActor * AbilityTarget, EAbilityOutcome & OutOutcome, TArray<FAbilityHitWithOutcome> & OutHits, 
	int16 & OutRandomNumberSeed, float & OutDirection)
{
	SERVER_CHECK;

	// Optimization: Could just pass this down instead of refinding it here
	const ECommanderSkillTreeNodeType NodeType = AbilityInstigator->GetCommanderSkillTreeNodeType(&AbilityInfo);
	const FCommanderAbilityTreeNodeInfo & NodeInfo = GI->GetCommanderSkillTreeNodeInfo(NodeType);

	if (NodeInfo.OnlyExecuteOnAquired())
	{
		// It's ok that last param is garbage. Expecting it not to be used
		AbilityInstigator->AquireNextRankForCommanderAbilityInternal(&NodeInfo, AbilityInstigator == PS, -1);
	}
	else
	{
		AbilityInstigator->OnCommanderAbilityUse(AbilityInfo, GS->GetGameTickCounter());
	}
}

void UCommanderAbilityBase::Client_Execute(const FCommanderAbilityInfo & AbilityInfo, 
	ARTSPlayerState * AbilityInstigator, ETeam InstigatorsTeam, const FVector & Location, 
	AActor * AbilityTarget, EAbilityOutcome Outcome, const TArray<FHitActorAndOutcome>& Hits, 
	int32 RandomNumberSeed, uint8 ServerTickCountAtTimeOfAbility, float Direction)
{
	// Might crash here since GetWorld() might not work for UObjects
	CLIENT_CHECK;

	if (AbilityInstigator->BelongsToLocalPlayer())
	{
		const ECommanderSkillTreeNodeType NodeType = AbilityInstigator->GetCommanderSkillTreeNodeType(&AbilityInfo);
		const FCommanderAbilityTreeNodeInfo & NodeInfo = GI->GetCommanderSkillTreeNodeInfo(NodeType);
		
		if (NodeInfo.OnlyExecuteOnAquired())
		{ 
			// It's ok that last param is garbage. Expecting it not to be used
			AbilityInstigator->AquireNextRankForCommanderAbilityInternal(&NodeInfo, false, -1);
		}
		else
		{
			/* This consumes charges and puts it on cooldown, but only the instigating player
			needs to do this */
			AbilityInstigator->OnCommanderAbilityUse(AbilityInfo, ServerTickCountAtTimeOfAbility);
		}
	}
}

int16 UCommanderAbilityBase::GenerateInitialRandomSeed() const
{
	return FMath::RandRange(INT16_MIN + 1, INT16_MAX - 1);
}

int32 UCommanderAbilityBase::SeedAs16BitTo32Bit(int16 Seed16Bits)
{
	// Same as AAbilityBase::SeedAs16BitTo32Bit
	return (int32)(Seed16Bits) * (int32)(INT16_MAX);
}
