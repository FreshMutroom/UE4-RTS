// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameStateAbilities.h"
#include "RTSGameState.h"

#include "Statics/DevelopmentStatics.h"
#include "MapElements/BuildingTargetingAbilities/BuildingTargetingAbilityBase.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//===============================================================================================//
//	This file contains a lot of function definitions for gameplay abilities. They are			 //
//	functions that belong to ARTSGameState. There's so many and it just bloats the RTSGameState  \\
//	cpp file so they have been moved here.														 \\
//===============================================================================================\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


// Creates the ability effect on server, then multicasts it to clients
void ARTSGameState::Server_CreateAbilityEffect(EAbilityUsageType UsageCase, uint8 UsageCaseAuxilleryData,
	const FContextButtonInfo & AbilityInfo, AActor * EffectInstigator, ISelectable * InstigatorAsSelectable,
	ETeam InstigatorsTeam, const FVector & AbilityLocation, AActor * AbilityTarget, ISelectable * TargetAsSelectable)
{
	SERVER_CHECK;

	const EContextButton AbilityType = AbilityInfo.GetButtonType();
	AAbilityBase * AbilityActor = ContextActionEffects[AbilityType];

	const FSelectableIdentifier InstigatorInfo = FSelectableIdentifier(InstigatorAsSelectable);
	const FSelectableIdentifier TargetInfo = FSelectableIdentifier(TargetAsSelectable);

	EAbilityOutcome Outcome;
	TArray <FAbilityHitWithOutcome> Hits;
	int16 Seed16Bits;

	const uint8 TickCount = GetGameTickCounter();

	/* This will create the effect on the server + give us all the information about its outcome */
	AbilityActor->Server_Begin(AbilityInfo, EffectInstigator, InstigatorAsSelectable, InstigatorsTeam,
		AbilityLocation, AbilityTarget, TargetAsSelectable, UsageCase, UsageCaseAuxilleryData,
		Outcome, Hits, Seed16Bits);

	// So if false we assume that it is an selectable's inventory slot use
	const bool bContextMenuUsageCase = (UsageCase == EAbilityUsageType::SelectablesActionBar);
	const uint8 InventorySlotIndex = UsageCaseAuxilleryData;

	/* Now begins a bunch of if/elses that will call 1 of 192 different multicast RPCs. We choose
	the RPC that uses the least bandwidth that gets all the info we need across */
	if (AbilityActor->HasMultipleOutcomes())
	{
		if (AbilityActor->IsAoEAbilityForStart())
		{
			if (AbilityActor->AoEHitsHaveMultipleOutcomes())
			{
				if (AbilityActor->RequiresTargetOtherThanSelf())
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, TargetInfo);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, TargetInfo);
								}
							}
						}
					}
				}
				else // Does not require another target
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, Hits, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Outcome, Hits, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, Hits, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo, Outcome, Hits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(AbilityType, InstigatorInfo, Outcome, Hits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo, Outcome, Hits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo, Outcome, Hits);
								}
							}
						}
					}
				}
			}
			else // Single AoEHit outcomes
			{
				/* Copy Hits array leaving out the outcome bytes because we don't need them */
				TArray < FSelectableIdentifier > HitsWithoutOutcomes;
				HitsWithoutOutcomes.Reserve(Hits.Num());
				for (int32 i = 0; i < Hits.Num(); ++i)
				{
					HitsWithoutOutcomes.Emplace(Hits[i]);
				}

				if (AbilityActor->RequiresTargetOtherThanSelf())
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, TargetInfo);
								}
							}
						}
					}
				}
				else // Does not require another target
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, TickCounter);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, Seed16Bits,TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Outcome, HitsWithoutOutcomes, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(AbilityType,
										InstigatorInfo, Outcome, HitsWithoutOutcomes, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(AbilityType,
										InstigatorInfo, Outcome, HitsWithoutOutcomes);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex,
										InstigatorInfo, Outcome, HitsWithoutOutcomes, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(InventorySlotIndex,
										InstigatorInfo, Outcome, HitsWithoutOutcomes);
								}
							}
						}
					}
				}
			}
		}
		else // Not AoE
		{
			/* If the ability is not an AoE one then there is no reason for it to use this array, so
			it should be empty. */
			assert(Hits.Num() == 0);

			if (AbilityActor->RequiresTargetOtherThanSelf())
			{
				if (AbilityActor->RequiresLocation())
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, AbilityLocation);
							}
						}
					}
				}
				else // Does not require location
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, TargetInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
									Outcome, TargetInfo);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, TargetInfo);
							}
						}
					}
				}
			}
			else // Targets self
			{
				if (AbilityActor->RequiresLocation())
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandom(AbilityType, InstigatorInfo,
									Outcome, AbilityLocation, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, AbilityLocation, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
									Outcome, AbilityLocation);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, AbilityLocation);
							}
						}
					}
				}
				else // Does not require location
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									Outcome, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandom(AbilityType, InstigatorInfo,
									Outcome, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Outcome, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
									Outcome, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo, Outcome, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom(AbilityType, InstigatorInfo, Outcome);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo, Outcome, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo, Outcome);
							}
						}
					}
				}
			}
		}
	}
	else // Does not have multiple outcomes
	{
		if (AbilityActor->IsAoEAbilityForStart())
		{
			if (AbilityActor->AoEHitsHaveMultipleOutcomes())
			{
				if (AbilityActor->RequiresTargetOtherThanSelf())
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Hits, TargetInfo, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
										Hits, TargetInfo);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Hits, TargetInfo);
								}
							}
						}
					}
				}
				else // Does not require another target
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(AbilityType, InstigatorInfo,
										Hits, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										Hits, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										Hits, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										Hits, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										Hits, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(AbilityType, InstigatorInfo,
										Hits, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										Hits, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										Hits, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo, Hits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(AbilityType, InstigatorInfo, Hits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo, Hits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo, Hits);
								}
							}
						}
					}
				}
			}
			else // Single AoE hit outcome
			{
				/* Copy Hits array leaving out the outcome bytes because we don't need them */
				TArray < FSelectableIdentifier > HitsWithoutOutcomes;
				HitsWithoutOutcomes.Reserve(Hits.Num());
				for (int32 i = 0; i < Hits.Num(); ++i)
				{
					HitsWithoutOutcomes.Emplace(Hits[i]);
				}

				if (AbilityActor->RequiresTargetOtherThanSelf())
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, TargetInfo);
								}
							}
						}
					}
				}
				else // Does not require another target
				{
					if (AbilityActor->RequiresLocation())
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, AbilityLocation);
								}
							}
						}
					}
					else // Does not require location
					{
						if (AbilityActor->HasRandomBehavior())
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(AbilityType, InstigatorInfo,
										HitsWithoutOutcomes, Seed16Bits);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, Seed16Bits, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
										HitsWithoutOutcomes, Seed16Bits);
								}
							}
						}
						else
						{
							if (bContextMenuUsageCase)
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo, HitsWithoutOutcomes, TickCount);
								}
								else
								{
									Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(AbilityType, InstigatorInfo, HitsWithoutOutcomes);
								}
							}
							else
							{
								if (AbilityActor->RequiresTickCount())
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo, HitsWithoutOutcomes, TickCount);
								}
								else
								{
									Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo, HitsWithoutOutcomes);
								}
							}
						}
					}
				}
			}
		}
		else // Not AoE
		{
			/* If the ability is not an AoE one then there is no reason for it to use this array, so
			it should be empty. */
			assert(Hits.Num() == 0);

			if (AbilityActor->RequiresTargetOtherThanSelf())
			{
				if (AbilityActor->RequiresLocation())
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									TargetInfo, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandom(AbilityType, InstigatorInfo,
									TargetInfo, AbilityLocation, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									TargetInfo, AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
									TargetInfo, AbilityLocation, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									TargetInfo, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
									TargetInfo, AbilityLocation);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									TargetInfo, AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									TargetInfo, AbilityLocation);
							}
						}
					}
				}
				else // Does not require location
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									TargetInfo, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandom(AbilityType, InstigatorInfo,
									TargetInfo, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									TargetInfo, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
									TargetInfo, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									TargetInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom(AbilityType, InstigatorInfo,
									TargetInfo);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									TargetInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									TargetInfo);
							}
						}
					}
				}
			}
			else // Targets self
			{
				if (AbilityActor->RequiresLocation())
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandom(AbilityType, InstigatorInfo,
									AbilityLocation, Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									AbilityLocation, Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandom(InventorySlotIndex, InstigatorInfo,
									AbilityLocation, Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(AbilityType, InstigatorInfo,
									AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom(AbilityType, InstigatorInfo,
									AbilityLocation);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									AbilityLocation, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom(InventorySlotIndex, InstigatorInfo,
									AbilityLocation);
							}
						}
					}
				}
				else // Does not require location
				{
					if (AbilityActor->HasRandomBehavior())
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(AbilityType, InstigatorInfo,
									Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandom(AbilityType, InstigatorInfo,
									Seed16Bits);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount(InventorySlotIndex, InstigatorInfo,
									Seed16Bits, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandom(InventorySlotIndex, InstigatorInfo,
									Seed16Bits);
							}
						}
					}
					else
					{
						if (bContextMenuUsageCase)
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(AbilityType, InstigatorInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom(AbilityType, InstigatorInfo);
							}
						}
						else
						{
							if (AbilityActor->RequiresTickCount())
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount(InventorySlotIndex, InstigatorInfo, TickCount);
							}
							else
							{
								Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom(InventorySlotIndex, InstigatorInfo);
							}
						}
					}
				}
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------------------------
//	Helper functions and macros for the multicast RPCs
//-----------------------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////////////////

