// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h" // Creates conflicts with the EUnitType I defined
//#include "UObject/ObjectMacros.h"
#include "GameFramework/Info.h"

#include "Settings/ProjectSettings.h"
#include "BuffAndDebuffManager.generated.h"

class ISelectable;
class BuffAndDebuffManager;
class AActor;
struct FTickableBuffOrDebuffInfo;
struct FTickableBuffOrDebuffInstanceInfo;
enum class EBuffOrDebuffApplicationOutcome : uint8;
struct FStaticBuffOrDebuffInfo;
struct FStaticBuffOrDebuffInstanceInfo;


// This is just to remove code that doesn't compile TODO remove this and deal with the errors
// Will need to remove the ticking of buffs/debuffs in AInfantry::Tick and ABuilding::Tick 
#define WITH_TICKING_BUFF_AND_DEBUFF_MANAGER 0

//===========================================================================================
//	------------- File for defining buffs/debuffs ----------------
//
//	A file for defining the behavior of buffs/debuffs. Since they're all statics they can 
//	technically be placed in any file as long as their function signature is correct.
//===========================================================================================

/**
 *	------------------------------------------------------------
 *	List of things that may still need doing or looking at: 
 *	------------------------------------------------------------	
 *
 *	- Need to assign function pointers at some point during GI initialization
 *
 *	- Try at least one compile with BUFF_AND_DEBUFF_ENABLED_GAME false. Make sure everything 
 *	compiles ok and test some gameplay 
 *	
 *	- Implement static buff/debuff updates i.e. put them in the unit's buff/debuff container and 
 *	make sure the HUD knows about them 
 *	
 *	- Consider what to do with buffs/debuffs and replication. Currently there is no replication 
 *	for them
 *	
 *	- Need to implement buffs/debuffs for buildings. Specifically the tick logic + their virtuals. 
 *	Could possibly skip resource spots - we can just assume they cannot receive buffs/debuffs, 
 *	or add another preprocessor like ALLOW_TICKABLE_BUFFS_AND_DEBUFFS_ON_RESOURCE_SPOTS
 */


/* Macros for quickly creating functions that use the 3 signatures of the function pointers 
defined above */

#define FUNCTION_SIGNATURE_TryApplyTo_Static AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable, ISelectable * Target, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo
#define FUNCTION_SIGNATURE_TryApplyTo_Tickable AActor * BuffOrDebuffInstigator, ISelectable * InstigatorAsSelectable, ISelectable * Target, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo
#define FUNCTION_SIGNATURE_DoTick ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo
#define FUNCTION_SIGNATURE_OnRemoved_Static ISelectable * Target, const FStaticBuffOrDebuffInstanceInfo & StateInfo, AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason, const FStaticBuffOrDebuffInfo & BuffOrDebuffInfo
#define FUNCTION_SIGNATURE_OnRemoved_Tickable ISelectable * Target, const FTickableBuffOrDebuffInstanceInfo & StateInfo, AActor * Remover, ISelectable * RemoverAsSelectable, EBuffAndDebuffRemovalReason RemovalReason, const FTickableBuffOrDebuffInfo & BuffOrDebuffInfo


/**
 *	Buff and debuff manager is where to define the behavior of buffs and debuffs. 
 *	This file has nothing to do with abilities. All it has is the behavior for each buff/debuff. 
 *
 *	Assign function pointers in URTSGameInstance::SetupBuffsAndDebuffInfos()
 *
 *	Buffs/debuffs also really have nothing to do with upgrades.
 *
 *	Buffs and debuffs come in two types: static and tickable. Most of your buffs/debuffs will 
 *	likely be tickable. Static buffs/debuffs do not have any tick logic and never expire on 
 *	their own. They require some event to remove them. They give a slight performance increase 
 *	over tickable buffs/debuffs so you should use them when you can.
 *	@See EStaticBuffAndDebuffType and ETickableBuffAndDebuffType
 *
 *	About ticking: if a buff ticks 3 times with a 2sec tick interval it will last 6 seconds, 
 *	and does not tick when first applied but rather 2 secs after. This behavior can easily be 
 *	changed though in the buffs TryApplyTo function. If you want it to tick straight away then 
 *	just add its logic in there
 *
 *	Stackable buffs/debuffs not implemented
 *
 *	Trying this with function pointers instead of virtuals but may switch to virtuals.
 *
 *	===========================================================================================
 *	How to create new buffs/debuffs:
 *
 *	1. Close editor if it is open
 *	2. In CommonEnums.h add an entry to either EStaticBuffAndDebuffType or 
 *	ETickableBuffAndDebuffType. 
 *	3. Copy and paste a group of 3 function signatures below. Do not need the DoTick one if 
 *	you chose to add a static buff/debuff in step 2. 
 *	4. Implement the 3 or 2 functions. 
 *	5. In URTSGameInstance::SetupBuffsAndDebuffInfos add entry for the newly defined buff/debuff. 
 *	6. Compile C++ code. 
 *	7. Inside your game instance blueprint there should be an entry for the buff/debuff you 
 *	just defined. Change the values to suit the behavior of your buff/debuff
 */
