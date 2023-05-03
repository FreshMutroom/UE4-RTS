// Fill out your copyright notice in the Description page of Project Settings.

#include "Structs_3.h"

#include "GameFramework/RTSGameState.h"
#include "GameFramework/Selectable.h"
#include "GameFramework/RTSPlayerState.h"


//=========================================================================================
//	>FSelectableIdentifier
//=========================================================================================

FSelectableIdentifier::FSelectableIdentifier() 
	: HitSelectableID(0)
	, HitSelectablesOwnerID(0)
{
	// When these are sent over the wire this appears to be called so have to comment assert
	//assert(0);
}

FSelectableIdentifier::FSelectableIdentifier(ISelectable * InSelectable) 
	: HitSelectableID(InSelectable->GetSelectableID()) 
	, HitSelectablesOwnerID(InSelectable->GetOwnersID())
{
}

FSelectableIdentifier::FSelectableIdentifier(const TScriptInterface<ISelectable>& InSelectable) 
	: HitSelectableID(InSelectable->GetSelectableID())
	, HitSelectablesOwnerID(InSelectable->GetOwnersID())
{
}

FSelectableIdentifier::FSelectableIdentifier(const FAbilityHitWithOutcome & OutcomeStruct) 
	: HitSelectableID(OutcomeStruct.HitSelectableID) 
	, HitSelectablesOwnerID(OutcomeStruct.HitSelectablesOwnerID)
{
}

FSelectableIdentifier::FSelectableIdentifier(ARTSPlayerState * InPlayerState) 
	: HitSelectableID(0)
	, HitSelectablesOwnerID(InPlayerState->GetPlayerIDAsInt())
{
}

AActor * FSelectableIdentifier::GetSelectable(ARTSGameState * GameState) const
{
	return GameState->GetPlayerFromID(HitSelectablesOwnerID)->GetSelectableFromID(HitSelectableID);
}

AActor * FSelectableIdentifier::GetPlayerState(ARTSGameState * GameState) const
{
	return GameState->GetPlayerFromID(HitSelectablesOwnerID);
}


//=========================================================================================
//	>FAbilityHitWithOutcome
//=========================================================================================

FAbilityHitWithOutcome::FAbilityHitWithOutcome()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FAbilityHitWithOutcome::FAbilityHitWithOutcome(ISelectable * InSelectable, EAbilityOutcome InOutcome)
	: Super(InSelectable)
	, Outcome(InOutcome)
{
}

EAbilityOutcome FAbilityHitWithOutcome::GetOutcome() const
{
	return Outcome;
}


//===============================================================================================
//	>FHitActorAndOutcome
//===============================================================================================

FHitActorAndOutcome::FHitActorAndOutcome()
{
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FHitActorAndOutcome::FHitActorAndOutcome(AActor * InActor, EAbilityOutcome InOutcome)
	: HitActor(InActor)
	, Outcome(InOutcome)
{
}

AActor * FHitActorAndOutcome::GetSelectable() const
{
	return HitActor;
}

EAbilityOutcome FHitActorAndOutcome::GetOutcome() const
{
	return Outcome;
}


//===============================================================================================
//	>FHousingResourceState
//===============================================================================================

FHousingResourceState::FHousingResourceState()
{
	// Use ctor with params instead 
	// Commented because crash here on startup after migrating to 4.21
	//assert(0);
}

FHousingResourceState::FHousingResourceState(int32 ResourceTypeAsInt, int32 InUnclampedAmountAlwaysProvided, int32 MaxAmountProvidedAllowed) 
	: AmountConsumed(0)
	, AmountProvided(InUnclampedAmountAlwaysProvided)
{
	if (AmountProvided > MaxAmountProvidedAllowed)
	{
		AmountProvidedClamped = MaxAmountProvidedAllowed;
	}
	else
	{
		AmountProvidedClamped = AmountProvided;
	}
}

void FHousingResourceState::AddConsumption(int16 Amount)
{
	AmountConsumed += Amount;
}

void FHousingResourceState::RemoveConsumption(int16 Amount)
{
	AmountConsumed -= Amount;
}

void FHousingResourceState::AddAmountProvided(int16 AmountToAdd, int32 MaxAmountAllowed)
{
	AmountProvided += AmountToAdd;
	if (AmountProvided > MaxAmountAllowed)
	{
		AmountProvidedClamped = MaxAmountAllowed;
	}
	else
	{
		AmountProvidedClamped = AmountProvided;
	}
}

void FHousingResourceState::RemoveAmountProvided(int16 AmountToRemove, int32 MaxAmountAllowed)
{
	AmountProvided -= AmountToRemove;
	if (AmountProvided > MaxAmountAllowed)
	{
		AmountProvidedClamped = MaxAmountAllowed;
	}
	else
	{
		AmountProvidedClamped = AmountProvided;
	}
}

int32 FHousingResourceState::GetAmountConsumed() const
{
	return AmountConsumed;
}

int32 FHousingResourceState::GetAmountProvidedClamped() const
{
	return AmountProvidedClamped;
}