/* Location data to pass in when location is irrelevant */
#define DEFAULT_LOCATION FVector::ZeroVector

/* A value to pass into AAbilityBase::Server_Begin/Client_Begin when the ability being used
is from a selectable's action bar. The value will be ignored */
#define DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO 0

/* Outcome to pass into AAbilityBase::Server_Begin/Client_Begin when the ability has specified 
that is does not require an outcome, meaning this value should be ignored */
#define DEFAULT_OUTCOME EAbilityOutcome::Default

/* Array param when we don't care about it */
#define DEFAULT_HIT_ACTORS_ARRAY TArray<FHitActorAndOutcome>()

/* Random number seed when we don't care about it */
#define DEFAULT_RANDOM_NUMBER_SEED 0

/* Tick counter to pass into AAbilityBase::Client_Begin when the ability says it does not 
require it. This value should be ignored so it doesn't really matter what it is */
#define DEFAULT_TICK_COUNT UINT8_MAX


/* Some macros. There's 96 * 2 of these multicast functions and I do not want to have to retype
this stuff when I need to make a change */
#define RETURN_IF_SERVER if (IsServer()) return; // Already did stuff on server


#define GET_INSTIGATOR_AND_TARGET \
AActor * TheInstigator = AbilityInstigator.GetSelectable(this); \
AActor * TheTarget = AbilityTarget.GetSelectable(this); 


