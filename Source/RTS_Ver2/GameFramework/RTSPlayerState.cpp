// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSPlayerState.h"
#include "UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Public/EngineUtils.h"
#include "Public/TimerManager.h"
#include "Components/AudioComponent.h"

#include "MapElements/Building.h"
#include "MapElements/ResourceSpot.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameInstance.h"
#include "Managers/FogOfWarManager.h"
#include "Managers/UpgradeManager.h"
#include "Managers/ObjectPoolingManager.h"
#include "UI/RTSHUD.h"
#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerController.h"
#include "Settings/ProjectSettings.h"
#include "MapElements/Infantry.h"
#include "Miscellaneous/CPUPlayerAIController.h"
#include "MapElements/InventoryItem.h"
#include "UI/InMatchWidgets/CommanderSkillTreeNodeWidget.h"
#include "UI/InMatchWidgets/CommanderSkillTreeWidget.h"
#include "MapElements/CommanderAbilities/CommanderAbilityBase.h" // For the FinalSetup() call


ARTSPlayerState::ARTSPlayerState()
{
	// Actor component setup

	BuildingBuiltAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Building Built Audio Comp"));
	BuildingBuiltAudioComp->SetUISound(true);
	BuildingBuiltAudioComp->bAllowSpatialization = false;

	// Doing this so editor will not give warnings 
	SetRootComponent(BuildingBuiltAudioComp);

	UnitBuiltAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Unit Built Audio Comp"));
	UnitBuiltAudioComp->SetUISound(true);
	UnitBuiltAudioComp->bAllowSpatialization = false;

	// Default values

	/* Default initialize TMap. */
	ResourceDepots.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (int32 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceDepots.Emplace(Statics::ArrayIndexToResourceType(i), FBuildingSet());
	}

	bUseCustomPlayerNames = true;

	IDMap.Init(nullptr, (int16)(ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER + 1));

	Affiliation = EAffiliation::Unknown;
}

void ARTSPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void ARTSPlayerState::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
}

void ARTSPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Different resources 
	DOREPLIFETIME_CONDITION(ARTSPlayerState, Resource_Cash, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ARTSPlayerState, Resource_Sand, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ARTSPlayerState, Experience, COND_OwnerOnly);
}

void ARTSPlayerState::UpdateClosestDepots(EResourceType ResourceType, ABuilding * Depot, bool bWasBuilt)
{
	if (bWasBuilt)
	{
		for (const auto & Spot : GS->GetResourceSpots(ResourceType))
		{
			if (Spot->GetClosestDepot(this) == nullptr)
			{
				/* If here then this was first depot built by player */
				Spot->SetClosestDepot(this, Depot);
			}
			else
			{
				/* Check if recently built building is now closest */
				const float DistanceSqr = Statics::GetPathDistanceSqr(Spot, Depot);
				const float CurrBestDistSqr = Statics::GetPathDistanceSqr(Spot, Spot->GetClosestDepot(this));
				if (DistanceSqr < CurrBestDistSqr)
				{
					Spot->SetClosestDepot(this, Depot);
				}
			}
		}
	}
	else
	{
		for (const auto & Spot : GS->GetResourceSpots(ResourceType))
		{
			/* If destroyed depot was the closest depot then find next closest */
			if (Spot->GetClosestDepot(this) == Depot)
			{
				ABuilding * ClosestDepot = nullptr;
				float BestDistanceSoFarSqr = FLT_MAX;
				for (const auto & OtherDepot : ResourceDepots[ResourceType].GetSet())
				{
					const float DistanceSqr = Statics::GetPathDistanceSqr(Spot, OtherDepot);
					if (DistanceSqr < BestDistanceSoFarSqr)
					{
						ClosestDepot = OtherDepot;
						BestDistanceSoFarSqr = DistanceSqr;
					}
				}

				Spot->SetClosestDepot(this, ClosestDepot);
			}
		}
	}
}

ABuilding * ARTSPlayerState::GetProductionBuilding(const FContextButton & Button, bool bShowHUDWarning, EProductionQueueType & OutQueueType)
{
	/* If for building then check the persistent queues */
	if (Button.IsForBuildBuilding())
	{
		for (const auto & Building : PersistentQueueSupportingBuildings)
		{
			if (Building->GetPersistentProductionQueue()->HasRoom(this))
			{
				OutQueueType = EProductionQueueType::Persistent;
				return Building;
			}
		}

		/* We are trying to build a building from the HUD persistent panel but no construction yard
		type buildings exist. This should not happen unless it is a networked situation */
		assert(GetWorld()->IsServer());
		OutQueueType = EProductionQueueType::None;
		return nullptr;
	}

	assert(Button.IsForTrainUnit() || Button.IsForResearchUpgrade());

	const TArray < ABuilding * > & PotentialBuildings = ProductionCapableBuildings[Button].GetArray();

	if (PotentialBuildings.Num() == 0)
	{
		/* Kind of not expected to reach here - it means that none of our buildings can produce
		what was requested. If this is the case then the player should never have been allowed
		to click the button on the HUD in the first place */
		if (bShowHUDWarning)
		{
			OnGameWarningHappened(EGameWarning::CannotProduce);
		}

		OutQueueType = EProductionQueueType::None;
		return nullptr;
	}
	else
	{
		for (const auto & Building : PotentialBuildings)
		{
			if (Building->GetProductionQueue()->HasRoom(this))
			{
				OutQueueType = EProductionQueueType::Context;
				return Building;
			}
		}

		/* If here then all production queues are full and we cannot add new item to queue */

		if (bShowHUDWarning)
		{
			Button.GetButtonType() == EContextButton::Train
				? OnGameWarningHappened(EGameWarning::TrainingQueueFull)
				: OnGameWarningHappened(EGameWarning::BuildingInProgress);
		}

		OutQueueType = EProductionQueueType::None;
		return nullptr;
	}
}

int32 & ARTSPlayerState::GetReplicatedResourceVariable(EResourceType ResourceType)
{
	switch (ResourceType)
	{
		case EResourceType::Cash:
		{
			return Resource_Cash;
		}
		case EResourceType::Sand:
		{
			return Resource_Sand;
		}
		default:
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unrecognized resource type: \"%s\". If added new resource "
				"recently make sure to give it its own replicated int32 and OnRep function, then "
				"return the variable inside this switch"), 
				TO_STRING(EResourceType, ResourceType));

			// Just to compile. will never be reached while testing
			return Resource_Cash;
		}
	}
}

ARTSPlayerState::FunctionPtrType ARTSPlayerState::GetReplicatedResourceOnRepFunction_DEPRECIATED(EResourceType ResourceType) const
{
	switch (ResourceType)
	{
		case EResourceType::Cash:
		{
			return &ARTSPlayerState::OnRep_ResourcesCash;
		}
		case EResourceType::Sand:
		{
			return &ARTSPlayerState::OnRep_ResourcesSand;
		}
		default:
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unrecognized resource type. If added new resource "
				"recently make sure to give it its own replicated int32 and OnRep function, then "
				"return the func inside this switch"));
			return nullptr;
		}
	}
}

ARTSPlayerState::FunctionPtrType ARTSPlayerState::GetReplicatedResourceOnRepFunction(EResourceType ResourceType) const
{
	switch (ResourceType)
	{
		case EResourceType::Cash:
		{
			return &ARTSPlayerState::Server_PostResourceChange_Cash;
		}
		case EResourceType::Sand:
		{
			return &ARTSPlayerState::Server_PostResourceChange_Sand;
		}
		default:
		{
			UE_LOG(RTSLOG, Fatal, TEXT("Unrecognized resource type. If added new resource "
				"recently make sure to give it its own replicated int32 and OnRep function, then "
				"return the func inside this switch"));
			return nullptr;
		}
	}
}

int32 ARTSPlayerState::ResourceTypeToArrayIndex(EResourceType ResourceType) const
{
	return static_cast<int32>(ResourceType) - 1;
}

EResourceType ARTSPlayerState::ArrayIndexToResourceType(int32 ResourceArrayIndex) const
{
	return static_cast<EResourceType>(ResourceArrayIndex + 1);
}

void ARTSPlayerState::OnRep_ResourcesCash()
{
	/* Could be null when setting up for match - resource value is set on server but client has
	not created their HUD yet. Will also be null for CPU players */
	if (HUDWidget != nullptr)
	{
		const int32 Index = ResourceTypeToArrayIndex(EResourceType::Cash);
		ResourceArray[Index] = Resource_Cash;
		
		HUDWidget->OnResourceChanged(EResourceType::Cash, OldResourceArray[Index], ResourceArray[Index]);

		OldResourceArray[Index] = ResourceArray[Index];
	}
}

void ARTSPlayerState::OnRep_ResourcesSand()
{
	if (HUDWidget != nullptr)
	{
		const int32 Index = ResourceTypeToArrayIndex(EResourceType::Sand);
		ResourceArray[Index] = Resource_Sand;
		
		HUDWidget->OnResourceChanged(EResourceType::Sand, OldResourceArray[Index], ResourceArray[Index]);

		OldResourceArray[Index] = ResourceArray[Index];
	}
}

void ARTSPlayerState::Server_PostResourceChange_Cash()
{
	const int32 Index = ResourceTypeToArrayIndex(EResourceType::Cash);
	ResourceArray[Index] = Resource_Cash;
	
	if (HUDWidget != nullptr)
	{
		HUDWidget->OnResourceChanged(EResourceType::Cash, OldResourceArray[Index], ResourceArray[Index]);

		OldResourceArray[Index] = ResourceArray[Index];
	}
}

void ARTSPlayerState::Server_PostResourceChange_Sand()
{
	const int32 Index = ResourceTypeToArrayIndex(EResourceType::Sand);
	ResourceArray[Index] = Resource_Sand;

	if (HUDWidget != nullptr)
	{
		HUDWidget->OnResourceChanged(EResourceType::Sand, OldResourceArray[Index], ResourceArray[Index]);

		OldResourceArray[Index] = ResourceArray[Index];
	}
}

void ARTSPlayerState::OnRep_Experience()
{
	/* Check if not max level yet */
	if (Rank < LevelingUpOptions::MAX_COMMANDER_LEVEL)
	{
		const uint8 OldRank = Rank;
		
		const FCommanderLevelUpInfo * NextLevelInfo = &FI->GetCommanderLevelUpInfo(Rank + 1);

		/* Check if have leveled up (possibly more than once) */
		while (Experience >= NextLevelInfo->GetCumulativeExperienceRequired())
		{
			Rank++;
			
			if (Rank == LevelingUpOptions::MAX_COMMANDER_LEVEL)
			{
				break;
			}
			else
			{
				NextLevelInfo = &FI->GetCommanderLevelUpInfo(Rank + 1);
			}
		}

		// Apply the effects of each level gained excluding the last one gained, so this loop does 
		// nothing if you only gained one level which is usually the case
		for (uint8 i = OldRank + 1; i < Rank; ++i)
		{
			OnLevelUp_NotLastForEvent(i - 1, FI->GetCommanderLevelUpInfo(i));
		}

		if (OldRank < Rank)
		{
			/* Apply the effects of leveling up */
			OnLevelUp_LastForEvent(OldRank, Rank, FI->GetCommanderLevelUpInfo(Rank));
		}
		else // Did not level up
		{
			if (bIsABot == false && bBelongsToLocalPlayer == true)
			{
				/* Update HUD */
				const float ExperienceTowardsNextRank = (Rank == 0) ? Experience : Experience - FI->GetCommanderLevelUpInfo(Rank).GetCumulativeExperienceRequired();
				HUDWidget->OnCommanderExperienceGained(ExperienceTowardsNextRank, NextLevelInfo->GetExperienceRequired());
			}
		}
	}
}

