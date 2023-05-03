// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h"
#include "CommanderAbilityBase.generated.h"

struct FCommanderAbilityInfo;
class ARTSPlayerState;
enum class ETeam : uint8;
enum class EAbilityOutcome : uint8;
struct FAbilityHitWithOutcome;
struct FHitActorAndOutcome;
class ARTSGameState;
class URTSGameInstance;


// @See EUninitializableBool
enum class ECommanderUninitializableBool : uint8
{
	Uninitialized,
	False,
	True
};


/**
 *	Base class for all abilities that are instigated by the commander. 
 *
 *	Examples of these from other games: 
 *	In C&C Generals: fuel air bomb, artillery strike, any general ability. 
 *	In Startcraft II: nothing that I know of. 
 */
UCLASS(Abstract)
class RTS_VER2_API UCommanderAbilityBase : public UObject
{
	GENERATED_BODY()

	/* 4 macros that help with development when the function signature is constantly changing */

#define FUNCTION_SIGNATURE_ServerExecute const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityInstigator,	\
		ETeam InstigatorsTeam, const FVector & Location, AActor * AbilityTarget,											\
		EAbilityOutcome & OutOutcome, TArray <FAbilityHitWithOutcome> & OutHits, int16 & OutRandomNumberSeed,				\
		float & OutDirection				

#define FUNCTION_SIGNATURE_ClientExecute const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityInstigator,	\
		ETeam InstigatorsTeam, const FVector & Location, AActor * AbilityTarget, EAbilityOutcome Outcome,					\
		const TArray<FHitActorAndOutcome> & Hits, int32 RandomNumberSeed, uint8 ServerTickCountAtTimeOfAbility,			\
		float Direction

#define SuperServerExecute Super::Server_Execute(AbilityInfo, AbilityInstigator, InstigatorsTeam, Location, AbilityTarget, OutOutcome, OutHits, OutRandomNumberSeed, OutDirection);

#define SuperClientExecute Super::Client_Execute(AbilityInfo, AbilityInstigator, InstigatorsTeam, Location, AbilityTarget, Outcome, Hits, RandomNumberSeed, ServerTickCountAtTimeOfAbility, Direction);

public:

	UCommanderAbilityBase();
	
	/* Called during ARTSPlayerState::Client_FinalSetup */
	virtual void FinalSetup(URTSGameInstance * GameInst, ARTSGameState * GameState, ARTSPlayerState * LocalPlayersPlayerState);

	/** 
	 *	Just like AAbilityBase::Server_Begin. Make sure to call Super in your overridden 
	 *	implementation of this. 
	 *	
	 *	@param AbilityTarget - either a ARTSPlayerState if the ability targets a player, or a 
	 *	selectable if it targets a selectable 
	 *	@param OutDirection - the direction of the ability. Abilities like the warthog airstrike 
	 *	use this.
	 */
	virtual void Server_Execute(const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityInstigator,
		ETeam InstigatorsTeam, const FVector & Location, AActor * AbilityTarget, 
		EAbilityOutcome & OutOutcome, TArray <FAbilityHitWithOutcome> & OutHits, int16 & OutRandomNumberSeed, 
		float & OutDirection);

	/* Just like AAbilityBase::Client::Begin */
	virtual void Client_Execute(const FCommanderAbilityInfo & AbilityInfo, ARTSPlayerState * AbilityInstigator,
		ETeam InstigatorsTeam, const FVector & Location, AActor * AbilityTarget, EAbilityOutcome Outcome, 
		const TArray <FHitActorAndOutcome> & Hits, int32 RandomNumberSeed, uint8 ServerTickCountAtTimeOfAbility, 
		float Direction);

protected:

	/**
	 *	Generate initial random int to be used for random stream.
	 *
	 *	Note we use a 16 bit integer as the seed to reduce the amount of data sent across the wire
	 */
	int16 GenerateInitialRandomSeed() const;

	static int32 SeedAs16BitTo32Bit(int16 Seed16Bits);

	//-----------------------------------------------------
	//	------- Data -------
	//-----------------------------------------------------
	
	URTSGameInstance * GI;
	ARTSGameState * GS;
	// Player state of the local player
	ARTSPlayerState * PS;

	ECommanderUninitializableBool bHasMultipleOutcomes;
	ECommanderUninitializableBool bCallAoEStartFunction;
	ECommanderUninitializableBool bAoEHitsHaveMultipleOutcomes;
	ECommanderUninitializableBool bRequiresSelectableTarget;
	ECommanderUninitializableBool bRequiresPlayerTarget;
	ECommanderUninitializableBool bRequiresLocation;
	ECommanderUninitializableBool bHasRandomBehavior;
	ECommanderUninitializableBool bRequiresTickCount;
	ECommanderUninitializableBool bRequiresDirection;

	/* Something like this could be useful if for the resource steal ability: the server calculates 
	how much resources are stolen, then sends it to the player who was targeted by the ability 
	and they could have something on the UI show how much was taken */
	//ECommanderUninitializableBool bRequires4BitsOfMagnitude;
	
	/* Something like this could also be useful for resource steal ability: only the instigator 
	and the target need to know about it and if this was true then instead of a multicast we 
	could send 2 client RPCs - one to instigator and one to target. If the ability doesn't 
	require a target then even better: just send client RPC to instigator */
	//ECommanderUninitializableBool bEveryoneNeedsToKnowAboutIt;
};