bool ARTSGameState::IsServer() const
{
	return HasAuthority();
}

bool ARTSGameState::AbilityMulticasts_PassedInstigatorValidChecks(AActor * InInstigator)
{
	CLIENT_CHECK;
	
	/* Just return true. Not right TODO */
	return true;

	/* Adding like a 1 sec delay from when a unit spawns to when it can be affected by 
	stuff (its collision becomes enabled)
	is likely going to help with reducing how many times this returns false */
	
	//if (InInstigator != nullptr)
	//{
	//	if (InInstigator->IsPendingKill() == false)
	//	{
	//		return true;
	//	}
	//	else
	//	{
	//		/* Actor pending kill. Return false so the ability isn't executed but do not note 
	//		it down to be executed later. Because the actor is pending kill it will go away 
	//		soon so we don't really care about it */
	//		return false;
	//	}
	//}
	//else
	//{
	//	/* Will likely need some params or something for this func - some way of knowing the 
	//	ability + its targets etc */
	//	NoteDownUnexecutedRPC_Ability();
	//
	//	return false;
	//}
}

bool ARTSGameState::AbilityMulticasts_PassedInstigatorAndTargetValidChecks(AActor * InInstigator, AActor * InTarget)
{
	CLIENT_CHECK;

	/* Just return true. Not right obviously. TODO
	Need to possibly add another bool to AAbilityBase like "bHasSideEffects". If this is true 
	then the ability can affect I guess players or something: basically things that aren't 
	the instigator, target or the array of hits. 
	
	Bools could be like: 
	 - bAffectsInstigatorStats
	 - bAffectsTargetStats 
	 - bAffectsPlayerStats  

	 Something like that. Basically we want to know whether if a target and/or instigator is null 
	 and/or pending kill whether we should hold up executing the ability. Because if not then it 
	 gives us some freedom */
	return true;

	// Code below here seriously needs reworking

	//if (InInstigator != nullptr && InTarget != nullptr)
	//{
	//	const bool bInstigatorPendingKill = InInstigator->IsPendingKill();
	//	const bool bTargetPendingKill = InTarget->IsPendingKill();
	//	
	//	if (!bInstigatorPendingKill && !bTargetPendingKill)
	//	{
	//		return true;
	//	}
	//	else // At least one of them is pending kill
	//	{
	//		if (bInstigatorPendingKill && bTargetPendingKill)
	//		{
	//			
	//		}
	//		// Just instigator pending kill
	//		else if (bInstigatorPendingKill)
	//		{
	//
	//		}
	//		// Just target pending kill
	//		else
	//		{
	//
	//		}
	//	}
	//}
	//else
	//{
	//	if (InInstigator )
	//}
	//
	//return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------------------------
//	Begin Multicast RPCs
//-----------------------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////////////////

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom_Implementation(EContextButton AbilityType,
	FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits,
	FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0);// Implementation not a working one

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);
	AActor * TheTarget = AbilityTarget.GetSelectable(this);

	if (TheInstigator != nullptr && TheTarget != nullptr)
	{
		if ((TheTarget->IsPendingKill() == false) && (TheInstigator->IsPendingKill() == false))
		{
			/* Now check validity of all the hit actors... brutal. This is starting to eat into
			performance more than I am comfortable with. What about this... when an actor
			spawns we add a delay YES delay is the solution to everything ha */
			bool bHaveAllTargetsRepped = true;
			TArray < FHitActorAndOutcome > HitActors; // Struct should contain AActor* and EAbilityOutcome
			HitActors.Reserve(Hits.Num());
			for (const auto & Elem : Hits)
			{
				AActor * HitActor = Elem.GetSelectable(this);
				if (HitActor == nullptr)
				{
#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
					a;/* Actor is null but there's a possiblity it's the case where it is actually
					  being destroyed instead of not repped yet. We need to recognize this case
					  because otherwise the ability will never run on the client and their attributes
					  will become incorrect TODO, but how do we do it? If the ability hit 10 targets
					  but one was pending kill then the ability will never run and those 9 targets
					  never get the effect applied to them just because 1 target was pending kill
					  hmmm have to think of some way to make this work. I mean stuff should rarely
					  be able to do an ability if it isn't above zero health
					  Periodically updating attributes
					  with unreliable multicasts seems like a temping option right now... */
					  // While thinking about that maybe start implementing mana 

					bHaveAllTargetsRepped = false;
					break;
#endif
				}

				HitActors.Emplace(FHitActorAndOutcome(HitActor, Elem.GetOutcome()));
			}

			if (bHaveAllTargetsRepped)
			{
				/* Carry out ability */

				ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
				ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
				const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);
				const int32 RandomNumberSeed32Bits = AAbilityBase::SeedAs16BitTo32Bit(RandomNumberSeed);

				ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
					InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), Location, TheTarget,
					TargetAsSelectable, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
					Outcome, HitActors, RandomNumberSeed32Bits, DEFAULT_TICK_COUNT);
			}
			else
			{
#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
				AddToQueueOrSomething();
#endif
			}
		}

		/* It's assumed that if the actor was not null but was pending kill that it is being
		destroyed and not that it hasn't repped so we will not do anything since it is about
		to go away anyway */
	}
	else
	{
		/* Check if either one is pending kill. If they are then I'm hoping that the reason they
		are no longer repping is because they are about to be destroyed as opposed to them just
		spawning */
		if ((TheTarget != nullptr && TheTarget->IsPendingKill() == false)
			|| (TheInstigator != nullptr && TheInstigator->IsPendingKill() == false))
		{
#if WITH_TICKING_BUFF_AND_DEBUFF_MANAGER
			a;/* If here it basically means that the instigator and/or target has not repped to us yet.
			  We have to note down this ability's logic has not run yet. When the actor reps through to
			  us we will apply this logic on its spawn. Remember to take into account upgrades ordering */

			  // COND_Custom... maybe possible to do all this attribute replication using this along 
			  // with replicated variables.

			  // Putting stuff on cooldown will need to have its logic in the Server_Begin and 
			  // Client_Begin functions. So they can accurately know the cooldown perhaps pass 
			  // a FContextButtonInfo& as a param to Server_Begin and Client_Begin. Calling Super 
			  // could put it on cooldown and it would actually have a use for once. Also Super 
			  // can deduct the SelectableResource(mana) cost. Super should update the HUD, 
			  // so we may need a HUD param added to Begin_*. Or perhaps put it in 
			  // ISelectable::OnContextCommand. Nah Begin_* is best.
			  // Also probably better to tell the HUD that we have used the ability as opposed to 
			  // it constantly checking timer handles each tick... iirc that's whats happening 
			  // right now

			  // Need to implement the rep of ticks + fall off of buffs/debuffs probably 
			  // Some ideas on this:
			  // BuffAndDebuffManager is now an actor that ticks and uses the last tick group. 
			  // Will have a reference to the HUD for fast updating 
			  // Buff/debuffs added freshly go into another array, not the ticking array 
			  // then after the tick they are added to ticking array, so they skip their first tick
			  // Each tick will have to calculate the tick logic on server, group it up, then 
			  // send it to clients
			  // Will help if we identify which buffs/debuffs have more than one outcome so that 
			  // we can avoid sending the 'outcome' byte
			  // Some rough calcs: 50 selectables with 2 buffs each: 
			  // Each selectable needs 2 ID bytes, 1 byte for number of buffs/debuffs without 
			  // multiple outcomes, 1 byte for number of buffs with multiple outcomes, and 1/2 
			  // bytes for each buff/debuff, 1 if single outcome, 2 if multiple outcomes
			  // When like 100 buffs/debuffs tick/falloff in one tick it will be brutal on bandwidth
			  // Try think up what could be done about that
			  // I mean we're sending a lot of reliabe multicast RPCs here, don't know if we 
			  // can do this 
			  // Function signature idea:
			  // FBuffAndDebuffUpdateData

			  // Note: because we do this check here we can remove it from all ability logic
			  // So we can assume validity in them
			AddTOQueueOrSOmething(FUnrunAbilityInfo(LotsOfInfoAboutAbility, LastUpgradeResearched));

			/*
			Stuff done towards using a ticking buff and debuff manager:
			- Changed class to ABuffAndDebuffManager and inherit from AInfo
			- added its tick function
			- added Info.h include
			- commented ObjectMacros.h include

			Stuff that needs to be done:
			- spawn buffdebuffmanager on server only
			- remove buff/debuff ticking logic from AInfantry/ABuilding tick
			*/
#endif
		}
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);
	AActor * TheTarget = AbilityTarget.GetSelectable(this);

	if (TheInstigator != nullptr && TheTarget != nullptr)
	{
		if (TheTarget->IsPendingKill() == false && TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION,
				TheTarget, TargetAsSelectable, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				Outcome, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION,
				nullptr, nullptr, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				Outcome, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);
	AActor * TheTarget = AbilityTarget.GetSelectable(this);

	if (TheInstigator != nullptr && TheTarget != nullptr)
	{
		if ((TheTarget->IsPendingKill() == false) && (TheInstigator->IsPendingKill() == false))
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION,
				TheTarget, TargetAsSelectable, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), Location, nullptr,
				nullptr, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, AAbilityBase::SeedAs16BitTo32Bit(RandomNumberSeed), DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);
			
			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), Location, nullptr,
				nullptr, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION, nullptr,
				nullptr, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget)
{
	RETURN_IF_SERVER;
	GET_INSTIGATOR_AND_TARGET;

	if (AbilityMulticasts_PassedInstigatorAndTargetValidChecks(TheInstigator, TheTarget))
	{
		/* Can carry out ability */
		
		ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
		ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
		 
		/* Would be nice to consider storing the item info pointer in the inventory slot to 
		avoid the effort of getting to ability info */ 
		const FInventorySlotState & InvSlot = TargetAsSelectable->GetInventory()->GetSlotGivenServerIndex(ServerInventorySlotIndex);
		const FInventoryItemInfo * ItemsInfo = GI->GetInventoryItemInfo(InvSlot.GetItemType());
		const FContextButtonInfo * AbilityInfo = ItemsInfo->GetUseAbilityInfo();

		ContextActionEffects[AbilityInfo->GetButtonType()]->Client_Begin(*AbilityInfo, TheInstigator,
			InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), TheTarget->GetActorLocation(),
			TheTarget, TargetAsSelectable, EAbilityUsageType::SelectablesInventory, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
			Outcome, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT); 
	}
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FInventorySlotState & InvSlot = InstigatorAsSelectable->GetInventory()->GetSlotGivenServerIndex(ServerInventorySlotIndex);
			const FContextButtonInfo * AbilityInfo = GI->GetInventoryItemInfo(InvSlot.GetItemType())->GetUseAbilityInfo();

			ContextActionEffects[AbilityInfo->GetButtonType()]->Client_Begin(*AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION,
				nullptr, nullptr, EAbilityUsageType::SelectablesInventory, ServerInventorySlotIndex, 
				Outcome, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed)
{
	// TODO implementation probably not a 100% rigerous implementation

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);

	if (TheInstigator != nullptr)
	{
		if (TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			const FInventorySlotState & InvSlot = InstigatorAsSelectable->GetInventory()->GetSlotGivenServerIndex(ServerInventorySlotIndex);
			const FContextButtonInfo * AbilityInfo = GI->GetInventoryItemInfo(InvSlot.GetItemType())->GetUseAbilityInfo();
			const int32 RandomNumberSeed32Bits = AAbilityBase::SeedAs16BitTo32Bit(RandomNumberSeed);

			ContextActionEffects[AbilityInfo->GetButtonType()]->Client_Begin(*AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), Location,
				nullptr, nullptr, EAbilityUsageType::SelectablesInventory, ServerInventorySlotIndex,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, RandomNumberSeed32Bits, DEFAULT_TICK_COUNT);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandom_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	// TODO implementation probably not a 100% solid one

	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);
	AActor * TheTarget = AbilityTarget.GetSelectable(this);

	if (TheInstigator != nullptr && TheTarget != nullptr)
	{
		if (TheTarget->IsPendingKill() == false && TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION, TheTarget,
				TargetAsSelectable, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				Outcome, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED,
				TickCounterOnServerAtTimeOfAbility);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	// TODO implementation probably not a 100% solid one
	
	RETURN_IF_SERVER;

	AActor * TheInstigator = AbilityInstigator.GetSelectable(this);
	AActor * TheTarget = AbilityTarget.GetSelectable(this);

	if (TheInstigator != nullptr && TheTarget != nullptr)
	{
		if (TheTarget->IsPendingKill() == false && TheInstigator->IsPendingKill() == false)
		{
			ISelectable * InstigatorAsSelectable = CastChecked<ISelectable>(TheInstigator);
			ISelectable * TargetAsSelectable = CastChecked<ISelectable>(TheTarget);
			const FContextButtonInfo & AbilityInfo = GI->GetContextInfo(AbilityType);

			ContextActionEffects[AbilityType]->Client_Begin(AbilityInfo, TheInstigator,
				InstigatorAsSelectable, InstigatorAsSelectable->GetTeam(), DEFAULT_LOCATION, TheTarget,
				TargetAsSelectable, EAbilityUsageType::SelectablesActionBar, DEFAULT_ACTION_BAR_ABILITY_AUXILLERY_INFO,
				DEFAULT_OUTCOME, DEFAULT_HIT_ACTORS_ARRAY, DEFAULT_RANDOM_NUMBER_SEED, 
				TickCounterOnServerAtTimeOfAbility);
		}
	}
	else
	{
		NoteDownUnexecutedRPC_Ability();
	}
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfAbilityUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount_Implementation(EContextButton AbilityType, FSelectableIdentifier AbilityInstigator, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const TArray<FSelectableIdentifier>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_MultipleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, EAbilityOutcome Outcome, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoEMultipleHitOutcomesNoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FAbilityHitWithOutcome>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeAoESingleHitOutcomeNoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const TArray<FSelectableIdentifier>& Hits, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoEWithTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, FSelectableIdentifier AbilityTarget, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetWithLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, const FVector_NetQuantize & Location, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, int16 RandomNumberSeed, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}

void ARTSGameState::Multicast_NotifyOfInventorySlotUse_SingleOutcomeNotAoENoTargetNoLocationNotRandomWithTickCount_Implementation(uint8 ServerInventorySlotIndex, FSelectableIdentifier AbilityInstigator, uint8 TickCounterOnServerAtTimeOfAbility)
{
	assert(0); // No implementation
}



//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Commander Abilities -------
//==============================================================================================
//----------------------------------------------------------------------------------------------

void ARTSGameState::Server_CreateAbilityEffect(const FCommanderAbilityInfo & AbilityInfo,
	ARTSPlayerState * AbilityInstigator, ETeam InstigatorsTeam, const FVector & AbilityLocation,
	AActor * AbilityTarget)
{
	SERVER_CHECK;

	EAbilityOutcome Outcome;
	TArray <FAbilityHitWithOutcome> Hits;
	int16 Seed16Bits;
	float Direction;

	const ECommanderAbility AbilityType = AbilityInfo.GetType();
	UCommanderAbilityBase * EffectObject = GetCommanderAbilityEffectObject(AbilityType);
	const uint8 InsitgatorsID = AbilityInstigator->GetPlayerIDAsInt();
	const uint8 TickCount = GetGameTickCounter();

	EffectObject->Server_Execute(AbilityInfo, AbilityInstigator, AbilityInstigator->GetTeam(),
		AbilityLocation, AbilityTarget, Outcome, Hits, Seed16Bits, Direction);

	FSelectableIdentifier AbilityTargetIdentifier; 
	if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresPlayer)
	{
		ARTSPlayerState * PlayerTarget = CastChecked<ARTSPlayerState>(AbilityTarget);

		AbilityTargetIdentifier = FSelectableIdentifier(PlayerTarget);
	}
	else if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresSelectable)
	{
		AbilityTargetIdentifier = FSelectableIdentifier(CastChecked<ISelectable>(AbilityTarget));
	}
	else if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable)
	{
		/* If a null for target is passed in then we take it that the ability is targeting a 
		location this time */
		if (AbilityTarget != nullptr)
		{
			AbilityTargetIdentifier = FSelectableIdentifier(CastChecked<ISelectable>(AbilityTarget));
		}
		else
		{
			/* This assigns both IDs to 0. What's important is the selectable ID is set to 0. 
			Because 0 is not allowed to be used as a selectable ID the client will be able to 
			deduce from it that the ability is targeting a location this time */
			AbilityTargetIdentifier = FSelectableIdentifier();
		}
	}

	/* Here is where we would go through all the bools like bHasRandomBehavior, 
	bRequiresGameTickCount etc and choose the RPC that uses the least bandwidth just 
	like with selectable's abilities. For now though I'm just going to send the maximum 
	bandwidth RPC TODO change this */
	Multicast_NotifyOfCommanderAbilityUse_EverySingleParam(AbilityType, InsitgatorsID,
		AbilityLocation, AbilityTargetIdentifier, Outcome, Hits, Seed16Bits, TickCount, Direction);
}