void ARTSPlayerState::OnLevelUp_NotLastForEvent(uint8 OldRank, const FCommanderLevelUpInfo & LevelGainedsInfo)
{
	// Add to skill points
	NumUnspentSkillPoints += LevelGainedsInfo.GetSkillPointGain();
}

void ARTSPlayerState::OnLevelUp_LastForEvent(uint8 OldRank, uint8 NewRank, const FCommanderLevelUpInfo & LevelGainedsInfo)
{
	// Add to skill points
	NumUnspentSkillPoints += LevelGainedsInfo.GetSkillPointGain();
	
	if (bBelongsToLocalPlayer)
	{
		// Play sound effect
		if (LevelGainedsInfo.GetSound() != nullptr)
		{
			GI->PlayEffectSound(LevelGainedsInfo.GetSound());
		}

		if (NewRank < LevelingUpOptions::MAX_COMMANDER_LEVEL)
		{
			const FCommanderLevelUpInfo & NextLevelInfo = FI->GetCommanderLevelUpInfo(Rank + 1);
			const float ExperienceTowardsNextRank = Experience - FI->GetCommanderLevelUpInfo(NewRank).GetCumulativeExperienceRequired();

			PC->OnLevelUp_LastForEvent(OldRank, NewRank, NumUnspentSkillPoints, ExperienceTowardsNextRank,
				NextLevelInfo.GetExperienceRequired());
		}
		else // Max rank
		{
			PC->OnLevelUp_LastForEvent(OldRank, NewRank, NumUnspentSkillPoints, -1.f, -1.f);
		}
	}
}

void ARTSPlayerState::InitIDQueue()
{
	const uint8 MAX_NUM_SELECTABLES_PER_PLAYER = ProjectSettings::MAX_NUM_SELECTABLES_PER_PLAYER;
	
	/* If MAX_NUM_SELECTABLES_PER_PLAYER == 255 then we will add 255 entries, the values
	will be 1 through to 255. Remember that 0 is reserved as a kind of null value */
	for (uint8 i = 0; i < MAX_NUM_SELECTABLES_PER_PLAYER; ++i)
	{
		IDQueue.Enqueue(i + 1);
	}

	IDQueueNum = MAX_NUM_SELECTABLES_PER_PLAYER;
}

void ARTSPlayerState::Client_ShowHUDNotificationMessage_Implementation(EGameNotification NotificationType)
{
	const FGameNotificationInfo & Info = GI->GetHUDNotificationMessageInfo(NotificationType);
	
	// Show message
	HUDWidget->OnGameMessageReceived(Info.GetMessage(), EMessageType::GameNotification);

	// Play sound
	if (Info.GetSound() != nullptr)
	{
		GI->PlayEffectSound(Info.GetSound());
	}
}

bool ARTSPlayerState::CanShowGameWarning(EGameWarning MessageType)
{
	/* May want different implementations for server and client but sticking with same for now.
	Few notes: warning can be 'found out' and shown on client, followed by a warning arriving
	from server before cooldown on client has finished. To change this we would need client to
	notify server of warnings that happen but that just seems too inefficient */

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_GameWarning))
	{
		return false;
	}
	else
	{
		Delay(TimerHandle_GameWarning, &ARTSPlayerState::DoNothing, GI->GetHUDGameMessageCooldown());
		return true;
	}
}

bool ARTSPlayerState::CanShowGameWarning(EAbilityRequirement ReasonForMessage)
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_GameWarning))
	{
		return false;
	}
	else
	{
		Delay(TimerHandle_GameWarning, &ARTSPlayerState::DoNothing, GI->GetHUDGameMessageCooldown());
		return true;
	}
}

void ARTSPlayerState::Client_OnGameWarningHappened_Implementation(EGameWarning MessageType) const
{
	// Show message on HUD
	HUDWidget->OnGameMessageReceived(FI->GetHUDErrorMessage(MessageType), EMessageType::GameWarning);
	
	// Play a sound
	if (FI->GetWarningSound(MessageType) != nullptr)
	{
		GI->PlayEffectSound(FI->GetWarningSound(MessageType));
	}
}

void ARTSPlayerState::Client_OnGameWarningHappenedAbility_Implementation(EAbilityRequirement ReasonForMessage) const
{
	// Show message on HUD
	HUDWidget->OnGameMessageReceived(FI->GetHUDErrorMessage(ReasonForMessage), EMessageType::GameWarning);

	// Play a sound
	if (FI->GetWarningSound(ReasonForMessage) != nullptr)
	{
		GI->PlayEffectSound(FI->GetWarningSound(ReasonForMessage));
	}
}

void ARTSPlayerState::Client_OnGameWarningHappenedMissingResource_Implementation(EResourceType ResourceType) const
{
	// Show message on HUD
	HUDWidget->OnGameMessageReceived(FI->GetHUDMissingResourceMessage(ResourceType), EMessageType::GameWarning);
	
	// Play a sound
	if (FI->GetWarningSound(ResourceType) != nullptr)
	{
		GI->PlayEffectSound(FI->GetWarningSound(ResourceType));
	}
}

void ARTSPlayerState::Client_OnGameWarningHappenedMissingHousingResource_Implementation(EHousingResourceType HousingResourceType) const
{
	// Show message on HUD
	HUDWidget->OnGameMessageReceived(FI->GetHUDMissingHousingResourceMessage(HousingResourceType), EMessageType::GameWarning);
	
	// Play a sound
	if (FI->GetWarningSound(HousingResourceType) != nullptr)
	{
		GI->PlayEffectSound(FI->GetWarningSound(HousingResourceType));
	}
}

void ARTSPlayerState::Delay(FTimerHandle & TimerHandle, void(ARTSPlayerState::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void ARTSPlayerState::DoNothing()
{
}

void ARTSPlayerState::StartAbilitysCooldownTimer(FTimerHandle & AbilitysCooldownTimerHandle, FAquiredCommanderAbilityState * Ability, float DelayAmount)
{
	FTimerDelegate TimerDel;

	TimerDel.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ARTSPlayerState, OnCommanderAbilityCooledDown), *Ability);
	GetWorldTimerManager().SetTimer(AbilitysCooldownTimerHandle, TimerDel, DelayAmount, false);
}

void ARTSPlayerState::OnItemAddedToAProductionQueue(const FTrainingInfo & Item)
{
	// We're currently only keeping this container up to date for server + owner
	assert(GetWorld()->IsServer() || BelongsToLocalPlayer());
	
	if (Item.IsProductionForBuilding())
	{
		NumQueuedSelectables++;
		
		// Do this because we don't default initialize all possible entries to 0
		if (InQueueBuildingTypeQuantities.Contains(Item.GetBuildingType()) == false)
		{
			InQueueBuildingTypeQuantities.Emplace(Item.GetBuildingType(), 0);
		}

		InQueueBuildingTypeQuantities[Item.GetBuildingType()] += 1;
	}
	else if (Item.IsForUnit())
	{
		NumQueuedSelectables++;
		
		// Do this because we don't default initialize all possible entries to 0
		if (InQueueUnitTypeQuantities.Contains(Item.GetUnitType()) == false)
		{
			InQueueUnitTypeQuantities.Emplace(Item.GetUnitType(), 0);
		}
		
		InQueueUnitTypeQuantities[Item.GetUnitType()] += 1;

		const FUnitInfo * UnitInfo = FI->GetUnitInfo(Item.GetUnitType());

		// Consume housing resource
		AddHousingResourceConsumption(UnitInfo->GetHousingCosts());
	}
	else // Assumed upgrade
	{
		// Nothing that needs to be done
	}
}

void ARTSPlayerState::OnItemRemovedFromAProductionQueue(const FTrainingInfo & Item)
{
	// We're currently only keeping this container up to date for server + owner
	assert(GetWorld()->IsServer() || BelongsToLocalPlayer());
	
	if (Item.IsProductionForBuilding())
	{
		NumQueuedSelectables--;
		InQueueBuildingTypeQuantities[Item.GetBuildingType()] -= 1;
	}
	else if (Item.IsForUnit())
	{
		NumQueuedSelectables--;
		InQueueUnitTypeQuantities[Item.GetUnitType()] -= 1;
	}
	else // Assumed upgrade
	{
		// Nothing that needs to be done
	}
}

void ARTSPlayerState::OnBuildingPlaced(ABuilding * Building)
{
	/* This will run whether we own this PS or not */

	assert(Building != nullptr);

#if GAME_THREAD_FOG_OF_WAR
	GS->GetFogManager()->OnBuildingPlaced(Building, Building->GetActorLocation(),
		Building->GetActorRotation());
#elif MULTITHREADED_FOG_OF_WAR
	MultithreadedFogOfWarManager::Get().AddRecentlyCreatedBuilding(Building);
#endif

	Buildings.Emplace(Building);

	// Only continue if on server or owned by local player
	if (!GetWorld()->IsServer() && !BelongsToLocalPlayer())
	{
		return;
	}

	if (PlacedBuildingTypeQuantities.Contains(Building->GetType()))
	{
		PlacedBuildingTypeQuantities[Building->GetType()] += 1;
	}
	else
	{
		PlacedBuildingTypeQuantities.Emplace(Building->GetType(), 1);
	}
}

void ARTSPlayerState::OnBuildingBuilt(ABuilding * Building, EBuildingType BuildingType, ESelectableCreationMethod CreationMethod)
{
	/* TODO Add some checks in here to ensure this runs only on server and owning client.
	Have func defined in this class already for that, may need to modify it */

	/* I think this is going to get called for any building that is built, friend or foe. Need
	to check if this is out local player state before updating prereqs. Same for
	OnBuildingDestroyed */

	/* TODO figure out what in this func needs to happen on server/client and what doesnt. Pretty
	sure whole func needs to run on server and owning client, but the Buildings array
	Emplace/Remove could be for everyone since I guess clients could see how many buildings
	eeveryone has */

	/* Try play a sound */
	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
	if (ShouldPlayBuildingBuiltSound_ConstructionComplete(*BuildingInfo, CreationMethod))
	{
		PlayBuildingBuiltSound(BuildingInfo->GetJustBuiltSound());
	}

	// Only run on server or client if this is their player state
	if (!GetWorld()->IsServer() && !BelongsToLocalPlayer())
	{
		return;
	}

	assert(Building != nullptr);

	/* Update housing provided */
	AddHousingResourcesProvided(Building->GetBuildingAttributes()->GetHousingResourcesProvidedArray());

	/* Update resource depot info */
	for (const auto & ResourceType : Building->GetBuildingAttributes()->GetResourceCollectionTypes())
	{
		/* Tell resource depot this building is a drop point for it */
		ResourceDepots[ResourceType].AddBuilding(Building);

		/* Update each resources spots closest depot info */
		UpdateClosestDepots(ResourceType, Building, true);
	}

	/* Whether this building is the first of its type or not */
	bool bNewPrereqObtained;

	/* Add entry to the "completed construction" container */
	if (CompletedBuildingTypeQuantities.Contains(BuildingType) && CompletedBuildingTypeQuantities[BuildingType] > 0)
	{
		CompletedBuildingTypeQuantities.Add(BuildingType, CompletedBuildingTypeQuantities[BuildingType] + 1);

		bNewPrereqObtained = false;
	}
	else
	{
		CompletedBuildingTypeQuantities.Add(BuildingType, 1);

		bNewPrereqObtained = true;
	}

	/* Remove from the "only placed" container */
	PlacedBuildingTypeQuantities[BuildingType] -= 1;
	assert(PlacedBuildingTypeQuantities[BuildingType] >= 0);

	/* Register if a construction yard type building */
	if (Building->GetPersistentProductionQueue()->GetCapacity() > 0)
	{
		assert(!PersistentQueueSupportingBuildings.Contains(Building));
		PersistentQueueSupportingBuildings.Emplace(Building);
	}

	/* Register context buttons to let us know this building is a candidate for being chosen
	by the HUD persistent panel to build things */
	for (const auto & Button : Building->GetAttributes()->GetContextMenu().GetButtonsArray())
	{
		/* The contains check is like asking "is tab type not None?" */
		if (ProductionCapableBuildings.Contains(Button))
		{
			/* Crashes here? Probably PostEdit not working correctly. Try adding then deleting a
			button from selectables context menu */
			ProductionCapableBuildings[Button].AddBuilding(Building);
		}
	}

	/* Update players HUD if they are not a CPU player and this is local players player state */
	if (!bIsABot && BelongsToLocalPlayer())
	{
		/* Crashes here probably mean a player who does not own this player state got this far */
		HUDWidget->OnBuildingConstructed(BuildingType, bNewPrereqObtained);
	}
}

