// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Structs_3.generated.h"

class AActor;
class ARTSGameState;
class ISelectable;
struct FAbilityHitWithOutcome;
enum class EAbilityOutcome : uint8;
class ARTSPlayerState;


/** 
 *	This struct contains enough info to derive which selectable we are talking about. It is used 
 *	to be sent over the wire 
 */
USTRUCT()
struct FSelectableIdentifier
{
	GENERATED_BODY()

protected:

	/* Instead of using a raw pointer which I assume is 8 bytes we send 1 byte for the
	selectable and 1 byte for the selectable's owner. That is enough information to derive the
	selectable */

	/* The selectable ID of the selectable that was hit */
	UPROPERTY()
	uint8 HitSelectableID;

	/* The owner ID of the hit selectable */
	UPROPERTY()
	uint8 HitSelectablesOwnerID;

public:

	// Default ctor never to be called
	FSelectableIdentifier();
	
	FSelectableIdentifier(ISelectable * InSelectable);
	FSelectableIdentifier(const TScriptInterface<ISelectable> & InSelectable);

	/* This ctor is for removing the outcome data when we don't need to send it over the wire */
	FSelectableIdentifier(const FAbilityHitWithOutcome & OutcomeStruct);

	/* I also use this struct at times to store a player state ID even though it would be 
	more efficient just to send a single uint8. This is the ctor for player states */
	FSelectableIdentifier(ARTSPlayerState * InPlayerState);

	/* Return the selectable ID this struct is holding */
	uint8 GetSelectableID() const { return HitSelectableID; }

	/* Returns the selectable that this info is for. */
	AActor * GetSelectable(ARTSGameState * GameState) const;

	/* Returns the player that owns the selectable, or if this is just being used to store a 
	player state then returns that player state */
	AActor * GetPlayerState(ARTSGameState * GameState) const;
};


/* This struct is info about a hit actor for an ability + the result of the it */
USTRUCT()
struct FAbilityHitWithOutcome : public FSelectableIdentifier
{
	GENERATED_BODY()

protected:

	/* This is the custom result of the ability casted to a uint8. e.g. Success, Faliure,
	TargetAbove30PercentCase, etc.
	If the ability only ever has one outcome then this variable is irrelevant */
	UPROPERTY()
	EAbilityOutcome Outcome;

public:

	// Default ctor never to be called
	FAbilityHitWithOutcome();

	/**
	 *	Constructor
	 *
	 *	@param InHitSelectablesID - selectable ID for selectable that was hit
	 *	@param InHitSelectablesOwnerID - ID of the selectables owner
	 *	@param InOutcome - the result of the ability
	 */
	FAbilityHitWithOutcome(ISelectable * InSelectable, EAbilityOutcome InOutcome);

	EAbilityOutcome GetOutcome() const;
	
	template <typename T> 
	T GetOutcome() const
	{
		return static_cast<T>(Outcome);
	}
};


/* This is just an FAbilityHitWithOutcome but we actually have an actual AActor pointer to '
the actor as opposed to just 2 uint8 */
USTRUCT()
struct FHitActorAndOutcome
{
	GENERATED_BODY()

protected:

	AActor * HitActor;

	EAbilityOutcome Outcome;

public:

	// Never call this ctor
	FHitActorAndOutcome();

	FHitActorAndOutcome(AActor * InActor, EAbilityOutcome InOutcome);

	AActor * GetSelectable() const;
	EAbilityOutcome GetOutcome() const;

	template <typename T>
	T GetOutcome() const
	{
		return static_cast<T>(Outcome);
	}
};


/* Holds how much of a housing resource we are consuming and how much we are providing */
USTRUCT()
struct FHousingResourceState
{
	GENERATED_BODY()

protected:

	/* Amount we are consuming */
	int32 AmountConsumed;

	/* Amount we are providing. Clamped to limits */
	int32 AmountProvidedClamped;

	/* Amount we are providing disregarding any limits */
	int32 AmountProvided;

public:

	FHousingResourceState();

	/* @param ResourceTypeAsInt - EHousingResourceType converted using
	Statics::HousingResourceTypeToArrayIndex */
	FHousingResourceState(int32 ResourceTypeAsInt, int32 InUnclampedAmountAlwaysProvided,
		int32 MaxAmountProvidedAllowed);

	/* Basic adjusting functions */
	void AddConsumption(int16 Amount);
	void RemoveConsumption(int16 Amount);
	/* @param MaxAmountAllowed - maximum amount of this resource we are allowed e.g. in SCII for
	housing it is 200 */
	void AddAmountProvided(int16 AmountToAdd, int32 MaxAmountAllowed);
	void RemoveAmountProvided(int16 AmountToRemove, int32 MaxAmountAllowed);

	/* Get how much we are consuming e.g. if we have 10 zealots this would be 20 */
	int32 GetAmountConsumed() const;

	/* Get how much we are providing e.g. if we have 2 pylons this would be 16.
	This includes any extra amounts the faction always provides.
	This value is also clamped to take into account limits e.g. if I have 4000 pylons this
	would still only return 200 */
	int32 GetAmountProvidedClamped() const;
};