void ARTSGameState::Multicast_NotifyOfCommanderAbilityUse_EverySingleParam_Implementation(ECommanderAbility AbilityType, uint8 InstigatorsID, const FVector_NetQuantize & AbilityLocation, const FSelectableIdentifier & AbilityTargetInfo, EAbilityOutcome Outcome, const TArray<FAbilityHitWithOutcome>& Hits, int16 RandomNumberSeed16Bits, uint8 ServerTickCountAtTimeOfAbility, float Direction)
{
	RETURN_IF_SERVER; 

	ARTSPlayerState * AbilityInstigator = GetPlayerFromID(InstigatorsID);
	if (AbilityInstigator != nullptr)
	{
		if (AbilityInstigator->IsPendingKill() == false)
		{
			const FCommanderAbilityInfo & AbilityInfo = GI->GetCommanderAbilityInfo(AbilityType);

			AActor * AbilityTarget = nullptr;
			if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresSelectable)
			{
				AbilityTarget = AbilityTargetInfo.GetSelectable(this);

				if (AbilityTarget == nullptr || AbilityTarget->IsPendingKill())
				{
					NoteDownUnexecutedRPC_CommanderAbility();
					return;
				}
			}
			else if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresPlayer)
			{
				AbilityTarget = AbilityTargetInfo.GetPlayerState(this);

				if (AbilityTarget == nullptr || AbilityTarget->IsPendingKill())
				{
					// Because it's a player state probably won't ever be able to resolve this: 
					// that player state is gone and never coming back
					NoteDownUnexecutedRPC_CommanderAbility();
					return;
				}
			}
			else if (AbilityInfo.GetTargetingMethod() == EAbilityTargetingMethod::RequiresWorldLocationOrSelectable)
			{
				// 0 is used to signal 'ability is targeting a location, not a selectable'
				if (AbilityTargetInfo.GetSelectableID() != 0)
				{
					AbilityTarget = AbilityTargetInfo.GetSelectable(this);

					if (AbilityTarget == nullptr || AbilityTarget->IsPendingKill())
					{
						NoteDownUnexecutedRPC_CommanderAbility();
						return;
					}
				}
			}

			/* Check every actor hit by the AoE of the ability is valid */
			bool bHaveAllAoETargetsRepped = true;
			TArray<FHitActorAndOutcome> HitActors;
			HitActors.Reserve(Hits.Num());
			for (const auto & Elem : Hits)
			{
				AActor * HitActor = Elem.GetSelectable(this);
				if (HitActor == nullptr)
				{
					/* @See the massive comment I had in 
					Multicast_NotifyOfAbilityUse_MultipleOutcomeAoEMultipleHitOutcomesWithTargetWithLocationRandom_Implementation */
					bHaveAllAoETargetsRepped = false;
					break;
				}
				else
				{
					HitActors.Emplace(FHitActorAndOutcome(HitActor, Elem.GetOutcome()));
				}
			}

			if (bHaveAllAoETargetsRepped == false)
			{
				NoteDownUnexecutedRPC_CommanderAbility();
				return;
			}

			CommanderAbilityEffects[AbilityType]->Client_Execute(AbilityInfo, AbilityInstigator,
				AbilityInstigator->GetTeam(), AbilityLocation, AbilityTarget, Outcome, HitActors,
				AAbilityBase::SeedAs16BitTo32Bit(RandomNumberSeed16Bits), ServerTickCountAtTimeOfAbility, 
				Direction);
		}
	}
	// Not really sure what to do if it is null or pending kill. We're basically buggered then 
	// since it will never come out of being null/pending.kill. Gotta make sure that when a 
	// player leaves the match we keep their player state around for like 20sec or something 
	// so all the RPCs with it get sent. Maybe the engine does this already. Bear in mind that 
	// we're sending these RPCs through the GS so it's possible the engine only makes sure that 
	// RPCs on the player state's channel get sent - the fact that the PS is a param in a GS 
	// RPC means it might not wait around for that to send before destroying the PS. The game 
	// mode/state I think remembers player 
	// states in case players rejoin the match so could look at that code and see if I can use 
	// it to do this. But yeah perhaps the engine does this already. Perhaps I'll never encounter 
	// a null/pending.kill here. Testing will reveal this.
}