void ARTSPlayerState::OnBuildingZeroHealth(ABuilding * Building)
{
	/* TODO Add some checks in here to ensure this runs only on server and owning client.
	Have func defined in this class already for that, may need to modify it */

	assert(Building != nullptr);
	assert(Buildings.Contains(Building));

	// Only continue if on server or owned by local player
	if (!GetWorld()->IsServer() && !BelongsToLocalPlayer())
	{
		return;
	}

	const EBuildingType BuildingType = Building->GetType();

	/* Update housing provided */
	RemoveHousingResourcesProvided(Building->GetBuildingAttributes()->GetHousingResourcesProvidedArray());

	/* Update resource depot info */
	for (const auto & ResourceType : Building->GetBuildingAttributes()->GetResourceCollectionTypes())
	{
		ResourceDepots[ResourceType].RemoveBuilding(Building);

		/* Update each resources spots closest depot info */
		UpdateClosestDepots(ResourceType, Building, false);
	}

	/* Whether this building was the last of its type */
	bool bPrerequisiteLost = false;

	/* Remove from correct container depending on whether it has completed construction or not */
	if (Building->IsConstructionComplete())
	{
		/* Can be false for when creating faction info */
		if (CompletedBuildingTypeQuantities.Contains(BuildingType))
		{
			CompletedBuildingTypeQuantities[BuildingType] -= 1;
			assert(CompletedBuildingTypeQuantities[BuildingType] >= 0);

			if (CompletedBuildingTypeQuantities[BuildingType] == 0)
			{
				bPrerequisiteLost = true;
			}
		}
	}
	else
	{
		PlacedBuildingTypeQuantities[BuildingType] -= 1;
		assert(PlacedBuildingTypeQuantities[BuildingType] >= 0);
	}

	/* Unregister if a construction yard type building */
	if (Building->GetPersistentProductionQueue()->GetCapacity() > 0)
	{
		PersistentQueueSupportingBuildings.Remove(Building);
	}

	// TODO could also cancel ghost if player has one spawned, completely optional though
	/* Remove from the list of buildings with complete BuildInTab production. */
	BuildsInTabCompleteBuildings.Remove(Building);

	/* Unregister context buttons to us know building is no longer a candidate to build these
	things */
	for (const auto & Button : Building->GetAttributes()->GetContextMenu().GetButtonsArray())
	{
		/* The contains check is like asking "is tab type not None?" */
		if (ProductionCapableBuildings.Contains(Button))
		{
			ProductionCapableBuildings[Button].RemoveBuilding(Building);
		}
	}

	if (!bIsABot && BelongsToLocalPlayer())
	{
		HUDWidget->OnBuildingDestroyed(BuildingType, bPrerequisiteLost);
	}
}

void ARTSPlayerState::OnBuildingDestroyed(ABuilding * Building)
{
	FreeUpSelectableID(Building);
}

void ARTSPlayerState::RemoveBuildingFromBuildingsContainer(ABuilding * Building)
{
	assert(Buildings.Contains(Building));

	/* O(n) */
	Buildings.RemoveSingleSwap(Building);

#if GAME_THREAD_FOG_OF_WAR
	GS->GetFogManager()->OnBuildingDestroyed(Building);
#elif MULTITHREADED_FOG_OF_WAR
	MultithreadedFogOfWarManager::Get().OnBuildingDestroyed(Building);
#endif
}

void ARTSPlayerState::OnUnitBuilt(AInfantry * Unit, EUnitType UnitType, ESelectableCreationMethod CreationMethod)
{
	assert(Unit != nullptr);

	Units.Emplace(Unit);

	// Only run if on server or this is our player state
	if (GetWorld()->IsServer() || BelongsToLocalPlayer())
	{
		/* Increase quantity */
		if (UnitTypeQuantities.Contains(UnitType))
		{
			UnitTypeQuantities[UnitType] += 1;
		}
		else
		{
			UnitTypeQuantities.Emplace(UnitType, 1);
		}
	}
	
	// Decide if should play a 'unit just built' sound
	const FBuildInfo * UnitInfo = FI->GetUnitInfo(UnitType);
	if (ShouldPlayUnitBuiltSound(UnitType, *UnitInfo, CreationMethod))
	{
		PlayUnitBuiltSound(UnitInfo->GetJustBuiltSound());
	}
}

void ARTSPlayerState::OnUnitDestroyed(AInfantry * Unit)
{
	assert(Unit != nullptr);

	const EUnitType UnitType = Unit->GetType();

	/* O(n) */
	Units.RemoveSingleSwap(Unit);

	// Only run if on server or this is our player state
	if (GetWorld()->IsServer() || BelongsToLocalPlayer())
	{
		/* Decrease quantity. */
		UnitTypeQuantities[UnitType] -= 1;
		assert(UnitTypeQuantities[UnitType] >= 0);

		/* Remove the housing resources this unit was using up */
		RemoveHousingResourceConsumption(UnitType);
	}

	FreeUpSelectableID(Unit);
}

void ARTSPlayerState::Setup_OnUnitDestroyed(AInfantry * Unit)
{
	assert(Unit != nullptr);

	/* O(n) */
	Units.Remove(Unit);
}

void ARTSPlayerState::Server_SetInitialValues(uint8 InID, FName InPlayerIDAsFName, ETeam InTeam, 
	EFaction InFaction, int16 InStartingSpot, const TArray <int32> & StartingResources)
{
	SERVER_CHECK;

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	/* Setup the list that assigns unique IDs to selectables when they are built */
	InitIDQueue();
	
	IDAsInt = InID;
	ID = InPlayerIDAsFName;
	Team = InTeam;
	SetTeamTag(GS->GetTeamTag(InTeam));
	Faction = InFaction;
	StartingSpot = InStartingSpot;

	FI = GI->GetFactionInfo(Faction);

	/* Set initial resource amounts */
	Server_SetInitialResourceAmounts(StartingResources);

	SetupProductionCapableBuildingsMap();

	/* If this doesn't belong to the server player then setup the commander skill tree. 
	The server player's will be set up later on so we don't do it now. */
	if (bIsABot || GetWorld()->GetFirstPlayerController()->PlayerState != this)
	{
		NumUnspentSkillPoints = FI->GetNumInitialCommanderSkillPoints();
		
		//-----------------------------------------------------------
		//	Spawn skill tree widget, extract data, then destroy
		//-----------------------------------------------------------

		TSubclassOf<UCommanderSkillTreeWidget> WidgetBlueprint = nullptr;
		if (FI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2) != nullptr)
		{
			WidgetBlueprint = FI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2);
		}
		else if (GI->IsMatchWidgetBlueprintSet(EMatchWidgetType::CommanderSkillTree_Ver2))
		{
			WidgetBlueprint = GI->GetMatchWidgetBP(EMatchWidgetType::CommanderSkillTree_Ver2);
		}
		
		if (WidgetBlueprint != nullptr)
		{
			UCommanderSkillTreeWidget * CommanderSkillTreeWidget = CreateWidget<UCommanderSkillTreeWidget>(GetWorld()->GetFirstPlayerController(), WidgetBlueprint);
			CommanderSkillTreeWidget->SetVisibility(ESlateVisibility::Hidden);
			CommanderSkillTreeWidget->SetupWidget_GettingDestroyedAfter(GI, PC);
			CommanderSkillTreeWidget->AddToViewport(-1000);
		
			/* This is the important line. It will inspect the widget and create state on this 
			player state. The 3rd param is garbage */
			CommanderSkillTreeWidget->MoreSetup(this, FI, ECommanderSkillTreeAnimationPlayRule::Never);
		
			/* Destroy since no longer needed. Hoping that is what this line does */
			CommanderSkillTreeWidget->RemoveFromViewport();
		}
	}
}

void ARTSPlayerState::Multicast_SetInitialValues_Implementation(uint8 InID, FName InPlayerIDAsFName, 
	ETeam InTeam, EFaction InFaction)
{
	// Ummm... && bIsABot == false too?
	bBelongsToLocalPlayer = (GetWorld()->GetFirstPlayerController()->PlayerState == this);

	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());

	IDAsInt = InID;
	ID = InPlayerIDAsFName;
	Faction = InFaction;
	Team = InTeam;
	SetTeamTag(GS->GetTeamTag(InTeam));

	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
	FI = GI->GetFactionInfo(Faction);

	GS->AddToTeam(this, Team);

	/* Because upgrade manager relies on faction this must come after faction has been set */
	SetupUpgradeManager();

	// Default initialize 0 values for these TMaps. Maybe only server/owner need to do this
	InQueueUnitTypeQuantities.Reserve(FI->GetUnitQuantityLimits().Num());
	UnitTypeQuantities.Reserve(FI->GetUnitQuantityLimits().Num());
	for (const auto & Elem : FI->GetUnitQuantityLimits())
	{
		InQueueUnitTypeQuantities.Emplace(Elem.Key, 0);
		UnitTypeQuantities.Emplace(Elem.Key, 0);
	}

	/* That's those variables done, but upgrade manager needs to rep + have its owner (this) rep
	too, so poll for that to complete. Yes OnRep funcs can be used and they are for setting up
	upgrade manager but I prefer having all the requirements here in one function. */
	BusyWaitForSetupToComplete();
}

