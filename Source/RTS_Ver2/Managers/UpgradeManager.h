// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/CommonEnums.h"
#include "UpgradeManager.generated.h"

class ARTSPlayerState;
class ISelectable;
class AFactionInfo;
class AInfantry;
class ABuilding;
class UUpgradeEffect;
struct FUpgradeTypeArray;

// TODO does this even need to be an actor? UObject probably ok since it doesnt replicate

/* How the player completed the upgrade */
enum class EUpgradeAquireMethod : uint8
{
	ResearchedFromBuilding, 
	CommanderAbility
};


/* Array of pointers to array of upgrade types */
struct FUpgradeTypeArrayPtrArray
{
	TArray<FUpgradeTypeArray*> Array;
};


/* Array containing upgrades. Workaround for non 2D TArrays */
USTRUCT()
struct FUpgradeArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray <EUpgradeType> UpgradesCompleted;

	void AddCompletedUpgrade(EUpgradeType UpgradeType)
	{
		UpgradesCompleted.Emplace(UpgradeType);
	}

	/* Getters and setters */

	TArray <EUpgradeType> & GetArray() { return UpgradesCompleted; }
};


/**
 *  One exists for each player state, both on server and client. On client-side
 *  this is mainly used for instant knowing of whether am upgrade button worked
 *  and for showing UI timers.
 */
UCLASS()
class RTS_VER2_API AUpgradeManager : public AInfo
{
	GENERATED_BODY()

public:

	AUpgradeManager();

private:

	virtual void BeginPlay() override;

	/* Reference to player state that owns this */
	UPROPERTY()
	ARTSPlayerState * PS;

	/* Reference to faction info of owning player state  */
	UPROPERTY()
	AFactionInfo * FI;

	/* Total number of upgrades this player has researched. This will exclude any upgrades 
	that can be obtained through using commander abilities */
	int32 TotalNumUpgradesResearched;

	/* Maps upgrade type to how many times it has been researched, which is
	limited to 0 or 1 at the moment */
	UPROPERTY()
	TMap <EUpgradeType, uint8> UpgradesCompleted;

	/* Maps upgrade type to its effects */
	UPROPERTY()
	TMap <EUpgradeType, UUpgradeEffect *> UpgradeEffects;

	/* Maps building type to array of all upgrades completed for it */
	UPROPERTY()
	TMap <EBuildingType, FUpgradeArray> CompletedBuildingUpgrades;

	/* Maps unit type to array of all upgrades completed for it */
	UPROPERTY()
	TMap <EUnitType, FUpgradeArray> CompletedUnitUpgrades;

	/* Maps from upgrade type to actor (usually building) that has upgrade queued (not necessarily
	researching it yet). Updated server-side only */
	UPROPERTY()
	TMap <EUpgradeType, AActor *> Queued;

	/** Upgrades that the owner's faction has on its roster but have not been researched yet.
	This was added to enable a quick way to get a random unresearched upgrade. 
	This will not include upgrades that are aquired through commander abilities */
	UPROPERTY()
	TArray <EUpgradeType> UnresearchedUpgrades;

	/** 
	 *	Maps upgrade type to an array. The array points to arrays in PS. These arrays contain 
	 *	upgrade types that have not yet been obtained but that are a prerequisite for a 
	 *	building/unit/upgrade. 
	 *
	 *	If the upgrade isn't a prerequisite for anything then it will not have an entry 
	 *	in any of these TMap.
	 */
	TMap<EUpgradeType, FUpgradeTypeArrayPtrArray> HasUpgradePrerequisite_Buildings;
	TMap<EUpgradeType, FUpgradeTypeArrayPtrArray> HasUpgradePrerequisite_Units;
	TMap<EUpgradeType, FUpgradeTypeArrayPtrArray> HasUpgradePrerequisite_Upgrades;

	void SetupReferences();

	void CreateUpgradeClassesAndPopulateUnresearchedArray();

	void SetupUpgradesCompleted();

	void SetupHasUpgradePrerequisiteContainers();

	/** 
	 *	Call 0 param function after a delay
	 *	
	 *	@param TimerHandle - timer handle to use
	 *	@param Function - function to call
	 *	@param Delay - delay for calling function 
	 */
	void Delay(FTimerHandle & TimerHandle, void(AUpgradeManager:: *Function)(), float Delay);

	bool bHasInited;

public:

	void Init();

	/* Return if fully ready to use */
	bool HasInited() const;

	/** 
	 *	Called when a production queue gets an upgrade added to it 
	 *	
	 *	@param UpgradeType - upgrade that was added to queue 
	 *	@param QueueOwner - the actor that owns the queue, usually a building 
	 */
	void OnUpgradePutInProductionQueue(EUpgradeType UpgradeType, AActor * QueueOwner);

	/* 
	 *	Called when a production queue starts researching an upgrade 
	 *	
	 *	@param UpgradeType - type of upgrade to research
	 *	@param ResearchingActor - actor that is researching upgrade (usually building)
	 */
	void OnUpgradeProductionStarted(EUpgradeType UpgradeType, AActor * ResearchingActor);

	/* Called when a production queue removes an upgrade from it not because production completed. 
	Two reasons this could be called: 
	1. the player cancels production of an upgrade 
	2. the researching building reaches zero health */
	void OnUpgradeCancelledFromProductionQueue(EUpgradeType UpgradeType);

	void OnUpgradeComplete(EUpgradeType UpgradeType, EUpgradeAquireMethod HowItWasCompleted = EUpgradeAquireMethod::ResearchedFromBuilding);

protected:

	/**
	 *	@param UpgradeType - upgrade that was just completed
	 *	@return - true if something went from having 1 outstanding upgrade prereq to 0, 
	 *	so there is a possibility it can be produced now 
	 */
	bool OnUpgradeComplete_UpdateSomeContainers(EUpgradeType UpgradeType, EUpgradeAquireMethod HowItWasCompleted);

public:

	/* Apply all completed upgrades to a freshly spawned selectable */
	void ApplyAllCompletedUpgrades(ABuilding * Building);
	void ApplyAllCompletedUpgrades(AInfantry * Infantry);

	/* Return true if upgrade cannot be researched anymore */
	bool HasBeenFullyResearched(EUpgradeType UpgradeType, bool bShowHUDWarning) const;

	/* Returns true if the upgrade is queued in a buildings production queue */
	bool IsUpgradeQueued(EUpgradeType UpgradeType, bool bShowHUDWarning) const;

	/* Returns true if the player has researched every upgrade for their faction */
	bool HasResearchedAllUpgrades() const;

	/* Get a random unresearched upgrade. Assumes there is at least one unresearched upgrade. */
	EUpgradeType Random_GetUnresearchedUpgrade() const;
};