UCLASS(NotBlueprintable, NotPlaceable)
class RTS_VER2_API ABuffAndDebuffManager : public AInfo
{
	GENERATED_BODY()

public:

	ABuffAndDebuffManager();

protected:

	virtual void Tick(float DeltaTime) override;

	/* Tick a single buff/debuff. More suited to be a member of FTickableBuffOrDebuffInstanceInfo */
	void TickSingleBuffOrDebuff(FTickableBuffOrDebuffInstanceInfo & InstanceInfo, float DeltaTime);

public:

	//==========================================================================================
	//	Behavior functions for each different buff/debuff
	//==========================================================================================

	//=============================================================
	//	50% move speed increase buff

	static EBuffOrDebuffApplicationOutcome Dash_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome Dash_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome Dash_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//=============================================================
	//	30% move speed increase buff

	static EBuffOrDebuffApplicationOutcome Haste_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome Haste_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome Haste_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//=============================================================
	//	Basic heal over time

	static EBuffOrDebuffApplicationOutcome BasicHealOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome BasicHealOverTime_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome BasicHealOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//=============================================================
	//	Heal over time that increases over time 

	static EBuffOrDebuffApplicationOutcome IncreasingHealOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome IncreasingHealOverTime_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome IncreasingHealOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//==============================================================
	//	'The plague'. Debuff that never expires on its own and increases damage taken by 20%

	static EBuffOrDebuffApplicationOutcome ThePlague_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Static);
	static EBuffOrDebuffRemovalOutcome ThePlague_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Static);


	//===============================================================
	//	Buff applied to the unit that cleanses 'the plague' 

	static EBuffOrDebuffApplicationOutcome CleansersMightOrWhatever_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome CleansersMightOrWhatever_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome CleansersMightOrWhatever_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//===============================================================
	//	A debuff that deals damage over time and heals the instigator if it kills the target

	static EBuffOrDebuffApplicationOutcome PainOverTime_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome PainOverTime_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome PainOverTime_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//===============================================================
	//	A debuff that deals damage over time

	static EBuffOrDebuffApplicationOutcome Corruption_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome Corruption_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome Corruption_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//================================================================
	//	A debuff that deals a massive amount of damage after a while

	static EBuffOrDebuffApplicationOutcome SealFate_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome SealFate_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome SealFate_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//=================================================================
	//	A debuff that deals damage over time and never expires

	static EBuffOrDebuffApplicationOutcome TheCurse_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome TheCurse_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome TheCurse_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//==================================================================
	//	A buff that makes you almost invulnerable

	static EBuffOrDebuffApplicationOutcome NearInvulnerability_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome NearInvulnerability_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome NearInvulnerability_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//==================================================================
	//	Enter stealth mode for 10sec

	static EBuffOrDebuffApplicationOutcome TempStealth_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome TempStealth_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome TempStealth_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	//==================================================================
	//	Regen 50% of max health over 15 sec

	static EBuffOrDebuffApplicationOutcome EatPumpkinEffect_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome EatPumpkinEffect_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome EatPumpkinEffect_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);


	static EBuffOrDebuffApplicationOutcome Beserk_TryApplyTo(FUNCTION_SIGNATURE_TryApplyTo_Tickable);
	static EBuffOrDebuffTickOutcome Beserk_DoTick(FUNCTION_SIGNATURE_DoTick);
	static EBuffOrDebuffRemovalOutcome Beserk_OnRemoved(FUNCTION_SIGNATURE_OnRemoved_Tickable);
};