void ARTSPlayerState::Client_FinalSetup_Implementation()
{
	/* Check this. The array actually always puts at least 1 entry in it cause you're not 
	allowed a C arary of size 0 */
	if (Statics::NUM_BUILDING_GARRISON_NETWORK_TYPES > 0)
	{
		/* ***** Setting up garrison info structs ***** */
		int32 Index = 0;
		/* Iterating regular C array. Hoping it starts from index 0 otherwise Index variable is wrong */
		for (auto & Elem : BuildingGarrisonNetworkInfos.NetworksContainer)
		{
			Elem.SetupStruct(GI->GetBuildingNetworkInfo(Statics::ArrayIndexToBuildingNetworkType(Index)));
			Index++;
		}
	}

	/* If player state is owned by a CPU player then do nothing - server player will do all this */
	if (bIsABot)
	{
		return;
	}

	// Set affiliation for this player state and others
	for (TActorIterator <ARTSPlayerState> Iter(GetWorld()); Iter; ++Iter)
	{
		ARTSPlayerState * OtherPlayerState = *Iter;

		if (OtherPlayerState == this)
		{
			OtherPlayerState->SetAffiliation(EAffiliation::Owned);
		}
		else if (OtherPlayerState->GetTeam() == this->GetTeam())
		{
			OtherPlayerState->SetAffiliation(EAffiliation::Allied);
		}
		else
		{
			OtherPlayerState->SetAffiliation(EAffiliation::Hostile);
		}
	}

	// Setup each team collision channels
	GS->SetupCollisionChannels();

	HUDWidget->PerformPlayerTargetingPanelFinalSetup();

	/* Setup inventory items already placed on map. Perhaps better performance to do this
	in the same loop as the resource spot one. Must happen before creating object pools though */
	for (TActorIterator <AInventoryItem> Iter(GetWorld()); Iter; ++Iter)
	{
		AInventoryItem * Elem = *Iter;

		/* It's important that all clients + server assign the same IDs to the same
		actors here i.e. iteration order is the same. If that cannot be guaranteed then
		something else will need to be arranged */
		Elem->SetupStuff(GI, GS, true);
	}

	// Make sure GS::SetupEffectsActors is called before this because they can use projectiles
	GI->GetPoolingManager()->CreatePools();

#if GAME_THREAD_FOG_OF_WAR 

	if (!PC->IsObserver())
	{
		/* Start ticking fog of war manager. It needs team to be set to do so, so could do it
		sooner but no point while map is still setting up */
		PC->GetFogOfWarManager()->SetActorTickEnabled(true);
	}

#endif

	// Setup resource spots on map
	for (TActorIterator <AResourceSpot> Iter(GetWorld()); Iter; ++Iter)
	{
		AResourceSpot * Elem = *Iter;
		Elem->SetupForLocalPlayer(GI, FI, GetTeam());
	}

	// Do final setup on commander abilities
	for (auto & Elem : GS->GetAllCommanderAbilityEffectObjects())
	{
		Elem.Value->FinalSetup(GI, GS, this);
	}

	// Acknowledge setup complete
	Server_AckFinalSetupComplete();
}

void ARTSPlayerState::SetStartLocation(const FVector & InLocation)
{
	StartLocation = InLocation;
}

bool ARTSPlayerState::BelongsToLocalPlayer() const
{
	return bBelongsToLocalPlayer;
}

bool ARTSPlayerState::HasBeenDefeated() const
{
	return bHasBeenDefeated;
}

void ARTSPlayerState::SetupUpgradeManager()
{
	assert(UpgradeManager == nullptr);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	const FTransform SpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
	UpgradeManager = GetWorld()->SpawnActor<AUpgradeManager>(AUpgradeManager::StaticClass(),
		SpawnTransform, SpawnParams);
}

void ARTSPlayerState::BusyWaitForSetupToComplete()
{
	if (HasFullySetup())
	{
		/* Get player controller to call RPC because ownership */
		ARTSPlayerController * PlayCon = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());
		PlayCon->Server_AckPSSetupOnClient();
	}
	else
	{
		FTimerHandle TimerHandle_Dummy;
		Delay(TimerHandle_Dummy, &ARTSPlayerState::BusyWaitForSetupToComplete, 0.01f);
	}
}

bool ARTSPlayerState::HasFullySetup() const
{
	/* Server I assume will always return true. 
	This code is left behind from when we used to replicate upgrade manager. It can probabaly 
	be removed and this can just return true instead */
	return UpgradeManager != nullptr && UpgradeManager->HasInited();
}

void ARTSPlayerState::Server_SetInitialResourceAmounts(const TArray < int32 > & InitialAmounts)
{
	SERVER_CHECK;

	assert(ResourceArray.Num() == 0 && OldResourceArray.Num() == 0);
	assert(InitialAmounts.Num() == Statics::NUM_RESOURCE_TYPES);

	for (int32 i = 0; i < InitialAmounts.Num(); ++i)
	{
		ResourceArray.Emplace(InitialAmounts[i]);
		OldResourceArray.Emplace(InitialAmounts[i]);

		int32 & ResourceVar = GetReplicatedResourceVariable(ArrayIndexToResourceType(i));
		ResourceVar = InitialAmounts[i];
	}

	// Do housing resources aswell
	const TArray < int32 > & ConstantAmountProvidedArray = FI->GetConstantHousingResourceAmountProvided();
	const TArray < int32 > & MaxProvidedAllowedArray = FI->GetHousingResourceLimits();
	assert(HousingResourceArray.Num() == 0); // Make sure we haven't done this already 
	HousingResourceArray.Reserve(Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		HousingResourceArray.Emplace(FHousingResourceState(i, ConstantAmountProvidedArray[i], MaxProvidedAllowedArray[i]));
	}
}

void ARTSPlayerState::Server_AckFinalSetupComplete_Implementation()
{
	GS->Server_AckFinalSetupComplete();
}

bool ARTSPlayerState::Server_AckFinalSetupComplete_Validate()
{
	return true;
}

void ARTSPlayerState::Client_SetInitialResourceAmounts(const TArray<int32>& InitialAmounts, URTSHUD * InHUD)
{
	HUDWidget = InHUD;
	
	/* OK I think server has already done most of this */
	if (GetWorld()->IsServer())
	{
		HUDWidget->SetInitialResourceAmounts(InitialAmounts, HousingResourceArray);
		
		return;
	}

	assert(ResourceArray.Num() == 0 && OldResourceArray.Num() == 0);
	assert(InitialAmounts.Num() == Statics::NUM_RESOURCE_TYPES);

	for (int32 i = 0; i < InitialAmounts.Num(); ++i)
	{
		ResourceArray.Emplace(InitialAmounts[i]);
		OldResourceArray.Emplace(InitialAmounts[i]);
	}

	// Do housing resources aswell
	const TArray < int32 > & ConstantAmountProvidedArray = FI->GetConstantHousingResourceAmountProvided();
	const TArray < int32 > & MaxProvidedAllowedArray = FI->GetHousingResourceLimits();
	assert(HousingResourceArray.Num() == 0); // Make sure we haven't done this already
	HousingResourceArray.Reserve(Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		/* Set our amount provided as what the faction always provides */
		HousingResourceArray.Emplace(FHousingResourceState(i, ConstantAmountProvidedArray[i], MaxProvidedAllowedArray[i]));
	}

	HUDWidget->SetInitialResourceAmounts(InitialAmounts, HousingResourceArray);
}

const TArray < int32 > & ARTSPlayerState::GetBuildingResourceCost(EBuildingType Type) const
{
	const FBuildInfo * const BuildInfo = FI->GetBuildingInfo(Type);

	return BuildInfo->GetCosts();
}

const TArray<int32>& ARTSPlayerState::GetUnitResourceCost(EUnitType Type) const
{
	const FBuildInfo * const BuildInfo = FI->GetUnitInfo(Type);

	return BuildInfo->GetCosts();
}

#if ENABLE_VISUAL_LOG
int32 ARTSPlayerState::GetNumResources(int32 ArrayIndex) const
{
	return ResourceArray[ArrayIndex];
}
#endif // ENABLE_VISUAL_LOG

bool ARTSPlayerState::HasEnoughResources(const TArray <int32> & CostArray, bool bShowHUDMessage)
{
	/* Assert sure every resource is accounted for when we iterate */
	assert(CostArray.Num() == Statics::NUM_RESOURCE_TYPES);

	/* Use the const Statics::NUM_RESOURCE_TYPES instead of CostArray.Num() for performance */
	for (int32 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		if (CostArray[i] > ResourceArray[i])
		{
			if (bShowHUDMessage)
			{
				// Try print game message on player HUD
				// TODO add cooldown of like 1second on this
				TryShowGameWarning_NotEnoughResources(Statics::ArrayIndexToResourceType(i));
			}

			return false;
		}
	}

	return true;
}

EResourceType ARTSPlayerState::HasEnoughResourcesSpecific(const TArray<int32>& CostArray)
{
	/* Assert sure every resource is accounted for when we iterate */
	assert(CostArray.Num() == Statics::NUM_RESOURCE_TYPES);

	for (int32 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		if (CostArray[i] > ResourceArray[i])
		{
			return Statics::ArrayIndexToResourceType(i);
		}
	}

	return EResourceType::None;
}

bool ARTSPlayerState::HasEnoughResources(EBuildingType Type, bool bShowHUDMessage)
{
	const FBuildInfo * const BuildInfo = FI->GetBuildingInfo(Type);

	return HasEnoughResources(BuildInfo->GetCosts(), bShowHUDMessage);
}

bool ARTSPlayerState::HasEnoughResources(EUnitType Type, bool bShowHUDMessage)
{
	const FBuildInfo * const BuildInfo = FI->GetUnitInfo(Type);

	return HasEnoughResources(BuildInfo->GetCosts(), bShowHUDMessage);
}

bool ARTSPlayerState::HasEnoughResources(EUpgradeType UpgradeType, bool bShowHUDWarning)
{
	const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(UpgradeType);

	return HasEnoughResources(UpgradeInfo.GetCosts(), bShowHUDWarning);
}

bool ARTSPlayerState::HasEnoughResources(const FContextButton & Button, bool bShowHUDWarning)
{
	if (Button.GetButtonType() == EContextButton::Train)
	{
		return HasEnoughResources(Button.GetUnitType(), bShowHUDWarning);
	}
	else if (Button.GetButtonType() == EContextButton::BuildBuilding)
	{
		return HasEnoughResources(Button.GetBuildingType(), bShowHUDWarning);
	}
	else // Assume for upgrade
	{
		assert(Button.GetButtonType() == EContextButton::Upgrade);

		return HasEnoughResources(Button.GetUpgradeType(), bShowHUDWarning);
	}
}

bool ARTSPlayerState::HasEnoughResources(const FTrainingInfo & InTrainingInfo, bool bShowHUDWarning)
{
	if (InTrainingInfo.IsProductionForBuilding())
	{
		return HasEnoughResources(InTrainingInfo.GetBuildingType(), bShowHUDWarning);
	}
	else if (InTrainingInfo.IsForUnit())
	{
		return HasEnoughResources(InTrainingInfo.GetUnitType(), bShowHUDWarning);
	}
	else // Assumed for upgrade
	{
		assert(InTrainingInfo.IsForUpgrade());

		return HasEnoughResources(InTrainingInfo.GetUpgradeType(), bShowHUDWarning);
	}
}

