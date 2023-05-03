// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/CommonEnums.h"
#include "Statics/Structs_3.h"
#include "AbilityBase.generated.h"

class ISelectable;
class ARTSGameState;
struct FContextButtonInfo;
enum class EAbilityUsageType : uint8;


/* This is just a bool but has a third value called "Uninitialized".
Side note: I get compile errors when this is declared as a UENUM */
enum class EUninitializableBool : uint8
{
	Uninitialized,
	False,
	True
};


/* 

A brief overview of this class and replication in general:

As people who have coded video game network code know, each client's version of the game's state 
is only an approximation of the server's state. It will rarely/never be an exact replica of the 
server's state (excluding lockstep deterministic). 
So with that in mind all the effort put into the Server and Client Begin functions are
to keep the client's visuals as accurate as possibly. This means anything that wants to be 
displayed on the UI and things like visual effects of abilities are what we are trying to 
achieve in the client versions of Begin

The idea is that anything that can change anything about a selectable is sent via a reliable 
multicast RPC from the game state. This means the following are done through the game state: 
- event of an upgrade completing 
- event of a buff/debuff being applied/ticking/falling off
- event of an ability being used
- event of a unit ranking/leveling up 

Everything is sent through the game state to keep RPC ordering intact. The engine guarantees 
that reliable RPCs ordering stays intact only for the same object. An example of this:

code executing on server:
MyInfantry->InfantryRPC();
MyGameState->GameStateRPC();

Here there is no guarantee that the infantry RPC will arrive before the game state one even 
though that is the order they executed on the server.

However: 

MyGameState->GameStateRPC1();
MyGameState->GameStateRPC2();

Here RPC1 will always arrive before RPC2. So that is something that is relied on here when 
implementing all this.

The event of an attack being made is also multicasted but is unreliable. As long as attacks 
just deal damage then this is ok since health is a replicated variable and clients will know 
about the change sooner or later. Of course there's also the issue of the projectile never being 
spawned for the client, but when the alternative outcome is we crash then we can live with that. 
Obvious alternative implementation is to replicate projectiles and I should test this to see 
how it performs.

When an ability is used the server must decide what the outcome of it is e.g. who was hit by 
it and the result of the hit. The result is quite important. Some abilities may only have one 
result. e.g. an ability that deals 30 damage to a target. But some other abilities may be more 
complicated e.g. an ability that increases attack speed by 50% when you are below 30% health, 
otherwise it does nothing. This ability has two outcomes - we need to know whether the target 
was above or below 30% health to know whether to apply the attack speed buff or not. Because the
client's state is always an approximation of the server we cannot reliably do it on the client. 
Therefore it is important we figure out the outcome on the server and pass that on to the clients.
So that's basically what the Result variable is. Some abilities will only have one result 
If you exclude the fact that floating point 
number's add/sub/mul/div can result in different values across different CPUs then client's 
attribute values are actually EXACT replicas of the server's values (can fix this by changing 
to fixed point numbers instead of floats). Perhaps we can live with these inaccuracies though, 
depends on how much server vs client values differ.

Note about floating point vs fixed point: 
Using fixed point would solve the 'different computers do floats in different ways' problem 
that we could get slightly different values between server and client. Of course this is "just" 
a UI problem e.g. one player after an upgrade their UI showsshows they do 7.9 damage while another 
says 7.8. Don't know if I care enough to use fixed point to corect this. But one advantage of fixed point is 
that I can get rid of the DefaultBlahBlahBlah values and the NumTempBlahBlahBlahModifiers variables 
because those only exist because of floating point drift. 

Notes about if I implement mana type resources: 
- Users would not allowed to have abilities that do things like "lose 50% of current mana" 
but instead "gain 150 mana" would be ok. Basically no multipliers, only addition/subraction are 
allowed.

Additional thoughts: something like a periodic bring selectable up to date RPC could be an option. 
But it would have to send a lot of data. Obviously it would be an unreliable multicast RPC. 
Ideas about it: whenever an ability/RankUp/Upgrade effects a selectable, add them to a queue. 
Then each tick remove say 4 units from the queue and send all their data as an RPC

*/