//----------------------------------------------------------------------------------------------
//==============================================================================================
//	------- Special Building Targeting Abilities -------
//==============================================================================================
//----------------------------------------------------------------------------------------------


void ARTSGameState::Server_CreateAbilityEffect(const FBuildingTargetingAbilityStaticInfo & AbilityInfo, AInfantry * AbilityInstigator, ABuilding * AbilityTarget)
{
	SERVER_CHECK;

	EAbilityOutcome Outcome;
	int16 Seed16Bits;

	UBuildingTargetingAbilityBase * EffectObject = AbilityInfo.GetEffectObject();

	EffectObject->Server_Execute(AbilityInfo, AbilityInstigator, AbilityTarget, Outcome, Seed16Bits);

	/* At this point you would go through all the if/elses and choose the most bandwidth 
	efficient RPC, but I'm just calling this one that does all params for speedier development */
	Multicast_NotifyOfBuildingTargetingAbilityUse_EverySingleParam(AbilityInfo.GetAbilityType(),
		FSelectableIdentifier(AbilityInstigator), FSelectableIdentifier(AbilityTarget), 
		Outcome, Seed16Bits);
}

void ARTSGameState::Multicast_NotifyOfBuildingTargetingAbilityUse_EverySingleParam_Implementation(EBuildingTargetingAbility AbilityType, const FSelectableIdentifier & AbilityInstigatorInfo, const FSelectableIdentifier & AbilityTargetInfo, EAbilityOutcome Outcome, int16 RandomNumberSeed16Bits)
{
	RETURN_IF_SERVER;

	/* Check instigator and target are valid */
	AActor * AbilityInstigator = AbilityInstigatorInfo.GetSelectable(this);
	AActor * AbilityTarget = AbilityTargetInfo.GetSelectable(this);
	if (AbilityInstigator != nullptr && AbilityTarget != nullptr)
	{
		if (AbilityInstigator->IsPendingKill() == false && AbilityTarget->IsPendingKill() == false)
		{
			const FBuildingTargetingAbilityStaticInfo & AbilityInfo = GI->GetBuildingTargetingAbilityInfo(AbilityType);
			AbilityInfo.GetEffectObject()->Client_Execute(AbilityInfo, AbilityInstigator, CastChecked<ABuilding>(AbilityTarget),
				Outcome, UBuildingTargetingAbilityBase::SeedAs16BitTo32Bit(RandomNumberSeed16Bits));
		}
	}
	else
	{
		NoteDownUnexecutedRPC_BuildingTargetingAbility();
	}
}