bool ARTSPlayerState::HasEnoughHousingResources(const TArray<int16>& HousingCostArray, bool bShowHUDWarning)
{
	assert(HousingCostArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	for (int32 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		if (HousingCostArray[i] > HousingResourceArray[i].GetAmountProvidedClamped() - HousingResourceArray[i].GetAmountConsumed())
		{
			if (bShowHUDWarning)
			{
				OnGameWarningHappened(Statics::ArrayIndexToHousingResourceType(i));
			}

			return false;
		}
	}
	
	return true;
}

bool ARTSPlayerState::HasEnoughHousingResources(EUnitType UnitType, bool bShowHUDWarning)
{
	const FUnitInfo * UnitInfo = FI->GetUnitInfo(UnitType);
	
	return HasEnoughHousingResources(UnitInfo->GetHousingCosts(), bShowHUDWarning);
}

void ARTSPlayerState::GainExperience(float RawExperienceBounty, bool bApplyRandomness)
{
	SERVER_CHECK;

	if (Rank < LevelingUpOptions::MAX_COMMANDER_LEVEL)
	{
		const float Multiplier = bApplyRandomness ? FMath::RandRange(1.f - LevelingUpOptions::COMMANDER_EXPERIENCE_GAIN_RANDOM_FACTOR, 1.f + LevelingUpOptions::COMMANDER_EXPERIENCE_GAIN_RANDOM_FACTOR) : 1.f;
		const float ExperienceGain = RawExperienceBounty * Multiplier;
		Experience += ExperienceGain;

		/* Call explicitly because on server */
		OnRep_Experience();
	}
}

float ARTSPlayerState::GetTotalExperience() const
{
	return Experience;
}

uint8 ARTSPlayerState::GetRank() const
{
	return Rank;
}

int32 ARTSPlayerState::GetNumUnspentSkillPoints() const
{
	return NumUnspentSkillPoints;
}

void ARTSPlayerState::RegisterCommanderSkillTreeNode(const FCommanderAbilityTreeNodeInfo * NodeInfo, uint8 ArrayIndex)
{
	AllCommanderSkillTreeNodes.Emplace(NodeInfo);
	
	for (int32 i = 0; i < NodeInfo->GetNumRanks(); ++i)
	{
		const FCommanderAbilityInfo * AbilityInfo = NodeInfo->GetAbilityInfo(i).GetAbilityInfo();
		
		UE_CLOG(CommanderAbilityToNodeType.Contains(AbilityInfo) == true, RTSLOG, Fatal,
			TEXT("Faction [%s]'s commander skill tree has 2+ nodes on it that have the same ability "
				"which is [%s]. This is not allowed "), TO_STRING(EFaction, Faction), 
				TO_STRING(ECommanderAbility, AbilityInfo->GetType()));
		
		CommanderAbilityToNodeType.Emplace(AbilityInfo, NodeInfo->GetNodeType());
	}

	UnaquiredCommanderAbilities.Emplace(NodeInfo->GetNodeType(), FUnaquiredCommanderAbilityState(NodeInfo));
}

FAquiredCommanderAbilityState * ARTSPlayerState::GetCommanderAbilityState(const FCommanderAbilityInfo & AbilityInfo) const
{
	return AquiredCommanderAbilitiesTMap[CommanderAbilityToNodeType[&AbilityInfo]];
}

ECommanderSkillTreeNodeType ARTSPlayerState::GetCommanderSkillTreeNodeType(const FCommanderAbilityInfo * AbilityInfo) const
{
	return CommanderAbilityToNodeType[AbilityInfo];
}

int32 ARTSPlayerState::GetCommanderAbilityObtainedRank(const FCommanderAbilityTreeNodeInfo & NodeInfo) const
{
	if (UnaquiredCommanderAbilities.Contains(NodeInfo.GetNodeType()))
	{
		return -1;
	}
	else
	{
		return AquiredCommanderAbilitiesTMap[NodeInfo.GetNodeType()]->GetAquiredRank();
	}
}

bool ARTSPlayerState::CanAffordCommanderAbilityAquireCost(const FCommanderAbilityTreeNodeInfo & NodeInfo) const
{
	const int32 AquiredNodeRank = GetCommanderAbilityObtainedRank(NodeInfo);
	
	// Will crash here if we've already aquired the max rank of the ability
	return NumUnspentSkillPoints >= NodeInfo.GetCost(AquiredNodeRank + 1);
}

bool ARTSPlayerState::ArePrerequisitesForNextCommanderAbilityRankMet(const FCommanderAbilityTreeNodeInfo & NodeInfo) const
{
	/* Only the first rank of the ability has prerequisites */

	const int32 ObtainedRank = GetCommanderAbilityObtainedRank(NodeInfo);
	if (ObtainedRank == -1)
	{
		return UnaquiredCommanderAbilities[NodeInfo.GetNodeType()].ArePrerequisitesMet();
	}
	else
	{
		return true;
	}

	/* May be possible to assert (ObtainedRank == -1) and avoid the else case */
}

TMap<ECommanderSkillTreeNodeType, FUnaquiredCommanderAbilityState>& ARTSPlayerState::GetUnaquiredCommanderAbilities()
{
	return UnaquiredCommanderAbilities;
}

EGameWarning ARTSPlayerState::CanAquireCommanderAbility(const FCommanderAbilityTreeNodeInfo & NodeInfo, bool bHandleHUDWarnings)
{
	EGameWarning Warning = EGameWarning::None;
	
	/* Check if the player has already aquired the max rank for this ability and therefore
	cannot aquire anymore */
	const int32 ObtainedAbilityRank = GetCommanderAbilityObtainedRank(NodeInfo);
	if (ObtainedAbilityRank + 1 == NodeInfo.GetNumRanks())
	{
		Warning = EGameWarning::CommanderSkillTree_MaxAbilityRankObtained;
	}
	/* Check if player's rank if high enough */
	else if (GetRank() < NodeInfo.GetUnlockRank(ObtainedAbilityRank + 1))
	{
		Warning = EGameWarning::CommanderSkillTree_CommanderRankNotHighEnough;
	}
	/* Check if all the prerequisites for the ability are met. Only need to do this for 
	the first rank of the ability */
	else if (ObtainedAbilityRank == -1 && ArePrerequisitesForNextCommanderAbilityRankMet(NodeInfo) == false)
	{
		Warning = EGameWarning::CommanderSkillTree_PrerequisitesNotMet;
	}
	/* Check the player can afford the ability */
	else if (GetNumUnspentSkillPoints() < NodeInfo.GetCost(ObtainedAbilityRank + 1))
	{
		Warning = EGameWarning::CommanderSkillTree_NotEnoughSkillPoints;
	}

	/* Show HUD message and play sound if there was a warning */
	if (bHandleHUDWarnings && Warning != EGameWarning::None)
	{
		OnGameWarningHappened(Warning);
	}

	return Warning;
}

EGameWarning ARTSPlayerState::CanAquireCommanderAbility(uint8 AllNodesArrayIndex, bool bHandleHUDWarnings)
{
	return CanAquireCommanderAbility(*(AllCommanderSkillTreeNodes[AllNodesArrayIndex]), bHandleHUDWarnings);
}

void ARTSPlayerState::AquireNextRankForCommanderAbility(uint8 AllNodesArrayIndex, bool bInstigatorIsServerPlayer)
{
	AquireNextRankForCommanderAbility(AllCommanderSkillTreeNodes[AllNodesArrayIndex], bInstigatorIsServerPlayer, AllNodesArrayIndex);
}

void ARTSPlayerState::AquireNextRankForCommanderAbility(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
	bool bInstigatorIsServerPlayer, uint8 AllNodesArrayIndex)
{
	if (NodeInfo->OnlyExecuteOnAquired())
	{
		SERVER_CHECK;
		
		const int32 AquiredAbilityRank = GetCommanderAbilityObtainedRank(*NodeInfo);

		/* Execute ability now. We're assuming the ability does not require any target or 
		a location as a target */
		GS->Server_CreateAbilityEffect(*NodeInfo->GetAbilityInfo(AquiredAbilityRank + 1).GetAbilityInfo(), 
			this, Team, FVector::ZeroVector, nullptr);
	}
	else
	{
		AquireNextRankForCommanderAbilityInternal(NodeInfo, bInstigatorIsServerPlayer, AllNodesArrayIndex);
	}
}

void ARTSPlayerState::AquireNextRankForCommanderAbilityInternal(const FCommanderAbilityTreeNodeInfo * NodeInfo, 
	bool bInstigatorIsServerPlayer, uint8 AllNodesArrayIndex)
{
	const ECommanderSkillTreeNodeType NodeType = NodeInfo->GetNodeType();

	// Increment aquired rank
	const int32 CurrentAbilityRank = GetCommanderAbilityObtainedRank(*NodeInfo);
	const int32 NewAbilityRank = CurrentAbilityRank + 1;
	if (CurrentAbilityRank == -1) // -1 means have not aquired any rank of ability yet
	{
		UnaquiredCommanderAbilities.Remove(NodeType);

		AquiredCommanderAbilities.Emplace(FAquiredCommanderAbilityState(this, NodeInfo->GetAbilityInfo(NewAbilityRank).GetAbilityInfo()));
		AquiredCommanderAbilitiesTMap.Emplace(NodeType, &AquiredCommanderAbilities.Last());
	}
	else
	{	
		AquiredCommanderAbilitiesTMap[NodeType]->OnAnotherRankAquired(NodeInfo->GetAbilityInfo(NewAbilityRank).GetAbilityInfo());
	}

	// Pay the skill point cost
	NumUnspentSkillPoints -= NodeInfo->GetCost(NewAbilityRank);

	// Update prerequisites for other abilities
	FCommanderSkillTreePrerequisiteArrayEntry Tuple(NodeType, NewAbilityRank + 1);
	for (auto & Pair : UnaquiredCommanderAbilities)
	{
		Pair.Value.OnAnotherAbilityAquired(Tuple);
	}

	//------------------------------------------------------
	//	------- UI updates -------
	//------------------------------------------------------

	if (bIsABot == false && bBelongsToLocalPlayer == true)
	{
		// Update HUD which will also update the tooltip widget if it is showing. 
		// It will also update the commander skill tree and it's node widget
		HUDWidget->OnCommanderSkillAquired(AquiredCommanderAbilitiesTMap[NodeType], NodeInfo, NumUnspentSkillPoints);
	}

	//------------------------------------------------------
	//	------- RPC -------
	//------------------------------------------------------

	/* Do not send RPC if OnlyExecuteOnAquired - GS::Server_CreateAbilityEffect will handle 
	notifying the players in UCommanderAbilityBase::Client_Execute instead */
	if (GetWorld()->IsServer() && bInstigatorIsServerPlayer == false && NodeInfo->OnlyExecuteOnAquired() == false)
	{
		/* Really it would be better to have a replicated array, or perhaps a 64 bit bitfield
		that we signal this with instead of a reliable RPC */
		Client_AquireNextRankForCommanderAbility(AllNodesArrayIndex);
	}
}

void ARTSPlayerState::Client_AquireNextRankForCommanderAbility_Implementation(uint8 AllNodesArrayIndex)
{
	AquireNextRankForCommanderAbilityInternal(AllCommanderSkillTreeNodes[AllNodesArrayIndex], false, AllNodesArrayIndex);
}

void ARTSPlayerState::OnCommanderAbilityUse(const FCommanderAbilityInfo & AbilityInfo, uint8 GameTickCountAtTimeOfAbilityUse)
{
	assert(GetWorld()->IsServer() || BelongsToLocalPlayer());

	AquiredCommanderAbilitiesTMap[CommanderAbilityToNodeType[&AbilityInfo]]->OnAbilityUsed(this, AbilityInfo, HUDWidget);
}

void ARTSPlayerState::OnCommanderAbilityCooledDown(FAquiredCommanderAbilityState & CooledDownAbility)
{
	if (BelongsToLocalPlayer())
	{
		HUDWidget->OnCommanderAbilityCooledDown(*CooledDownAbility.GetAbilityInfo(), CooledDownAbility.GetGlobalSkillsPanelButtonIndex());
	}
}

void ARTSPlayerState::GetActorToBuildFrom(const FContextButton & Button, bool bShowHUDWarning,
	ABuilding *& OutActor, EProductionQueueType & QueueType)
{
	assert(OutActor == nullptr);

	/* The assumption with calling this function is you are clicking a button on the HUD
	persistent panel. */

	/* Check if any fully constructed buildings can produce this. This inner function will take
	care of warnings */
	OutActor = GetProductionBuilding(Button, bShowHUDWarning, QueueType);
}

TArray<ABuilding*>& ARTSPlayerState::GetBuildsInTabCompleteBuildings()
{
	return BuildsInTabCompleteBuildings;
}

void ARTSPlayerState::OnBuildsInTabProductionComplete(ABuilding * ProducerBuilding)
{
	/* While persistent queue capacitites are limited to 1 this is valid */
	assert(!BuildsInTabCompleteBuildings.Contains(ProducerBuilding));

	BuildsInTabCompleteBuildings.Emplace(ProducerBuilding);
}

void ARTSPlayerState::OnGameEventHappened(EGameNotification NotificationType)
{
	Client_ShowHUDNotificationMessage(NotificationType);
}

void ARTSPlayerState::OnGameWarningHappened(EGameWarning MessageType)
{
	/* There is no "not enough resources" message. Instead you must use specific messages 
	for each resource type. You can call HasEnoughResourcesSpecific to know what specific 
	resource is missing instead of regular HasEnoughResources. 
	Then call OnGameWarningHappened(EResourceType) instead of this. 
	Could allow a "not enough resources" message though if users want it */
	assert(MessageType != EGameWarning::NotEnoughResources);
	
	const bool bCanIssueWarning = CanShowGameWarning(MessageType);

	if (bCanIssueWarning)
	{
		Client_OnGameWarningHappened(MessageType);
	}
}

void ARTSPlayerState::OnGameWarningHappened(EAbilityRequirement ReasonForMessage)
{
	/* If this throws it could mean a programming error in one of the functions that issues 
	commands or int32 overflow */
	assert(ReasonForMessage != EAbilityRequirement::Uninitialized);
	
	const bool bCanIssueWarning = CanShowGameWarning(ReasonForMessage);

	if (bCanIssueWarning)
	{
		Client_OnGameWarningHappenedAbility(ReasonForMessage);
	}
}

void ARTSPlayerState::TryShowGameWarning_NotEnoughResources(EResourceType ResourceType)
{
	const bool bCanIssueWarning = CanShowGameWarning(EGameWarning::NotEnoughResources);

	if (bCanIssueWarning)
	{
		Client_OnGameWarningHappenedMissingResource(ResourceType);
	}
}

void ARTSPlayerState::OnGameWarningHappened(EHousingResourceType HousingResourceType)
{
	/* Could use another value here if desired like NotEnoughHousingResources */
	const bool bCanIssueWarning = CanShowGameWarning(EGameWarning::NotEnoughResources);

	if (bCanIssueWarning)
	{
		Client_OnGameWarningHappenedMissingHousingResource(HousingResourceType);
	}
}

void ARTSPlayerState::OnGameWarningHappened(EResourceType MissingResource)
{
	TryShowGameWarning_NotEnoughResources(MissingResource);
}

int32 ARTSPlayerState::SpendResource(EResourceType ResourceType, int32 AmountToSpend, bool bClampToWithinLimits)
{
	assert(GetWorld()->IsServer());

	/* Stop now plus don't want OnRep called - will cause divide by zero later on */
	if (AmountToSpend == 0)
	{
		return 0;
	}

	int32 & ResourceVariable = GetReplicatedResourceVariable(ResourceType);
	ResourceVariable -= AmountToSpend;

	if (bClampToWithinLimits)
	{
		ClampResource(ResourceVariable);
	}
	
	// Call explicity because on server 
	FunctionPtrType FuncPtr = GetReplicatedResourceOnRepFunction(ResourceType);
	(this->* (FuncPtr))();

	return ResourceVariable;
}

int32 ARTSPlayerState::GainResource(EResourceType ResourceType, int32 AmountToGain, bool bClampToWithinLimits)
{
	assert(GetWorld()->IsServer());

	/* Stop now plus don't want OnRep called - will cause divide by zero later on */
	if (AmountToGain == 0)
	{
		return 0;
	}

	int32 & ResourceVariable = GetReplicatedResourceVariable(ResourceType);
	ResourceVariable += AmountToGain;

	if (bClampToWithinLimits)
	{
		ClampResource(ResourceVariable);
	}

	// Call explicity because on server 
	FunctionPtrType FuncPtr = GetReplicatedResourceOnRepFunction(ResourceType);
	(this->* (FuncPtr))();

	return ResourceVariable;
}

void ARTSPlayerState::SpendResources(const TArray<int32>& CostArray, bool bClampToWithinLimits)
{
	assert(GetWorld()->IsServer());

	for (int32 i = 0; i < CostArray.Num(); ++i)
	{
		SpendResource(ArrayIndexToResourceType(i), CostArray[i], bClampToWithinLimits);
	}
}

void ARTSPlayerState::GainResources(const TArray<int32>& GainArray, bool bClampToWithinLimits)
{
	assert(GetWorld()->IsServer());

	for (int32 i = 0; i < GainArray.Num(); ++i)
	{
		GainResource(ArrayIndexToResourceType(i), GainArray[i], bClampToWithinLimits);
	}
}

void ARTSPlayerState::GainResources(int32 * GainArray, bool bClampToWithinLimits)
{
	SERVER_CHECK;

	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		GainResource(ArrayIndexToResourceType(i), GainArray[i], bClampToWithinLimits);
	}
}