/**
 *	This class should be overridden. It is used to create effects
 *	of when context buttons are clicked, such as calling down an artillery strike or healing an
 *	ally. It is very basic. All functionaility is left to the user to implement. Some notes though:
 *	This is a manager/info type class - only one effect actor is spawned for
 *	every type of effect. This means using things like GetActorLocation() within a class deriving
 *	from this is not useful. Instead a struct for each active effect is used. This stores state for
 *	each instance of the effect active. If you need more state info then create a custom struct
 *	for your custom effect. See ArtilleryStrike.h for an example. Because of this, no actors are
 *	spawned during gameplay. "Spawn" behavior should be implemented inside
 *	Server_Start/Client_Start.
 *
 *	Effects do not replicate. If the effect has random behavior then a random number seed can be
 *	passed to clients by saving the return value of Sever_Start. Otherwise you can ignore it
 *
 *===============================================================================================
 *-----------------------------------------------------------------------------------------------
 *	A non-comprehensive list of things you cannot do with abilities:
 *	- put another ability on cooldown (similar to counterspell in WoW)
 *	- teleportation effects 
 *-----------------------------------------------------------------------------------------------
 *===============================================================================================
 *
 *	List of things already checked by PC and do not need to be in the IsUsable_ functions:
 *	- whether ability is on cooldown 
 *	- whether target is not null, valid, the right affiliation and whether they are ourself 
 *	- whether target is the right targeting type
 *	- maybe some other stuff
 *
 *	TODO inherit from UObject instead of AInfo since not replicated?
 */
UCLASS(NotBlueprintable)
class RTS_VER2_API AAbilityBase : public AInfo
{
	GENERATED_BODY()

public:
	
	AAbilityBase();

protected:

	/* Use Server_Start and Client_Start to start behaviour */
	virtual void BeginPlay() override;

	/* Reference to game state */
	UPROPERTY()
	ARTSGameState * GS;

	//===========================================================================================
	//	Bandwidth Optimization Variables
	//-------------------------------------------------------------------------------------------
	//	You can set these all true and your ability will function fine, but if you don't need 
	//	some of these features then turning them off is an optimization
	//===========================================================================================

	// TODO make these 2 bit flags 

	/** 
	 *	Whether the ability has multiple outcomes that we should tell the client about. This 
	 *	excludes hits on AoE abilities 
	 *
	 *	False = save 1 byte
	 */
	EUninitializableBool bHasMultipleOutcomes;
	
	/** 
	 *	Whether this ability has some kind of AoE effect and the result of what is hits would 
	 *	like to be sent across the wire
	 *	
	 *	Example of true: ghost EMP
	 *	Example of false: ghost cloaking, ghost snipe
	 *
	 *	Currently I have not implemented any kind of persistent AoE behavior e.g. an ability that 
	 *	every second deals 500 damage to stuff within 500 range for 6 seconds. For this there will 
	 *	need to be an RPC or something to handle this 
	 */
	EUninitializableBool bCallAoEStartFunction;

	/** 
	 *	This is only relevant if bCallAoEStartFunction is true. Whether there is more than one 
	 *	outcome to each hit selectable 
	 */
	EUninitializableBool bAoEHitsHaveMultipleOutcomes;

	/** 
	 *	If true the server will send a target for the ability. If the user is always the target 
	 *	e.g. it's a self heal, self buff, etc type ability then you can turn this off
	 *	
	 *	Example of true: ghost snipe
	 *	Example of false: ghost cloaking, ghost EMP 
	 */
	EUninitializableBool bRequiresTargetOtherThanSelf;

	/** 
	 *	Whether we need to send the location of the ability. We always send the user, so if the 
	 *	user's location is enough then you can turn this off. 
	 *	
	 *	Example of true: ghost EMP 
	 *	Example of false: ghost cloaking, ghost snipe 
	 */
	EUninitializableBool bRequiresLocation;

	/** 
	 *	Whether this ability has random behavior, excluding random damage amounts. 
	 *	If only damage amounts are random then you can safely turn this off to save some bandwidth.
	 */
	EUninitializableBool bHasRandomBehavior;

	/* This is currently only needed by abilities that modify selectable resources (mana) */
	EUninitializableBool bRequiresTickCount;

	//==========================================================================================
	//	Other stuff
	//==========================================================================================

	/** 
	 *	Generate initial random int to be used for random stream. 
	 *	
	 *	Note we use a 16 bit integer as the seed to reduce the amount of data sent across the wire 
	 */
	int16 GenerateInitialRandomSeed() const;

	/* Generate random float within range */
	float GetRandomFloat(const FRandomStream & RandomStream, float Min, float Max);

	/** 
	 *	Generate random int32 within range, inclusive of Min and Max
	 *	
	 *	@param Min - the minimum number possible
	 *	@param Max - the maximum number possible
	 *	@return - random number between Min and Max inclusive 
	 */
	int32 GetRandomInteger(const FRandomStream & RandomStream, int32 Min, int32 Max);

public:

	/* Convert the initial seed from 16 bit number to 32 bit number that we can use in the
	FRandomStream constructor */
	static int32 SeedAs16BitTo32Bit(int16 As16Bit);

