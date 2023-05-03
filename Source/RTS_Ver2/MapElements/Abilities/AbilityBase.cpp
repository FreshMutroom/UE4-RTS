// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityBase.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

#include "Statics/DevelopmentStatics.h"
#include "GameFramework/RTSGameState.h"
#include "Statics/Statics.h"


// Sets default values
AAbilityBase::AAbilityBase()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetActorEnableCollision(false);
	bHidden = true;
	bCanBeDamaged = false;

	bReplicates = false;
	bReplicateMovement = false;
	bAlwaysRelevant = false;

	/* These will all need to be set as either true or false in child classes. They are all 
	performance related settings. You can just set them to true to ensure everything works 
	as expected but if you set what you don't need to false then you can save some performance. */
	bHasMultipleOutcomes = EUninitializableBool::Uninitialized;
	bCallAoEStartFunction = EUninitializableBool::Uninitialized;
	bAoEHitsHaveMultipleOutcomes = EUninitializableBool::Uninitialized;
	bRequiresTargetOtherThanSelf = EUninitializableBool::Uninitialized;
	bRequiresLocation = EUninitializableBool::Uninitialized;
	bHasRandomBehavior = EUninitializableBool::Uninitialized;
	bRequiresTickCount = EUninitializableBool::Uninitialized;
}

void AAbilityBase::BeginPlay()
{
	Super::BeginPlay();

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Since this is an info/manager type struct remember that any behavior here wil not be 
	called when the ability should start. Use Server_Begin/Client_Begin */
}

int16 AAbilityBase::GenerateInitialRandomSeed() const
{
	/* Recently changed Min from 0 to INT16_MIN + 1 */
	return FMath::RandRange(INT16_MIN + 1, INT16_MAX - 1);
}

float AAbilityBase::GetRandomFloat(const FRandomStream & RandomStream, float Min, float Max)
{
	/* If thrown then make sure to change this to true */
	assert(HasRandomBehavior());

	const float RandomNumber = UKismetMathLibrary::RandomFloatInRangeFromStream(Min, Max, RandomStream);

	return RandomNumber;
}

int32 AAbilityBase::GetRandomInteger(const FRandomStream & RandomStream, int32 Min, int32 Max)
{
	/* If thrown then make sure to change this to true */
	assert(HasRandomBehavior());

	const int32 RandomNumber = UKismetMathLibrary::RandomIntegerInRangeFromStream(Min, Max, RandomStream);

	return RandomNumber;
}

int32 AAbilityBase::SeedAs16BitTo32Bit(int16 As16Bit)
{
	/*
	0 -> 0
	1 -> 32767
	2 -> 65534
	...
	Might just want to double check this is returning the expected results */
	return (int32)(As16Bit) * (int32)(INT16_MAX);
}

bool AAbilityBase::HasMultipleOutcomes() const
{
	assert(bHasMultipleOutcomes != EUninitializableBool::Uninitialized);
	
	return bHasMultipleOutcomes == EUninitializableBool::True;
}

bool AAbilityBase::IsAoEAbilityForStart() const
{
	assert(bCallAoEStartFunction != EUninitializableBool::Uninitialized);
	
	return bCallAoEStartFunction == EUninitializableBool::True;
}

bool AAbilityBase::AoEHitsHaveMultipleOutcomes() const
{
	assert(bAoEHitsHaveMultipleOutcomes != EUninitializableBool::Uninitialized);
	
	return bAoEHitsHaveMultipleOutcomes == EUninitializableBool::True;
}

bool AAbilityBase::RequiresTargetOtherThanSelf() const
{
	assert(bRequiresTargetOtherThanSelf != EUninitializableBool::Uninitialized);
	
	return bRequiresTargetOtherThanSelf == EUninitializableBool::True;
}

bool AAbilityBase::RequiresLocation() const
{
	assert(bRequiresLocation != EUninitializableBool::Uninitialized);
	
	return bRequiresLocation == EUninitializableBool::True;
}

bool AAbilityBase::HasRandomBehavior() const
{
	assert(bHasRandomBehavior != EUninitializableBool::Uninitialized);
	
	return bHasRandomBehavior == EUninitializableBool::True;
}

bool AAbilityBase::RequiresTickCount() const
{
	return bRequiresTickCount == EUninitializableBool::True;
}

bool AAbilityBase::IsUsable_SelfChecks(ISelectable * AbilityInstigator, EAbilityRequirement & OutMissingRequirement) const
{
	// By default return true
	return true;
}

bool AAbilityBase::IsUsable_SelfChecks(ISelectable * AbilityInstigator) const
{
	EAbilityRequirement DummyVariable;
	return IsUsable_SelfChecks(AbilityInstigator, DummyVariable);
}

bool AAbilityBase::IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target,
	EAbilityRequirement & OutMissingRequirement) const
{
	// By default return true
	return true;
}

bool AAbilityBase::IsUsable_TargetChecks(ISelectable * AbilityInstigator, ISelectable * Target) const
{
	EAbilityRequirement DummyVariable;
	return IsUsable_TargetChecks(AbilityInstigator, Target, DummyVariable);
}

void AAbilityBase::OnListenedProjectileHit(int32 AbilityInstanceUniqueID)
{
}

void AAbilityBase::Server_Begin(FUNCTION_SIGNATURE_ServerBegin)
{
	SERVER_CHECK;
		
	if (MethodOfUsage == EAbilityUsageType::SelectablesActionBar)
	{
		/* Deduct the resource cost and start cooldown timer handle */
		InstigatorAsSelectable->OnAbilityUse(AbilityInfo, GS->GetGameTickCounter());
	}
	else // Assumed SelectablesInventory
	{
		assert(MethodOfUsage == EAbilityUsageType::SelectablesInventory);

		/* Hmmm probably need to pass in a pointer to the inventory or a slot index or something
		so we know what slot to decrement a charge from. Same for client version too */
		InstigatorAsSelectable->OnInventoryItemUse(AuxilleryUsageData, AbilityInfo, GS->GetGameTickCounter());
	}
}

void AAbilityBase::Client_Begin(FUNCTION_SIGNATURE_ClientBegin)
{
	CLIENT_CHECK;

	if (MethodOfUsage == EAbilityUsageType::SelectablesActionBar)
	{
		// Deduct the resource cost and start cooldown timer handle
		InstigatorAsSelectable->OnAbilityUse(AbilityInfo, ServerTickCountAtTimeOfAbility);
	}
	else // Assumed SelectablesInventory
	{
		assert(MethodOfUsage == EAbilityUsageType::SelectablesInventory);

		InstigatorAsSelectable->OnInventoryItemUse(AuxilleryUsageData, AbilityInfo, ServerTickCountAtTimeOfAbility);
	}
}

#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
a;/* 
  Just thinking about the 'something hit on server that hasn't repped to client yet' issue, 
  what we could do is whenever an ability/buff encounters a non valid on client side then 
  we add that to some list along with all the logic. Then when a selectable calls its 
  ISelectable::Setup it checks if it is in the list and if so it executes all that logic 
  (would want to do it after spawn upgrades have applied). 

  It gets complicated though when this happens: 
  - Buff applied 
  - 4th upgrade happens 

  So on spawn we'll need to apply the upgrades in the order they happened (well don't HAVe to) 
  but the key point is say between the 3rd and 4th upgrade we got that buff then between 
  applying the 3rd and 4th upgrade we'll need to apply the buff. 

  */

/* Can remove the AoE and regular funcs, and have only one func that always has the array param. 
If the ability is AoE then the array will have stuff in it. Otherwise it will not */

/* In those GS multicasts make sure that we just return if we are on the server */
#endif