void ARTSPlayerState::GainResources(const TArray<int32>& GainArray, float GainMultiplier, bool bClampToWithinLimits)
{
	SERVER_CHECK;

	if (GainMultiplier != 0.f)
	{
		for (int32 i = 0; i < GainArray.Num(); ++i)
		{
			GainResource(ArrayIndexToResourceType(i), GainArray[i] * GainMultiplier, bClampToWithinLimits);
		}
	}
}

void ARTSPlayerState::AdjustResources(const TArray<int32>& AdjustArray, int32 * PreAdjustAmounts, 
	int32 * PostAdjustAmounts, bool bClampToWithinLimits)
{
	SERVER_CHECK;
	assert(AdjustArray.Num() == Statics::NUM_RESOURCE_TYPES);
	
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		PreAdjustAmounts[i] = ResourceArray[i];
		PostAdjustAmounts[i] = GainResource(ArrayIndexToResourceType(i), AdjustArray[i], bClampToWithinLimits);
	}
}

void ARTSPlayerState::ClampResource(int32 & ResourceVariable)
{
	if (ResourceVariable < 0)
	{
		ResourceVariable = 0;
	}
}

void ARTSPlayerState::AddHousingResourceConsumption(const TArray<int16>& HousingCostArray)
{
	// Assert every resource type has an entry
	assert(HousingCostArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	for (int32 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		HousingResourceArray[i].AddConsumption(HousingCostArray[i]);
	}

	if (BelongsToLocalPlayer())
	{
		// Tell HUD 
		if (Statics::NUM_HOUSING_RESOURCE_TYPES > 0 && bIsABot == false)
		{
			HUDWidget->OnHousingResourceConsumptionChanged(HousingResourceArray);
		}
	}
}

void ARTSPlayerState::AddHousingResourceConsumption(EUnitType UnitType)
{
	const FUnitInfo * UnitInfo = FI->GetUnitInfo(UnitType);
	AddHousingResourceConsumption(UnitInfo->GetHousingCosts());
}

void ARTSPlayerState::RemoveHousingResourceConsumption(const TArray<int16>& HousingCostArray)
{
	assert(HousingCostArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	for (int32 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		HousingResourceArray[i].RemoveConsumption(HousingCostArray[i]);
	}

	if (BelongsToLocalPlayer())
	{
		// Tell HUD 
		if (Statics::NUM_HOUSING_RESOURCE_TYPES > 0 && bIsABot == false)
		{
			HUDWidget->OnHousingResourceConsumptionChanged(HousingResourceArray);
		}
	}
}

void ARTSPlayerState::RemoveHousingResourceConsumption(EUnitType UnitType)
{
	const FUnitInfo * UnitInfo = FI->GetUnitInfo(UnitType);
	RemoveHousingResourceConsumption(UnitInfo->GetHousingCosts());
}

void ARTSPlayerState::AddHousingResourcesProvided(const TArray<int16>& HousingProvidedArray)
{
	assert(HousingProvidedArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	const TArray < int32 > & MaxAmountsAllowed = FI->GetHousingResourceLimits();
	
	for (int32 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		HousingResourceArray[i].AddAmountProvided(HousingProvidedArray[i], MaxAmountsAllowed[i]);
	}

	if (BelongsToLocalPlayer())
	{
		// Tell HUD
		if (Statics::NUM_HOUSING_RESOURCE_TYPES > 0 && bIsABot == false)
		{
			HUDWidget->OnHousingResourceProvisionsChanged(HousingResourceArray);
		}
	}
}

void ARTSPlayerState::RemoveHousingResourcesProvided(const TArray<int16>& HousingProvidedArray)
{
	assert(HousingProvidedArray.Num() == Statics::NUM_HOUSING_RESOURCE_TYPES);
	
	const TArray < int32 > & MaxAmountsAllowed = FI->GetHousingResourceLimits();

	for (int32 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		HousingResourceArray[i].RemoveAmountProvided(HousingProvidedArray[i], MaxAmountsAllowed[i]);
	}

	if (BelongsToLocalPlayer())
	{
		// Tell HUD
		if (Statics::NUM_HOUSING_RESOURCE_TYPES > 0 && bIsABot == false)
		{
			HUDWidget->OnHousingResourceProvisionsChanged(HousingResourceArray);
		}
	}
}

int32 ARTSPlayerState::GetNumPersistentQueues() const
{
	return PersistentQueueSupportingBuildings.Num();
}

int32 ARTSPlayerState::GetNumSupportedProducers(const FContextButton & InButton) const
{
	return ProductionCapableBuildings[InButton].GetArray().Num();
}

const TArray<ABuilding*>& ARTSPlayerState::GetProductionCapableBuildings(const FContextButton & Button) const
{
	return ProductionCapableBuildings[Button].GetArray();
}

const TSet<ABuilding*>& ARTSPlayerState::GetPersistentQueueSupportingBuildings() const
{
	return PersistentQueueSupportingBuildings;
}

bool ARTSPlayerState::ArePrerequisitesMet(const FContextButton & InButton, bool bShowHUDMessage)
{
	const EContextButton ButtonType = InButton.GetButtonType();

	if (ButtonType == EContextButton::BuildBuilding)
	{
		return ArePrerequisitesMet(InButton.GetBuildingType(), bShowHUDMessage);
	}
	else if (ButtonType == EContextButton::Train)
	{
		return ArePrerequisitesMet(InButton.GetUnitType(), bShowHUDMessage);
	}
	else if (ButtonType == EContextButton::Upgrade)
	{
		const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(InButton.GetUpgradeType());
		
		return ArePrerequisitesMet(UpgradeInfo, InButton.GetUpgradeType(), bShowHUDMessage);
	}
	else // Button is for a custom ability
	{
		// Abilities don't have prereqs so return true
		return true;

		/* Note: this is called quite a bit. Perhaps see if there's any info on the widgets 
		that we can use to know ahead of time which of the 4 cases it is */
	}
}

bool ARTSPlayerState::ArePrerequisitesMet(EBuildingType BuildingType, bool bShowHUDMessage)
{
	const FBuildInfo * BuildInfo = FI->GetBuildingInfo(BuildingType);

	// Check prerequisites that are buildings
	for (const auto & Elem : BuildInfo->GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (BuildingTypeToMissingUpgradePrereqs.Contains(BuildingType) 
		&& BuildingTypeToMissingUpgradePrereqs[BuildingType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		return false;
	}

	return true;
}

bool ARTSPlayerState::ArePrerequisitesMet(EUnitType UnitType, bool bShowHUDMessage)
{
	const FBuildInfo * BuildInfo = FI->GetUnitInfo(UnitType);

	// Check prerequisites that are buildings
	for (const auto & Elem : BuildInfo->GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (UnitTypeToMissingUpgradePrereqs.Contains(UnitType) 
		&& UnitTypeToMissingUpgradePrereqs[UnitType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		return false;
	}

	return true;
}

bool ARTSPlayerState::ArePrerequisitesMet(EUpgradeType UpgradeType, bool bShowHUDMessage)
{
	const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(UpgradeType);
	return ArePrerequisitesMet(UpgradeInfo, UpgradeType, bShowHUDMessage);
}

bool ARTSPlayerState::ArePrerequisitesMet(const FUpgradeInfo & UpgradeInfo, EUpgradeType UpgradeType, bool bShowHUDMessage)
{
	// Check prerequisites that are buildings
	for (const auto & Elem : UpgradeInfo.GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (UpgradeTypeToMissingUpgradePrereqs.Contains(UpgradeType) 
		&& UpgradeTypeToMissingUpgradePrereqs[UpgradeType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		return false;
	}

	return true;
}

bool ARTSPlayerState::ArePrerequisitesMet(const FTrainingInfo & InTrainingInfo, bool bShowHUDMessage)
{
	if (InTrainingInfo.IsForUnit())
	{
		return ArePrerequisitesMet(InTrainingInfo.GetUnitType(), bShowHUDMessage);
	}
	else if (InTrainingInfo.IsProductionForBuilding())
	{
		return ArePrerequisitesMet(InTrainingInfo.GetBuildingType(), bShowHUDMessage);
	}
	else // Assuming for upgrade
	{
		assert(InTrainingInfo.IsForUpgrade());
		return ArePrerequisitesMet(InTrainingInfo.GetUpgradeType(), bShowHUDMessage);
	}
}

bool ARTSPlayerState::ArePrerequisitesMet(EBuildingType BuildingType, bool bShowHUDMessage, 
	EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade)
{
	const FBuildInfo * BuildInfo = FI->GetBuildingInfo(BuildingType);

	// Check prerequisites that are buildings
	for (const auto & Elem : BuildInfo->GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			OutFirstMissingPrereq_Building = Elem;

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (BuildingTypeToMissingUpgradePrereqs.Contains(BuildingType) 
		&& BuildingTypeToMissingUpgradePrereqs[BuildingType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		// Set this to NotBuilding so the caller knows it was an upgrade that was missing
		OutFirstMissingPrereq_Building = EBuildingType::NotBuilding;
		OutFirstMissingPrereq_Upgrade = BuildingTypeToMissingUpgradePrereqs[BuildingType].Array[0];
		return false;
	}

	return true;
}

bool ARTSPlayerState::ArePrerequisitesMet(EUnitType UnitType, bool bShowHUDMessage, 
	EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade)
{
	const FBuildInfo * BuildInfo = FI->GetUnitInfo(UnitType);

	// Check prerequisites that are buildings
	for (const auto & Elem : BuildInfo->GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			OutFirstMissingPrereq_Building = Elem;

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (UnitTypeToMissingUpgradePrereqs.Contains(UnitType) 
		&& UnitTypeToMissingUpgradePrereqs[UnitType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		// Set this to NotBuilding so the caller knows it was an upgrade that was missing
		OutFirstMissingPrereq_Building = EBuildingType::NotBuilding;
		OutFirstMissingPrereq_Upgrade = UnitTypeToMissingUpgradePrereqs[UnitType].Array[0];
		return false;
	}

	return true;
}

bool ARTSPlayerState::ArePrerequisitesMet(EUpgradeType UpgradeType, bool bShowHUDMessage, 
	EBuildingType & OutFirstMissingPrereq_Building, EUpgradeType & OutFirstMissingPrereq_Upgrade)
{
	const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(UpgradeType);
	
	// Check prerequisites that are buildings
	for (const auto & Elem : UpgradeInfo.GetPrerequisites())
	{
		if (!CompletedBuildingTypeQuantities.Contains(Elem) || CompletedBuildingTypeQuantities[Elem] == 0)
		{
			if (bShowHUDMessage)
			{
				// Try show HUD message
				OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
			}

			OutFirstMissingPrereq_Building = Elem;

			return false;
		}
	}

	// Check prerequisites that are upgrades
	if (UpgradeTypeToMissingUpgradePrereqs.Contains(UpgradeType)
		&& UpgradeTypeToMissingUpgradePrereqs[UpgradeType].Array.Num() > 0)
	{
		if (bShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::PrerequisitesNotMet);
		}
		
		// Set this to NotBuilding so the caller knows it was an upgrade that was missing
		OutFirstMissingPrereq_Building = EBuildingType::NotBuilding;
		OutFirstMissingPrereq_Upgrade = UpgradeTypeToMissingUpgradePrereqs[UpgradeType].Array[0];
		return false;
	}

	return true;
}

bool ARTSPlayerState::HasQueueSupport(const FContextButton & Button, bool bTryShowHUDWarnings)
{
	const bool bHasSupport = Button.IsForBuildBuilding() ? (GetNumPersistentQueues() > 0) : (GetNumSupportedProducers(Button));
	
	if (bTryShowHUDWarnings && !bHasSupport)
	{
		// Possibly the wrong warning type for units/upgrades
		OnGameWarningHappened(EGameWarning::CannotProduce);
	}

	return bHasSupport;
}

bool ARTSPlayerState::IsAtSelectableCap(bool bIncludeQueuedSelectables, bool bTryShowHUDMessage)
{
	assert(IDQueueNum >= 0);
	assert(IDQueueNum - NumQueuedSelectables >= 0);

	const bool bAtCap = bIncludeQueuedSelectables 
		? (IDQueueNum - NumQueuedSelectables == 0) 
		: (IDQueueNum == 0);

	if (bAtCap && bTryShowHUDMessage)
	{
		OnGameWarningHappened(EGameWarning::InternalSelectableCapReached);
	}

	return bAtCap;
}

uint8 ARTSPlayerState::GenerateSelectableID(AActor * Selectable)
{
	assert(GetWorld()->IsServer());
	assert(Selectable != nullptr);
	// Assumed to have checked this before calling this func
	assert(!IsAtSelectableCap(true, false));

	uint8 SelectableID; 
	const bool bResult = IDQueue.Dequeue(SelectableID);
	// If thrown means queue was empty
	assert(bResult);
	// 0 is reserved for a null type and should not even be in the queue in the first place
	assert(SelectableID != 0);

	IDQueueNum--;

	IDMap[SelectableID] = Selectable;

	return SelectableID;
}

void ARTSPlayerState::FreeUpSelectableID(ISelectable * Selectable)
{
	const uint8 SelectableID = Selectable->GetSelectableID();
	
	/* No longer allow this ID to map to this selectable. I guess assigning it nullptr works */
	IDMap[SelectableID] = nullptr;

	if (GetWorld()->IsServer())
	{
		/* Update ID queue. Makes this ID available again */
		IDQueue.Enqueue(SelectableID);
		IDQueueNum++;
	}
	else
	{
		/* Still update this on clients so they can try and keep track of whether they are at 
		selectable cap or not */
		IDQueueNum++;
	}
}

void ARTSPlayerState::Client_RegisterSelectableID(uint8 SelectableID, AActor * Selectable)
{	
	assert(!GetWorld()->IsServer());
	
	assert(SelectableID != 0 && Selectable != nullptr);
	IDMap[SelectableID] = Selectable;

	IDQueueNum--;
}

bool ARTSPlayerState::IsAtUniqueBuildingCap(EBuildingType BuildingType, bool bIncludeQueuedSelectables, 
	bool bTryShowHUDMessage)
{	
	/* This function is written with the assumption that a queue can have 1 building max in it */
	 
	if (FI->IsQuantityLimited(BuildingType))
	{
		/* Put entries in the TMaps if they're missing because we don't default initialize them 
		with 0 */
		if (InQueueBuildingTypeQuantities.Contains(BuildingType) == false)
		{
			InQueueBuildingTypeQuantities.Emplace(BuildingType, 0);
		}
		if (PlacedBuildingTypeQuantities.Contains(BuildingType) == false)
		{
			PlacedBuildingTypeQuantities.Emplace(BuildingType, 0);
		}
		if (CompletedBuildingTypeQuantities.Contains(BuildingType) == false)
		{
			CompletedBuildingTypeQuantities.Emplace(BuildingType, 0);
		}

		bool bAtCap;
		if (bIncludeQueuedSelectables)
		{
			const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
			const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
			if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
			{
				bAtCap = (InQueueBuildingTypeQuantities[BuildingType] + PlacedBuildingTypeQuantities[BuildingType] + CompletedBuildingTypeQuantities[BuildingType] == FI->GetQuantityLimit(BuildingType));
			}
			else
			{
				/* Note: the build method BuildsItself doubles up on the "InQueue" and "Placed" 
				containers i.e. if we place a building using BuildsItself it will get an entry 
				in both containers. This calculation here though is correct */
				bAtCap = (PlacedBuildingTypeQuantities[BuildingType] + CompletedBuildingTypeQuantities[BuildingType] == FI->GetQuantityLimit(BuildingType));
			}
		}
		else
		{
			bAtCap = (PlacedBuildingTypeQuantities[BuildingType] + CompletedBuildingTypeQuantities[BuildingType] == FI->GetQuantityLimit(BuildingType));
		}

		if (bAtCap && bTryShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::BuildingTypeQuantityLimit);
		}

		return bAtCap;
	}
	else
	{
		return false;
	}
}

bool ARTSPlayerState::IsAtUniqueUnitCap(EUnitType UnitType, bool bIncludeQueuedSelectables, 
	bool bTryShowHUDMessage)
{
	if (FI->IsQuantityLimited(UnitType))
	{
		UE_CLOG(InQueueUnitTypeQuantities.Contains(UnitType) == false, RTSLOG, Fatal,
			TEXT("Missing TMap entry for unit type [%s] for faction [%s] "), 
			TO_STRING(EUnitType, UnitType), TO_STRING(EFaction, FI->GetFaction()));
		
		const bool bAtCap = bIncludeQueuedSelectables
			? InQueueUnitTypeQuantities[UnitType] + UnitTypeQuantities[UnitType] == FI->GetQuantityLimit(UnitType)
			: UnitTypeQuantities[UnitType] == FI->GetQuantityLimit(UnitType);

		if (bAtCap && bTryShowHUDMessage)
		{
			OnGameWarningHappened(EGameWarning::UnitTypeQuantityLimit);
		}

		return bAtCap;
	}
	else
	{
		return false;
	}
}

bool ARTSPlayerState::IsAtUniqueSelectableCap(const FTrainingInfo & TrainingInfo, bool bIncludeQueuedSelectables,
	bool bTryShowHUDMessage)
{
	if (TrainingInfo.IsProductionForBuilding())
	{
		return IsAtUniqueBuildingCap(TrainingInfo.GetBuildingType(), bIncludeQueuedSelectables, bTryShowHUDMessage);
	}
	else // Assumed for unit
	{
		assert(TrainingInfo.IsForUnit());
		return IsAtUniqueUnitCap(TrainingInfo.GetUnitType(), bIncludeQueuedSelectables, bTryShowHUDMessage);
	}
}

bool ARTSPlayerState::IsAllied(const ARTSPlayerState * Other) const
{
	return (Team == Other->GetTeam());
}

EGameWarning ARTSPlayerState::CanBeTargetedByAbility(const FCommanderAbilityInfo & AbilityInfo,
	ARTSPlayerState * AbilityInstigator) const
{
	if (HasBeenDefeated())
	{
		return EGameWarning::TargetPlayerHasBeenDefeated;
	}

	if (Team != AbilityInstigator->GetTeam())
	{
		return AbilityInfo.CanTargetEnemies() ? EGameWarning::None : EGameWarning::TargetPlayerIsHostile;
	}
	else if (AbilityInstigator != this)
	{
		return AbilityInfo.CanTargetFriendlies() ? EGameWarning::None : EGameWarning::TargetPlayerIsAllied;
	}
	else
	{
		return AbilityInfo.CanTargetSelf() ? EGameWarning::None : EGameWarning::TargetPlayerIsSelf;
	}
}

FVector ARTSPlayerState::GetCommandLocation() const
{
	SERVER_CHECK;

	/* Just use the starting location. In future may want to have it change as buildings 
	are built or whatnot */
	return StartLocation;
}

void ARTSPlayerState::SetupProductionCapableBuildingsMap()
{
	/* Default initialize hashmap */
	for (const auto & Elem : FI->GetHUDPersistentTabButtons())
	{
		const TArray < FContextButton > & ButtonArray = Elem.Value.GetButtons();

		for (const auto & Button : ButtonArray)
		{
			ProductionCapableBuildings.Emplace(Button, FBuildingArray());
		}
	}
}

FBuildingNetworkState * ARTSPlayerState::GetBuildingGarrisonNetworkInfo(EBuildingNetworkType GarrisonNetworkType)
{
	return BuildingGarrisonNetworkInfos.GetNetworkInfo(GarrisonNetworkType);
}


//---------------------------------------------------------------
//	Audio
//---------------------------------------------------------------

bool ARTSPlayerState::ShouldPlayBuildingBuiltSound_ConstructionComplete(const FBuildingInfo & BuildingInfo, ESelectableCreationMethod CreationMethod)
{
	if (BuildingInfo.GetJustBuiltSound() != nullptr)
	{
		// Never play 'just built' sound if player started match with this
		if (CreationMethod != ESelectableCreationMethod::StartingSelectable)
		{
			if (bBelongsToLocalPlayer)
			{
				// Return true if currently not playing any sound
				return BuildingBuiltAudioComp->IsPlaying() == false;
			}
			else
			{
				if (BuildingInfo.AnnounceToAllWhenBuilt())
				{
					// Return true if currently not playing any sound
					return BuildingBuiltAudioComp->IsPlaying() == false;
				}
			}
		}
	}

	return false;
}

void ARTSPlayerState::PlayBuildingBuiltSound(USoundBase * SoundToPlay)
{
	assert(SoundToPlay != nullptr);

	BuildingBuiltAudioComp->SetSound(SoundToPlay);

	if (!BuildingBuiltAudioComp->IsPlaying())
	{
		BuildingBuiltAudioComp->Play();
	}
}

bool ARTSPlayerState::ShouldPlayUnitBuiltSound(EUnitType UnitJustBuilt, const FBuildInfo & UnitInfo,
	ESelectableCreationMethod CreationMethod)
{
	/* This function will be called whenever any player builds a unit. Also note that the
	owners player state gets this called on it meaning different audio comps play sounds for
	different players */

	if (UnitInfo.GetJustBuiltSound() != nullptr)
	{
		/* Never play 'just built' sound if unit was one of the starting units */
		if (CreationMethod != ESelectableCreationMethod::StartingSelectable)
		{
			if (bBelongsToLocalPlayer)
			{
				// If not playing a sound then return true
				return UnitBuiltAudioComp->IsPlaying() == false;
			}
			else
			{
				/* This checks if unit is similar to a Kirov in Red Alert II where everyone gets a
				notification when it is built */
				if (UnitInfo.AnnounceToAllWhenBuilt())
				{
					// If not playing a sound then return true
					return UnitBuiltAudioComp->IsPlaying() == false;
				}
			}
		}
	}

	return false;
}

void ARTSPlayerState::PlayUnitBuiltSound(USoundBase * SoundToPlay)
{
	assert(SoundToPlay != nullptr);

	// Just setting this while it is already playing should play the sound
	UnitBuiltAudioComp->SetSound(SoundToPlay);

	if (!UnitBuiltAudioComp->IsPlaying())
	{
		UnitBuiltAudioComp->Play();
	}
}

void ARTSPlayerState::OnDefeated()
{
	bHasBeenDefeated = true;
	
	ARTSPlayerController * LocalPC = CastChecked<ARTSPlayerController>(GetWorld()->GetFirstPlayerController());

	/* Check if we (the local player) were defeated */
	if (this == LocalPC->GetPS())
	{
		// We were one of the players defeated. Disable our game input
		LocalPC->DisableInput(LocalPC);

		if (!bIsABot)
		{
			// Make HUD uninteractable
			LocalPC->DisableHUD();

			// Show a defeat widget
			LocalPC->ShowWidget(EMatchWidgetType::Defeat, true);
		}
	}
	else
	{
		/* OnGameEventHappened does a client RPC so message will only go to the defeated player which 
		is not what we want */
		// We are not one of the players defeated. Show notification on HUD
		//OnGameEventHappened(EGameNotification::PlayerDefeated);

		if (!bIsABot)
		{
			HUDWidget->OnAnotherPlayerDefeated(this);
		}
	}

	if (GetWorld()->IsServer())
	{
		/* Server has a bit more stuff to do:
		- Destroy all selectables the player owns */

		/* Getting crashes here when playing in PIE with CPU players because this function
		is actually getting called when playing with CPUs, but doesn't when not */

		/* Destroy all selectables this player owns */
		for (auto & Elem : Buildings)
		{
			Elem->Server_OnOwningPlayerDefeated();
		}
		for (auto & Elem : Units)
		{
			Elem->Server_OnOwningPlayerDefeated();
		}

		/* Flagging the player controller to not issue commands shouldn't be required since all
		selectables are given a notification that their player has been defeated which should
		set them to zero health which should make commands have no effect on them anyway */
	}
	else
	{
		// Server already did this, do it on client too to keep up to date
		GS->GetUndefeatedPlayers().RemoveSwap(this);
	}
}

AFactionInfo * ARTSPlayerState::GetFI() const
{
	return FI;
}

void ARTSPlayerState::SetFactionInfo(AFactionInfo * NewInfo)
{
	assert(NewInfo != nullptr);
	FI = NewInfo;

	NumUnspentSkillPoints = FI->GetNumInitialCommanderSkillPoints();
}

ARTSPlayerController * ARTSPlayerState::GetPC() const
{
	return PC;
}

void ARTSPlayerState::SetPC(ARTSPlayerController * PlayerController)
{
	PC = PlayerController;
}

ACPUPlayerAIController * ARTSPlayerState::GetAIController() const
{
	return AICon;
}

void ARTSPlayerState::SetAIController(ACPUPlayerAIController * InAIController)
{
	AICon = InAIController;
}

ARTSGameState * ARTSPlayerState::GetGS() const
{
	return GS;
}

void ARTSPlayerState::SetGS(ARTSGameState * GameState)
{
	GS = GameState;
}

void ARTSPlayerState::SetGI(URTSGameInstance * GameInstance)
{
	assert(GameInstance != nullptr);

	GI = GameInstance;
}

uint8 ARTSPlayerState::GetBuildingIndex(EBuildingType Type) const
{
	return static_cast<uint8>(Type);
}

uint8 ARTSPlayerState::GetPlayerIDAsInt() const
{
	return IDAsInt;
}

FName ARTSPlayerState::GetPlayerID() const
{
	assert(ID != NAME_None); // If calling this it is assumed you wouldn't want this
	return ID;
}

ETeam ARTSPlayerState::GetTeam() const
{
	return Team;
}

void ARTSPlayerState::SetTeam(ETeam NewValue)
{
	Team = NewValue;
}

EAffiliation ARTSPlayerState::GetAffiliation() const
{
	return Affiliation;
}

void ARTSPlayerState::SetAffiliation(EAffiliation InAffiliation)
{
	Affiliation = InAffiliation;
}

const FName & ARTSPlayerState::GetTeamTag() const
{
	assert(TeamTag != NAME_None); // If calling this it is assumed you wouldn't want this
	return TeamTag;
}

void ARTSPlayerState::SetTeamTag(const FName & InTag)
{
	assert(InTag != FName());
	TeamTag = InTag;
}

EFaction ARTSPlayerState::GetFaction() const
{
	return Faction;
}

int16 ARTSPlayerState::GetStartingSpot() const
{
	return StartingSpot;
}

void ARTSPlayerState::AICon_SetFinalStartingSpot(int16 InStartingSpot)
{
	StartingSpot = InStartingSpot;

	// AI controller likes to know about this too
	AICon->SetInitialLocAndRot(InStartingSpot);
}

const TArray<ABuilding*>& ARTSPlayerState::GetBuildings() const
{
	return Buildings;
}

const TArray <AInfantry *> & ARTSPlayerState::GetUnits() const
{
	return Units;
}

TMap<EBuildingType, int32>& ARTSPlayerState::GetPrereqInfo()
{
	return CompletedBuildingTypeQuantities;
}

AUpgradeManager * ARTSPlayerState::GetUpgradeManager() const
{
	return UpgradeManager;
}

AActor * ARTSPlayerState::GetSelectableFromID(uint8 SelectableID) const
{
	assert(SelectableID != 0);

	/* Returning null can mean no entry was ever there because Init(nullptr, X) is called
	on this in ctor */
	return IDMap[SelectableID];
}

const FSlateBrush * ARTSPlayerState::GetPlayerNormalImage()
{
#if WITH_EDITOR
	if (PlayerNormalImage.GetResourceObject() == nullptr)
	{
		AssignPlayerImages();
	}
#endif

	return &PlayerNormalImage;
}

const FSlateBrush * ARTSPlayerState::GetPlayerHoveredImage()
{
#if WITH_EDITOR
	if (PlayerHoveredImage.GetResourceObject() == nullptr)
	{
		AssignPlayerImages();
	}
#endif

	return &PlayerHoveredImage;
}

const FSlateBrush * ARTSPlayerState::GetPlayerPressedImage()
{
#if WITH_EDITOR
	if (PlayerPressedImage.GetResourceObject() == nullptr)
	{
		AssignPlayerImages();
	}
#endif

	return &PlayerPressedImage;
}

URTSHUD * ARTSPlayerState::GetHUDWidget() const
{
	return HUDWidget;
}

#if WITH_EDITOR
void ARTSPlayerState::AssignPlayerImages()
{
	/* If we do not have an image assigned then use an engine image */
	FString Path;
	switch (GetPlayerIDAsInt())
	{
	case 1:
	{
		Path = TEXT("Texture2D'/Engine/EditorResources/Ai_Spawnpoint.Ai_Spawnpoint'");
		break;
	}
	case 2:
	{
		Path = TEXT("Texture2D'/Engine/EditorResources/S_TextRenderActorIcon.S_TextRenderActorIcon'");
		break; 
	}
	case 3:
	{
		Path = TEXT("Texture2D'/Engine/EditorResources/S_Pawn.S_Pawn'");
		break;
	}
	case 4:
	{
		Path = TEXT("Texture2D'/Engine/EditorResources/Spawn_Point.Spawn_Point'");
		break;
	}
	}

	UTexture2D * Image = LoadObject<UTexture2D>(nullptr, *Path);
	PlayerNormalImage.SetResourceObject(Image);
	PlayerHoveredImage.SetResourceObject(Image);
	PlayerHoveredImage.TintColor = FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 1.f));
	PlayerPressedImage.SetResourceObject(Image);
	PlayerPressedImage.TintColor = FSlateColor(FLinearColor(1.f, 0.f, 0.f, 1.f));
}
#endif