	bool HasMultipleOutcomes() const;

	/** 
	 *	Whether the ability has an AoE effect at the time it is used e.g. an ability that will 
	 *	put a debuff on every enemy within 500cm. 
	 *	
	 *	I have not implemented anything to do with abilities that have AoE effects that happen 
	 *	after their initial time of use e.g. an ability that does 10 damage in a 500cm radius 
	 *	every sec for 6 sec - this is something I have not added any support for yet 
	 */
	bool IsAoEAbilityForStart() const;

	bool AoEHitsHaveMultipleOutcomes() const;

	/* Whether to send a target over the wire or not */
	bool RequiresTargetOtherThanSelf() const;

	/* Whether to send a location over the wire or not */
	bool RequiresLocation() const;
	
	/* Whether to send a random seed over the wire or not */
	bool HasRandomBehavior() const;

	/* Whether the ability requires knowing what the server's RTS tick counter was at the time 
	the ability happened. Currently unless your ability modifies selectable resources (mana) 
	you do not need this */
	bool RequiresTickCount() const;

	/** 
	 *	This is called for both 1 and 2 click abilities. For 2 click abilities it is called both 
	 *	when the first click is done and when the 2nd is done also (could change this though but 
	 *	I don't forsee any abilities that wouldn't want this called automatically on the 2nd 
	 *	click aswell - it doesn't really make sense for an ability to allow the user to find a 
	 *	target/location for it but then not do those same checks when it comes time to actually 
	 *	activate the ability). Can probably assume instigator is valid. 
	 *	
	 *	SelfChecks isn't a 100% true name. Can check anything like the elapsed time in the match, 
	 *	more accurate is IsUsable_OnButtonClickOrMouseClick
	 *	
	 *	This function returns whether the selectable instigating the ability is in a state that 
	 *	allows it to do so. Some things like cooldown are automatically checked and do not need 
	 *	to be included in this check. 
	 *	If your ability has some custom logic like "only usable when below 40% health" or 
	 *	"only usable if the buff "Haste" is present then this is where to check that stuff now 
	 *	
	 *	@param OutMissingRequirement - if function returns false a reason it did so 
	 *	@return - true if all checks passed 
	 */
	virtual bool IsUsable_SelfChecks(ISelectable * AbilityInstigator, EAbilityRequirement & OutMissingRequirement) const;

	/** Overloaded version for the times we don't care about the reason */
	bool IsUsable_SelfChecks(ISelectable * AbilityInstigator) const;

	/** 
	 *	This is only for abilities that require 2 mouse clicks. 
	 *	Given a instigator and a target return whether the target is a useable target. Possibly 
	 *	can assume both are valid. 
	 *	Currently some stuff is already done by the player controller such as: 
	 *	- targeting type check 
	 *	- whether target is friendly/hostile 
	 *	- whether we can target self 
	 *	- more stuff @See FContextButtonInfo 
	 *	so these things do not need to appear in this function.
	 *	
	 *	TargetChecks isn't 100% accurate. Can check other things too. More accurate would be 
	 *	IsUsable_OnTargetAquiredChecks
	 *	
	 *	Main purpose of this function is to check things on the target, not yourself
	 *	
	 *	@param AbilityInstigator - the actor that instigated the ability, usually a selectable
	 *	@param Target - the target of the ability, usually a selectable
	 *	@param OutMissingRequirement - if function returns false a reason it did so
	 *	@return - true if all checks passed 
	 */
	virtual bool IsUsable_TargetChecks(ISelectable * AbilityInstigator, 
		ISelectable * Target, EAbilityRequirement & OutMissingRequirement) const;
	
	/* Overloaded version for the times when we don't care about the reason */
	bool IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target) const;

	/* ServerTickCountAtTimeOfAbility is not here, can just use GS->GetGameTickCounter() */
#define FUNCTION_SIGNATURE_ServerBegin const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, \
	ISelectable * InstigatorAsSelectable, ETeam InstigatorsTeam, const FVector & Location, AActor * Target, \
	ISelectable * TargetAsSelectable, EAbilityUsageType MethodOfUsage, uint8 AuxilleryUsageData, \
	EAbilityOutcome & OutOutcome, TArray <FAbilityHitWithOutcome> & OutHits, int16 & OutRandomNumberSeed

#define FUNCTION_SIGNATURE_ClientBegin const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, \
	ISelectable * InstigatorAsSelectable, ETeam InstigatorsTeam, const FVector & Location, \
	AActor * Target, ISelectable * TargetAsSelectable, EAbilityUsageType MethodOfUsage, uint8 AuxilleryUsageData, \
	EAbilityOutcome Outcome, const TArray <FHitActorAndOutcome> & Hits, int32 RandomNumberSeed, \
	uint8 ServerTickCountAtTimeOfAbility 

	// Macros to call Super::Server_Begin and Super::Client_Begin
#define SuperServerBegin Super::Server_Begin(AbilityInfo, EffectInstigator, InstigatorAsSelectable, \
	InstigatorsTeam, Location, Target, TargetAsSelectable, MethodOfUsage, AuxilleryUsageData, \
	OutOutcome, OutHits, OutRandomNumberSeed)

#define SuperClientBegin Super::Client_Begin(AbilityInfo, EffectInstigator, InstigatorAsSelectable, \
	InstigatorsTeam, Location, Target, TargetAsSelectable, MethodOfUsage, AuxilleryUsageData, \
	Outcome, Hits, RandomNumberSeed, ServerTickCountAtTimeOfAbility)

	/** 
	 *	Start behaviour server-side.  
	 *	
	 *	@param EffectInstigator - selectable that instigated this effect
	 *	@param InstigatorAsSelectable - instigator as an ISelectable
	 *	@param Team - team instigator is on
	 *	@param StartTransform - where to "spawn" effect. If an ability that targets self then use
	 *	own transform
	 *	@param Target - target of ability, or null if doesn't require target 
	 *	@param MethodOfUsage - indicates how this ability was instigated e.g. context menu button, 
	 *	inventory item use, etc 
	 *	@param AuxilleryUsageData - one thing this is used for is for knowing what inventory
	 *	slot was used if this ability is being used as an inventory item use.
	 *	@param OutOutcome - the outcome of the ability if it has multiple e.g. did Axe's culling 
	 *	blade get the less than 650 health result or the aboove 650 health result? If an ability 
	 *	only has one outcome then this can be ignored.
	 *	@param OutHits - all the selectables the ability hit along with an option result saying 
	 *	the type of hit. If this ability doesn't hit multiple selectables then it can be ignored
	 *	@param OutRandomNumberSeed - random number seed that we used for this ability
	 */
	virtual void Server_Begin(const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, 
		ISelectable * InstigatorAsSelectable, ETeam InstigatorsTeam, const FVector & Location, 
		AActor * Target, ISelectable * TargetAsSelectable, EAbilityUsageType MethodOfUsage, 
		uint8 AuxilleryUsageData, EAbilityOutcome & OutOutcome, 
		TArray <FAbilityHitWithOutcome> & OutHits, int16 & OutRandomNumberSeed);

	/** 
	 *	Start behaviour client-side. 
	 *	You can assume every actor passed in as a param is valid. 
	 *
	 *	@param EffectInstigator - selectable that instigated this effect
	 *	@param InstigatorsTeam - team instigator is on
	 *	@param Location - a world location where we consider the ability was used. Dependent on 
	 *	if bRequiresLocation was set true 
	 *	@param Target - target of ability if any.
	 *	@param MethodOfUsage - indicates how this ability was instigated e.g. context menu button,
	 *	inventory item use, etc
	 *	@param AuxilleryUsageData - one thing this is used for is for knowing what inventory 
	 *	slot was used if this ability is being used as an inventory item use.
	 *	@param Outcome - the outcome of the ability. If the ability only has one outcome then 
	 *	this can be ignored
	 *	@param Hits - if the ability can hit multiple actors, the actors plus the outcome 
	 *	of the hit. If the hits don't have more than one outcome then the outcome can be ignored
	 *	@param RandomNumberSeed - seed from server to use for generating random numbers if 
	 *	required
	 *	@param ServerTickCountAtTimeOfAbility - what GS::GetGameTickCounter() returned on the server 
	 *	when the ability was used on the server. If the ability does not have a selectable 
	 *	resource cost or does not modify anyone's mana in any way then this is actually 
	 *	irrelevant.
	 */
	virtual void Client_Begin(const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, 
		ISelectable * InstigatorAsSelectable, ETeam InstigatorsTeam, const FVector & Location, 
		AActor * Target, ISelectable * TargetAsSelectable, EAbilityUsageType MethodOfUsage, 
		uint8 AuxilleryUsageData, EAbilityOutcome Outcome, const TArray <FHitActorAndOutcome> & Hits, 
		int32 RandomNumberSeed, uint8 ServerTickCountAtTimeOfAbility);

	/** 
	 *	If the ability fires projectiles it can choose to be notified when they hit something. 
	 *	This function is called if the ability chooses to listen on projectiles 
	 *	
	 *	@param AbilityInstanceUniqueID - the unique ID of the ability 
	 */
	virtual void OnListenedProjectileHit(int32 AbilityInstanceUniqueID);
};
