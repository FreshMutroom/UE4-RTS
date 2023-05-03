// Fill out your copyright notice in the Description page of Project Settings.

#include "CPUPlayerAIController.h"
#include "Engine/World.h"
#include "Public/TimerManager.h"
#include "VisualLogger/VisualLogger.h"

#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameState.h"
#include "GameFramework/RTSGameInstance.h"
#include "Statics/DevelopmentStatics.h"
#include "GameFramework/FactionInfo.h"
#include "MapElements/Building.h"
#include "MapElements/Infantry.h"
#include "Managers/UpgradeManager.h"
#include "Statics/Statics.h"
#include "MapElements/ResourceSpot.h"


//==============================================================================================
//	Constant Statics
//==============================================================================================

/* Put the original location first, then gradualy move outwards. Really need a diagram to show
this */
const FVector2D ACPUPlayerAIController::BuildLocMultipliers[BUILD_LOC_MULTIPLERS_NUM] = {
	// Original location 
	FVector2D(0.f, 0.f),

	// Shifted 1
	FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(0.f, -1.f), FVector2D(-1.f, 0.f),
	FVector2D(1.f, -1.f), FVector2D(-1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-1.f, -1.f),

	// Shifted 1 / 2 (less than before)
	FVector2D(1.f, 0.f) * 0.5f, FVector2D(0.f, 1.f) * 0.5f, FVector2D(0.f, -1.f) * 0.5f, FVector2D(-1.f, 0.f) * 0.5f,
	FVector2D(1.f, -1.f) * 0.5f, FVector2D(-1.f, 1.f) * 0.5f, FVector2D(1.f, 1.f) * 0.5f, FVector2D(-1.f, -1.f) * 0.5f,

	// Shifted by 2
	FVector2D(1.f, 0.f) * 2, FVector2D(0.f, 1.f) * 2, FVector2D(0.f, -1.f) * 2, FVector2D(-1.f, 0.f) * 2,
	FVector2D(1.f, -1.f) * 2, FVector2D(-1.f, 1.f) * 2, FVector2D(1.f, 1.f) * 2, FVector2D(-1.f, -1.f) * 2,

	// Gaps of shifted by 2
	FVector2D(1.f, 0.5f) * 2, FVector2D(0.5f, 1.f) * 2, FVector2D(0.5f, -1.f) * 2, FVector2D(-1.f, 0.5f) * 2,
	FVector2D(1.f, -0.5f) * 2, FVector2D(-0.5f, 1.f) * 2, FVector2D(1.f, 0.5f) * 2, FVector2D(-0.5f, -1.f) * 2,
};

const float ACPUPlayerAIController::CIRCLE_PORTIONS_IN_RADIANS = (2.f * PI) / NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY;

/* 16 unit vectors representing different rotations of a circle all evenly spaced apart. They are 
ordered so that the 4 corners approach is tried instead of just slowly moving around the circle 
because that way if the previous spot did not work then there's a good chance the next won't either
Maybe better to just calculate them on the fly instead of caching them now? Don't even need 
exact sin/cos, approximate would be fine */
const FVector2D ACPUPlayerAIController::ResourceDepotBuildLocs[NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY] = {
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 0.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 0.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 4.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 4.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 8.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 8.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 12.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 12.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 2.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 2.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 6.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 6.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 10.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 10.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 14.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 14.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 1.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 1.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 5.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 5.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 9.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 9.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 13.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 13.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 3.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 3.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 7.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 7.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 11.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 11.f)),
	FVector2D(FMath::Cos(CIRCLE_PORTIONS_IN_RADIANS * 15.f), FMath::Sin(CIRCLE_PORTIONS_IN_RADIANS * 15.f))
};


//==============================================================================================
//	Structs
//==============================================================================================

//==============================================================================================
//	Basic Command Reason Struct
//==============================================================================================

FMacroCommandReason::FMacroCommandReason()
{
	// Apparently this causes crashes on editor startup
	//assert(0);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, 
	EBuildingType InBuildingType, EMacroCommandType InReason, EResourceType InResourceType)
{
	ActualCommand = InActualCommand;
	AuxilleryInfo = static_cast<uint8>(InBuildingType);

	OriginalCommand_PrimaryReason = InReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InResourceType);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, 
	EUnitType InUnitType, EMacroCommandType InReason, EResourceType InResourceType)
{
	ActualCommand = InActualCommand;
	AuxilleryInfo = static_cast<uint8>(InUnitType);

	OriginalCommand_PrimaryReason = InReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InResourceType);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InActualCommand, 
	EUnitType InUnitType, EMacroCommandType InReason)
{
	ActualCommand = InActualCommand;
	AuxilleryInfo = static_cast<uint8>(InUnitType);

	OriginalCommand_PrimaryReason = InReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InUnitType);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InActualCommand,
	EUpgradeType InUpgrade, EMacroCommandType InReason)
{
	ActualCommand = InActualCommand;
	AuxilleryInfo = static_cast<uint8>(InUpgrade);

	OriginalCommand_PrimaryReason = InReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InUpgrade);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InCommand, EBuildingType InBuildingType, 
	EMacroCommandType InCommandReason, EBuildingType InBuildingForCommandReason)
{
	assert(InCommand == EMacroCommandSecondaryType::BuildingBuilding);
	
	ActualCommand = InCommand;
	AuxilleryInfo = static_cast<uint8>(InBuildingType);

	OriginalCommand_PrimaryReason = InCommandReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InBuildingForCommandReason);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InCommand, EUnitType InUnitType, 
	EMacroCommandType InCommandReason, EBuildingType InBuildingForCommandReason)
{
	assert(InCommand == EMacroCommandSecondaryType::TrainingUnit);

	// Commented this just to get things working
	//assert(0);

	ActualCommand = InCommand;
	AuxilleryInfo = static_cast<uint8>(InUnitType);

	OriginalCommand_PrimaryReason = InCommandReason;
	OriginalCommand_AuxilleryInfo = static_cast<uint8>(InBuildingForCommandReason);
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InReason, EResourceType InResourceType, 
	const FMacroCommandReason & OriginalCommand)
{
	assert(0);

	ActualCommand = InReason;
	AuxilleryInfo = static_cast<uint8>(InResourceType);
	
	OriginalCommand_PrimaryReason = OriginalCommand.OriginalCommand_PrimaryReason;
	OriginalCommand_AuxilleryInfo = OriginalCommand.OriginalCommand_AuxilleryInfo;
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InReason, EBuildingType InBuildingType, 
	const FMacroCommandReason & OriginalCommand)
{
	// Commented this just to get things working
	//assert(0);

	ActualCommand = InReason;
	AuxilleryInfo = static_cast<uint8>(InBuildingType);

	OriginalCommand_PrimaryReason = OriginalCommand.OriginalCommand_PrimaryReason;
	OriginalCommand_AuxilleryInfo = OriginalCommand.OriginalCommand_AuxilleryInfo;
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InReason, EUnitType InUnitType, 
	const FMacroCommandReason & OriginalCommand)
{
	// Commented this to get this running. Maybe bad
	//assert(0);
	
	ActualCommand = InReason;
	AuxilleryInfo = static_cast<uint8>(InUnitType);

	OriginalCommand_PrimaryReason = OriginalCommand.OriginalCommand_PrimaryReason;
	OriginalCommand_AuxilleryInfo = OriginalCommand.OriginalCommand_AuxilleryInfo;
}

FMacroCommandReason::FMacroCommandReason(EMacroCommandSecondaryType InReason, EUpgradeType InUpgradeType, 
	const FMacroCommandReason & OriginalCommand)
{
	ActualCommand = InReason;
	AuxilleryInfo = static_cast<uint8>(InUpgradeType);

	OriginalCommand_PrimaryReason = OriginalCommand.OriginalCommand_PrimaryReason;
	OriginalCommand_AuxilleryInfo = OriginalCommand.OriginalCommand_AuxilleryInfo;
}

FMacroCommandReason::FMacroCommandReason(const FCPUPlayerTrainingInfo & InInfo)
{
	ActualCommand = InInfo.GetActualCommand();
	AuxilleryInfo = InInfo.GetRawAuxilleryInfo();

	OriginalCommand_PrimaryReason = InInfo.GetOriginalCommandReasonForProduction();
	OriginalCommand_AuxilleryInfo = InInfo.GetOriginalCommandAuxilleryInfo();
}

EMacroCommandSecondaryType FMacroCommandReason::GetActualCommand() const
{
	return ActualCommand;
}

uint8 FMacroCommandReason::GetRawAuxilleryReason() const
{
	return AuxilleryInfo;
}

EResourceType FMacroCommandReason::OriginalCommand_GetResourceType() const
{
	/* These are the only command types where resource type should be relevant */
	assert(OriginalCommand_PrimaryReason == EMacroCommandType::BuildResourceDepot
		|| OriginalCommand_PrimaryReason == EMacroCommandType::TrainCollector);
	
	return static_cast<EResourceType>(OriginalCommand_AuxilleryInfo);
}

EBuildingType FMacroCommandReason::GetBuildingType() const
{
	assert(ActualCommand == EMacroCommandSecondaryType::BuildingBuilding);
	
	return static_cast<EBuildingType>(AuxilleryInfo);
}

EUnitType FMacroCommandReason::GetUnitType() const
{
	assert(ActualCommand == EMacroCommandSecondaryType::TrainingUnit);
	
	return static_cast<EUnitType>(AuxilleryInfo);
}

EUpgradeType FMacroCommandReason::GetUpgradeType() const
{
	/* Type of commands that support this i.e. they set it 100% of the time */
	assert(ActualCommand == EMacroCommandSecondaryType::ResearchingUpgrade);
	
	return static_cast<EUpgradeType>(AuxilleryInfo);
}

EMacroCommandType FMacroCommandReason::GetOriginalCommandMainReason() const
{
	return OriginalCommand_PrimaryReason;
}

uint8 FMacroCommandReason::GetOriginalCommandAuxilleryData() const
{
	return OriginalCommand_AuxilleryInfo;
}


//==============================================================================================
//	Saving Up Info Struct
//==============================================================================================

FSavingUpForInfo::FSavingUpForInfo()
{
	bSavingUp = false;
	bSavingForResourceDepot = false;
	Building = EBuildingType::NotBuilding;
	Unit = EUnitType::NotUnit;
	Upgrade = EUpgradeType::None;
	TargetResourceSpot = nullptr;
	ProducerBuilding = nullptr;
	ProducerWorker = nullptr;
}

void FSavingUpForInfo::SetSavingsTarget(EBuildingType BuildingType, ABuilding * InProducerBuilding, 
	AResourceSpot * ResourceSpot)
{
	bSavingUp = true;
	bSavingForResourceDepot = true;
	Building = BuildingType;
	Unit = EUnitType::NotUnit;
	Upgrade = EUpgradeType::None;
	TargetResourceSpot = ResourceSpot;
	ProducerBuilding = InProducerBuilding;
	ProducerWorker = nullptr;
	CommandReason = FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, BuildingType, 
		EMacroCommandType::BuildResourceDepot, ResourceSpot->GetType());
}

void FSavingUpForInfo::SetSavingsTarget(EBuildingType BuildingType, AInfantry * InWorker, 
	AResourceSpot * ResourceSpot)
{
	bSavingUp = true;
	bSavingForResourceDepot = true;
	Building = BuildingType;
	Unit = EUnitType::NotUnit;
	Upgrade = EUpgradeType::None;
	TargetResourceSpot = ResourceSpot;
	ProducerBuilding = nullptr;
	ProducerWorker = InWorker;
	CommandReason = FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, BuildingType, 
		EMacroCommandType::BuildResourceDepot, ResourceSpot->GetType());
}

void FSavingUpForInfo::SetSavingsTarget(EBuildingType BuildingType, ABuilding * InProducerBuilding, 
	FMacroCommandReason InCommandReason)
{
	bSavingUp = true;
	bSavingForResourceDepot = false;
	Building = BuildingType;
	Unit = EUnitType::NotUnit;
	Upgrade = EUpgradeType::None;
	ProducerBuilding = InProducerBuilding;
	ProducerWorker = nullptr;
	CommandReason = InCommandReason;
}

void FSavingUpForInfo::SetSavingsTarget(EBuildingType BuildingType, AInfantry * InWorker, 
	FMacroCommandReason InCommandReason)
{
	bSavingUp = true;
	bSavingForResourceDepot = false;
	Building = BuildingType;
	Unit = EUnitType::NotUnit;
	Upgrade = EUpgradeType::None;
	ProducerBuilding = nullptr;
	ProducerWorker = InWorker;
	CommandReason = InCommandReason;
}

void FSavingUpForInfo::SetSavingsTarget(EUnitType UnitType, ABuilding * InBarracks,
	FMacroCommandReason InCommandReason)
{
	bSavingUp = true;
	Building = EBuildingType::NotBuilding;
	Unit = UnitType;
	Upgrade = EUpgradeType::None;
	ProducerBuilding = InBarracks;
	ProducerWorker = nullptr;
	CommandReason = InCommandReason;
}

void FSavingUpForInfo::SetSavingsTarget(EUpgradeType UpgradeType, ABuilding * InProducerBuilding, 
	FMacroCommandReason InCommandReason)
{
	bSavingUp = true;
	Building = EBuildingType::NotBuilding;
	Unit = EUnitType::NotUnit;
	Upgrade = UpgradeType;
	ProducerBuilding = InProducerBuilding;
	ProducerWorker = nullptr;
	CommandReason = InCommandReason;
}

bool FSavingUpForInfo::IsSavingUp() const
{
	return bSavingUp;
}

bool FSavingUpForInfo::IsForBuilding() const
{
	return Building != EBuildingType::NotBuilding;
}

bool FSavingUpForInfo::IsForUnit() const
{
	return Unit != EUnitType::NotUnit;
}

bool FSavingUpForInfo::IsForUpgrade() const
{
	return Upgrade != EUpgradeType::None;
}

bool FSavingUpForInfo::HasEnoughResourcesForIt(ACPUPlayerAIController * AICon) const
{
	if (Building != EBuildingType::NotBuilding)
	{
		return AICon->GetPS()->HasEnoughResources(Building, false);
	}
	else if (Unit != EUnitType::NotUnit)
	{
		return AICon->GetPS()->HasEnoughResources(Unit, false);
	}
	else // Assumed for upgrade
	{
		assert(Upgrade != EUpgradeType::None);
		return AICon->GetPS()->HasEnoughResources(Upgrade, false);
	}
}

bool FSavingUpForInfo::ArePrerequisitesMetForIt(ACPUPlayerAIController * AICon, 
	EBuildingType & OutMissingPrereq_Building, EUpgradeType & OutMissingPrereq_Upgrade) const
{
	if (IsForBuilding())
	{
		return AICon->GetPS()->ArePrerequisitesMet(Building, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
	else if (IsForUnit())
	{
		return AICon->GetPS()->ArePrerequisitesMet(Unit, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
	else // Assumed for upgrade
	{
		assert(IsForUpgrade());
		return AICon->GetPS()->ArePrerequisitesMet(Upgrade, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
}

bool FSavingUpForInfo::HasBuilderForIt(ACPUPlayerAIController * AICon, bool bTryFindIfNone)
{
	/* Don't check queue here but will probably want to */
	
	const bool bHaveProducer = (ProducerBuilding.IsValid() && !Statics::HasZeroHealth(ProducerBuilding.Get()))
		|| (ProducerWorker.IsValid() && !Statics::HasZeroHealth(ProducerWorker.Get()));
	
	if (bTryFindIfNone)
	{
		if (bHaveProducer)
		{
			return true;
		}
		else
		{
			/* Check player's selectables to see if we can find a producer */

			if (IsForBuilding())
			{
				const FBuildingInfo * BuildingInfo = AICon->GetFI()->GetBuildingInfo(Building);

				// Try construction yards
				if (BuildingInfo->CanBeBuiltByConstructionYard())
				{
					// Find yard with best queue. Note iterating TSet
					for (const auto & ConstructionYard : AICon->GetPS()->GetPersistentQueueSupportingBuildings())
					{
						// Eh just use first one from iteration. Lazy
						ProducerBuilding = ConstructionYard;
						return true;
					}
				}
				
				// Try worker units
				if (BuildingInfo->CanBeBuiltByWorkers())
				{
					for (const auto & Worker : AICon->IdleWorkers)
					{
						// This func will at least need to check if button is on context menu
						if (AICon->CanUseIdleWorker(Worker.Get(), Building))
						{
							ProducerWorker = Worker;
							return true;
						}
					}
				}

				return false;
			}
			else if (IsForUnit())
			{
				/* Find a barracks that can build this */
				ABuilding * Barracks = nullptr;
				for (const auto & Elem : AICon->GetPS()->GetBuildings())
				{
					if (Elem->GetContextMenu()->HasButton(FContextButton(Unit)))
					{
						if (Elem->GetContextProductionQueue().AICon_HasRoom())
						{
							// Just using first barracks with room here. Should check queue times
							ProducerBuilding = Elem;
							return true;
						}
					}
				}

				return false;
			}
			else // Assumed for upgrade
			{
				assert(IsForUpgrade());
				/* This is exactly same code as for unit except checking for upgrade in context 
				menu now */

				/* Find a building that can research this */
				for (const auto & Elem : AICon->GetPS()->GetBuildings())
				{
					if (Elem->GetContextMenu()->HasButton(FContextButton(Upgrade)))
					{
						const FProductionQueue & Queue = Elem->GetContextProductionQueue();
						
						if (Queue.AICon_HasRoom())
						{
							// Update field and return true
							ProducerBuilding = Elem;
							return true;
						}
					}
				}

				return false;
			}
		}
	}
	else
	{
		return bHaveProducer;
	}
}

void FSavingUpForInfo::BuildIt(ACPUPlayerAIController * AICon)
{
	// Assumption that prereqs are met, enough resources and has builder
#if !UE_BUILD_SHIPPING
	EBuildingType Dummy_Building;
	EUpgradeType Dummy_Upgrade;
	assert(ArePrerequisitesMetForIt(AICon, Dummy_Building, Dummy_Upgrade));
	assert(HasEnoughResourcesForIt(AICon));
	assert(HasBuilderForIt(AICon, false));
#endif 

	if (IsForBuilding())
	{
		if (bSavingForResourceDepot && TargetResourceSpot.IsValid())
		{
			if (ProducerBuilding != nullptr)
			{
				AICon->TryIssueBuildResourceDepotCommand(Building, ProducerBuilding.Get(), 
					TargetResourceSpot.Get(), CommandReason);
			}
			else // Assuming using ProducerWorker
			{
				assert(ProducerWorker.IsValid());
				AICon->TryIssueBuildResourceDepotCommand(Building, ProducerWorker.Get(),
					TargetResourceSpot.Get(), CommandReason);
			}
		}
		else
		{
			if (ProducerBuilding != nullptr)
			{
				AICon->TryIssueBuildBuildingCommand(Building, ProducerBuilding.Get(), 
					CommandReason);
			}
			else // Assumed using ProducerWorker
			{
				assert(ProducerWorker.IsValid());
				AICon->TryIssueBuildBuildingCommand(Building, ProducerWorker.Get(), CommandReason);
			}
		}
	}
	else if (IsForUnit())
	{
		AICon->ActuallyIssueTrainUnitCommand(Unit, ProducerBuilding.Get(), CommandReason);

		/* We incremented pending commands when we set our savings target. The "Try/Actually" functions
		we're about to call will also increment it so decrement it here now */
		AICon->DecrementNumPendingCommands(CommandReason);
	}
	else // Assumed for upgrade
	{
		assert(IsForUpgrade());
		
		AICon->ActuallyIssueResearchUpgradeCommand(Upgrade, ProducerBuilding.Get(), CommandReason);

		/* We incremented pending commands when we set our savings target. The "Try/Actually" functions
		we're about to call will also increment it so decrement it here now */
		AICon->DecrementNumPendingCommands(CommandReason);
	}

	/* Store that we are no longer saving up by resetting this */
	bSavingUp = false;
}


//=============================================================================================
//	Struct for waiting for a production queue to not be full capacity
//=============================================================================================

bool FQueueWaitingInfo::IsForBuilding() const
{
	return Building != EBuildingType::NotBuilding;
}

bool FQueueWaitingInfo::IsForUnit() const
{
	return Unit != EUnitType::None;
}

bool FQueueWaitingInfo::IsForUpgrade() const
{
	return Upgrade != EUpgradeType::None;
}

FQueueWaitingInfo::FQueueWaitingInfo()
{
	bWaiting = false;
}

void FQueueWaitingInfo::WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EBuildingType WhatWeWantToBuild, FMacroCommandReason InCommandReason)
{
	bWaiting = true;
	QueueOwner = InQueueOwner;
	Queue = TargetQueue;
	Building = WhatWeWantToBuild;
	Unit = EUnitType::None;
	Upgrade = EUpgradeType::None;
	CommandReason = InCommandReason;
}

void FQueueWaitingInfo::WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EUnitType WhatWeWantToBuild, FMacroCommandReason InCommandReason)
{
	bWaiting = true;
	QueueOwner = InQueueOwner;
	Queue = TargetQueue;
	Building = EBuildingType::NotBuilding;
	Unit = WhatWeWantToBuild;
	Upgrade = EUpgradeType::None;
	CommandReason = InCommandReason;
}

void FQueueWaitingInfo::WaitForQueue(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EUpgradeType WhatWeWantToBuild, FMacroCommandReason InCommandReason)
{
	bWaiting = true;
	QueueOwner = InQueueOwner;
	Queue = TargetQueue;
	Building = EBuildingType::NotBuilding;
	Unit = EUnitType::None;
	Upgrade = WhatWeWantToBuild;
	CommandReason = InCommandReason;
}

bool FQueueWaitingInfo::IsWaiting() const
{
	return bWaiting;
}

bool FQueueWaitingInfo::IsQueueOwnerValidAndAlive() const
{
	return QueueOwner.IsValid() && !Statics::HasZeroHealth(QueueOwner.Get());
}

bool FQueueWaitingInfo::IsQueueNowUnFull() const
{
	return false;
}

bool FQueueWaitingInfo::ArePrerequisitesMetForIt(ACPUPlayerAIController * AICon, 
	EBuildingType & OutMissingPrereq_Building, EUpgradeType & OutMissingPrereq_Upgrade) const
{
	if (IsForBuilding())
	{
		return AICon->GetPS()->ArePrerequisitesMet(Building, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
	else if(IsForUnit())
	{
		return AICon->GetPS()->ArePrerequisitesMet(Unit, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
	else // Assumed for upgrade
	{
		assert(IsForUpgrade());
		return AICon->GetPS()->ArePrerequisitesMet(Upgrade, false, OutMissingPrereq_Building, OutMissingPrereq_Upgrade);
	}
}

bool FQueueWaitingInfo::HasEnoughResourcesForIt(ACPUPlayerAIController * AICon) const
{
	if (IsForBuilding())
	{
		return AICon->GetPS()->HasEnoughResources(Building, false);
	}
	else if (IsForUnit())
	{
		return AICon->GetPS()->HasEnoughResources(Unit, false);
	}
	else // Assumed for upgrade
	{
		assert(IsForUpgrade());
		return AICon->GetPS()->HasEnoughResources(Upgrade, false);
	}
}

void FQueueWaitingInfo::BuyIt()
{
	// Make sure to decrement the pending commands 


	bWaiting = false;
}

void FQueueWaitingInfo::CancelWaiting()
{
	bWaiting = false;
}


//==============================================================================================
//	Struct for trying to place a building (regular buildings and resource depot buildings) 
//==============================================================================================

FTryingToPlaceBuildingInfo::FTryingToPlaceBuildingInfo()
{
	PendingBuilding = EBuildingType::NotBuilding;
}

void FTryingToPlaceBuildingInfo::BeginForResourceDepot(EBuildingType InBuildingType, 
	ABuilding * InBuildingsProducer, AResourceSpot * InResourceSpot, 
	const FMacroCommandReason & InCommandReason)
{
	PendingBuilding = InBuildingType;
	bIsForDepot = true;
	bUsingConstructionYard = true;
	ResourceSpot = InResourceSpot;
	ProducerBuilding = InBuildingsProducer;
	ProducerInfantry = nullptr;
	CommandReason = InCommandReason;

	NumResourceDepotSpotsTried = 0;

	/* Start at a random rotation */
	ResourceDepotBuildLocsIndex = FMath::RandRange(0, ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY - 1);
}

void FTryingToPlaceBuildingInfo::BeginForResourceDepot(EBuildingType InBuildingType, 
	AInfantry * InBuildingsProducer, AResourceSpot * InResourceSpot, 
	const FMacroCommandReason & InCommandReason)
{
	PendingBuilding = InBuildingType;
	bIsForDepot = true;
	bUsingConstructionYard = false;
	ResourceSpot = InResourceSpot;
	ProducerBuilding = nullptr;
	ProducerInfantry = InBuildingsProducer;
	CommandReason = InCommandReason;

	NumResourceDepotSpotsTried = 0;

	/* Start at a random rotation */
	ResourceDepotBuildLocsIndex = FMath::RandRange(0, ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY - 1);
}

void FTryingToPlaceBuildingInfo::BeginForBuilding(ACPUPlayerAIController * AICon, 
	EBuildingType InBuildingType, ABuilding * InBuildingsProducer, const FMacroCommandReason & InCommandReason)
{
	PendingBuilding = InBuildingType;
	bIsForDepot = false;
	bUsingConstructionYard = true;
	ProducerBuilding = InBuildingsProducer;
	ProducerInfantry = nullptr;
	CommandReason = InCommandReason;

	NumGeneralAreasExhausted = 0;
	NumLocationsTriedForCurrentGeneralArea = 0;

	/* Generate a general area */
	GeneralArea = AICon->GenerateGeneralArea(InBuildingType);

	/* Start at random index */
	BuildLocMultipliersIndex = FMath::RandRange(0, ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM - 1);
}

void FTryingToPlaceBuildingInfo::BeginForBuilding(ACPUPlayerAIController * AICon, 
	EBuildingType InBuildingType, AInfantry * InBuildingsProducer, const FMacroCommandReason & InCommandReason)
{
	PendingBuilding = InBuildingType;
	bIsForDepot = false;
	bUsingConstructionYard = false;
	ProducerBuilding = nullptr;
	ProducerInfantry = InBuildingsProducer;
	CommandReason = InCommandReason;

	NumGeneralAreasExhausted = 0;
	NumLocationsTriedForCurrentGeneralArea = 0;

	/* Generate a general area */
	GeneralArea = AICon->GenerateGeneralArea(InBuildingType);

	/* Start at random index */
	BuildLocMultipliersIndex = FMath::RandRange(0, ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM - 1);
}

bool FTryingToPlaceBuildingInfo::IsPending() const
{
	return PendingBuilding != EBuildingType::NotBuilding;
}

EBuildingType FTryingToPlaceBuildingInfo::GetBuilding() const
{
	return PendingBuilding;
}

int32 FTryingToPlaceBuildingInfo::GetResourceDepotBuildLocsIndex() const
{
	return ResourceDepotBuildLocsIndex;
}

int32 FTryingToPlaceBuildingInfo::GetBuildLocMultipliersIndex() const
{
	return BuildLocMultipliersIndex;
}

int32 FTryingToPlaceBuildingInfo::GetNumGeneralAreasExhausted() const
{
	return NumGeneralAreasExhausted;
}

FVector FTryingToPlaceBuildingInfo::GetCurrentGeneralArea() const
{
	return GeneralArea;
}

FRotator FTryingToPlaceBuildingInfo::GetRotation(ACPUPlayerAIController * AICon) const
{
	return FRotator(0.f, AICon->StartingViewYaw, 0.f);
}

bool FTryingToPlaceBuildingInfo::TryPlaceForTick(ACPUPlayerAIController * AICon)
{
	// Resource, prereq and valid builder assumptions are made

	/* 4 while loops are very similar, either for resource depot or not, and either for 
	construction yard builder or worker builder */

	NumBuildLocationsTriedThisTick = 0;

	const FBuildingInfo * BuildingInfo = AICon->GetFI()->GetBuildingInfo(PendingBuilding);

	if (bIsForDepot)
	{
		if (bUsingConstructionYard)
		{
			while (NumBuildLocationsTriedThisTick < ACPUPlayerAIController::MAX_NUM_BUILD_LOC_TRIES_PER_TICK)
			{
				NumBuildLocationsTriedThisTick++;
				NumResourceDepotSpotsTried++;

				if (AICon->TryPlaceResourceDepot(*BuildingInfo, ResourceSpot.Get(), ProducerBuilding.Get(), 
					CommandReason))
				{
					// Successfully gave out order to place building. Flag no longer trying 
					PendingBuilding = EBuildingType::NotBuilding;
					
					return true;
				}
				else
				{
					/* Check if have tried every spot */
					if (NumResourceDepotSpotsTried == ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY)
					{
						AICon->GiveUpPlacingBuilding();
						return false;
					}
					
					// Advance index for next loop iteration
					ResourceDepotBuildLocsIndex++;
					ResourceDepotBuildLocsIndex %= ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY;
				}
			}

			return false;
		}
		else
		{
			while (NumBuildLocationsTriedThisTick < ACPUPlayerAIController::MAX_NUM_BUILD_LOC_TRIES_PER_TICK)
			{	
				NumBuildLocationsTriedThisTick++;
				NumResourceDepotSpotsTried++;

				if (AICon->TryPlaceResourceDepot(*BuildingInfo, ResourceSpot.Get(), ProducerInfantry.Get(), 
					CommandReason))
				{	
					// Successfully gave out order to place building. Flag no longer trying
					PendingBuilding = EBuildingType::NotBuilding;
					
					return true;
				}
				else
				{
					/* Check if have tried every spot */
					if (NumResourceDepotSpotsTried == ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY)
					{	
						AICon->GiveUpPlacingBuilding();
						return false;
					}

					// Advance index for next loop iteration
					ResourceDepotBuildLocsIndex++; 
					ResourceDepotBuildLocsIndex %= ACPUPlayerAIController::NUM_POTENTIAL_DEPOT_SPOTS_TO_TRY;
				}
			}

			return false;
		}
	}
	else
	{
		if (bUsingConstructionYard)
		{
			while (NumBuildLocationsTriedThisTick < ACPUPlayerAIController::MAX_NUM_BUILD_LOC_TRIES_PER_TICK)
			{
				NumBuildLocationsTriedThisTick++;
				NumLocationsTriedForCurrentGeneralArea++;

				if (AICon->TryPlaceBuilding(*BuildingInfo, ProducerBuilding.Get(), CommandReason))
				{
					// Successfully gave out command to place building. Flag no longer trying
					PendingBuilding = EBuildingType::NotBuilding;
					
					return true;
				}
				else
				{
					// Check if should give up. TryPlaceBuilding is in charge of updating this
					if (NumGeneralAreasExhausted == ACPUPlayerAIController::NUM_GENERAL_AREAS_TO_TRY)
					{
						AICon->GiveUpPlacingBuilding();
						return false;
					}

					// Check if need new general area
					if (NumLocationsTriedForCurrentGeneralArea == ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM)
					{
						GeneralArea = AICon->GenerateGeneralArea(PendingBuilding);

						// Start at random array index
						BuildLocMultipliersIndex = FMath::RandRange(0, ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM - 1);
					}
					else
					{
						// Advance array index
						BuildLocMultipliersIndex++;
						BuildLocMultipliersIndex %= ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM;
					}
				}
			}

			return false;
		}
		else
		{
			while (NumBuildLocationsTriedThisTick < ACPUPlayerAIController::MAX_NUM_BUILD_LOC_TRIES_PER_TICK)
			{
				NumBuildLocationsTriedThisTick++;
				NumLocationsTriedForCurrentGeneralArea++;

				if (AICon->TryPlaceBuilding(*BuildingInfo, ProducerInfantry.Get(), CommandReason))
				{
					// Successfully gave out command to place building. Flag no longer trying
					PendingBuilding = EBuildingType::NotBuilding;
					
					return true;
				}
				else
				{
					// Check if should give up. TryPlaceBuilding is in charge of updating this
					if (NumGeneralAreasExhausted == ACPUPlayerAIController::NUM_GENERAL_AREAS_TO_TRY)
					{
						AICon->GiveUpPlacingBuilding();
						return false;
					}
					
					// Check if need new general area
					if (NumLocationsTriedForCurrentGeneralArea == ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM)
					{
						GeneralArea = AICon->GenerateGeneralArea(PendingBuilding);

						// Start at random array index
						BuildLocMultipliersIndex = FMath::RandRange(0, ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM - 1);
					}
					else
					{
						// Advance array index
						BuildLocMultipliersIndex++;
						BuildLocMultipliersIndex %= ACPUPlayerAIController::BUILD_LOC_MULTIPLERS_NUM;
					}
				}
			}

			return false;
		}
	}
}

bool FTryingToPlaceBuildingInfo::IsProducerUsable() const
{	
	return (ProducerBuilding.IsValid() && !Statics::HasZeroHealth(ProducerBuilding.Get()))
		||
		(ProducerInfantry.IsValid() && !Statics::HasZeroHealth(ProducerInfantry.Get()));
}

EBuildingType FTryingToPlaceBuildingInfo::GiveUp(ACPUPlayerAIController * AICon)
{
	const FBuildingInfo * BuildingInfo = AICon->GetFI()->GetBuildingInfo(PendingBuilding);
	
	if (BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsInTab)
	{

	}
	else
	{

	}

	const EBuildingType ToReturn = PendingBuilding;

	// Flag that we are no longer trying to place a building
	PendingBuilding = EBuildingType::NotBuilding;

	return ToReturn;
}


//=============================================================================================
//	Structs for building buildings using the BuildsInTab build method
//=============================================================================================

//=============================================================================================
//	Single entry struct
//=============================================================================================

FSingleBuildsInTabInfo::FSingleBuildsInTabInfo()
{
	// Use a ctor with params instead
	// Commented because this causes crashes after migrating to 4.21 apparently
	//assert(0);
}

FSingleBuildsInTabInfo::FSingleBuildsInTabInfo(EBuildingType BuildingToBuild, ABuilding * ProducerBuilding,
	AResourceSpot * ResourceSpot, const FProductionQueue * InQueue)
{
	assert(BuildingToBuild != EBuildingType::NotBuilding);

	Building = BuildingToBuild;
	bForResourceDepot = true;
	ResourceSpotForDepot = ResourceSpot;
	ConstructionYard = ProducerBuilding;
	bProductionComplete = false;

#if !UE_BUILD_SHIPPING
	Queue = InQueue;
#endif
}

FSingleBuildsInTabInfo::FSingleBuildsInTabInfo(EBuildingType BuildingToBuild, 
	ABuilding * ProducerBuilding, FMacroCommandReason InCommandReason, const FProductionQueue * InQueue)
{
	assert(BuildingToBuild != EBuildingType::NotBuilding);

	Building = BuildingToBuild;
	bForResourceDepot = false;
	ConstructionYard = ProducerBuilding;
	bProductionComplete = false;
	CommandReason = InCommandReason;

#if !UE_BUILD_SHIPPING
	Queue = InQueue;
#endif
}

TWeakObjectPtr < ABuilding > FSingleBuildsInTabInfo::GetConstructionYard() const
{
	return ConstructionYard;
}

#if !UE_BUILD_SHIPPING
const FProductionQueue * FSingleBuildsInTabInfo::GetQueue() const
{
	return Queue;
}
#endif

void FSingleBuildsInTabInfo::FlagProductionAsComplete()
{
	bProductionComplete = true;
}

void FSingleBuildsInTabInfo::FlagProductionAsCancelled()
{
	Building = EBuildingType::NotBuilding;
}

bool FSingleBuildsInTabInfo::IsProductionComplete() const
{
	return bProductionComplete;
}

void FSingleBuildsInTabInfo::NowTryPlaceEachTick(ACPUPlayerAIController * AICon,
	FTryingToPlaceBuildingInfo & TryPlaceBuildingInfo)
{
	assert(AICon->CanUseConstructionYard(ConstructionYard.Get()));

	/* Make sure not already trying to place another building */
	assert(!TryPlaceBuildingInfo.IsPending());

	if (bForResourceDepot)
	{
		TryPlaceBuildingInfo.BeginForResourceDepot(Building, ConstructionYard.Get(), 
			ResourceSpotForDepot.Get(), CommandReason);
	}
	else
	{
		TryPlaceBuildingInfo.BeginForBuilding(AICon, Building, ConstructionYard.Get(), CommandReason);
	}

	/* Flag we are no longer building a building. This is ok as long as when trying to place
	building we always try build nothing that tick */
	Building = EBuildingType::NotBuilding;
}


//==============================================================================================
//	Struct with the array of FSingleBuildsInTabInfo
//==============================================================================================

void FBuildsInTabBuildingInfo::NoteAsProducing(EBuildingType BuildingToBuild, 
	ABuilding * ProducerBuilding, AResourceSpot * ResourceSpot, const FProductionQueue * InQueue)
{
	Array.Emplace(FSingleBuildsInTabInfo(BuildingToBuild, ProducerBuilding, ResourceSpot, InQueue));
}

void FBuildsInTabBuildingInfo::NoteAsProducing(EBuildingType BuildingToBuild, 
	ABuilding * ProducerBuilding, FMacroCommandReason CommandReason, const FProductionQueue * InQueue)
{
	Array.Emplace(FSingleBuildsInTabInfo(BuildingToBuild, ProducerBuilding, CommandReason, InQueue));
}

bool FBuildsInTabBuildingInfo::IsBuildingSomething() const
{
	return Array.Num() > 0;
}

void FBuildsInTabBuildingInfo::FlagProductionAsComplete(ABuilding * ConstructionYard, 
	const FProductionQueue * InQueue)
{
	for (auto & Elem : Array)
	{
		if (Elem.GetConstructionYard() == ConstructionYard)
		{
			/* Just checking the right queue is being used */
			assert(Elem.GetQueue() == InQueue);

			Elem.FlagProductionAsComplete();

			return;
		}
	}
}

bool FBuildsInTabBuildingInfo::NowTryPlaceEachTick(ACPUPlayerAIController * AICon, 
	FTryingToPlaceBuildingInfo & TryingToPlaceBuildingInfo)
{
	for (int32 i = Array.Num() - 1; i >= 0; --i)
	{
		FSingleBuildsInTabInfo & Elem = Array[i];

		if (Elem.IsProductionComplete())
		{
			Elem.NowTryPlaceEachTick(AICon, TryingToPlaceBuildingInfo);
			Array.RemoveAt(i, 1, false);
			return true;
		}
	}
	
	return false;
}


//==============================================================================================
//	AI Controller
//==============================================================================================

ACPUPlayerAIController::ACPUPlayerAIController()
{
	/* Never ticks on its own: tick manager handles ticking */
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bWantsPlayerState = true;

	/* Fill some containers */

	CommandReasons_BuildResourceDepot.Reserve(Statics::NUM_RESOURCE_TYPES);
	CommandReasons_TrainCollector.Reserve(Statics::NUM_RESOURCE_TYPES);
	NumAquiredResourceSpots.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		CommandReasons_BuildResourceDepot.Emplace(Statics::ArrayIndexToResourceType(i), 0);
		CommandReasons_TrainCollector.Emplace(Statics::ArrayIndexToResourceType(i), 0);
		NumAquiredResourceSpots.Emplace(Statics::ArrayIndexToResourceType(i), 0);
	}

	for (uint8 i = 0; i < ACPUPlayerAIController::GetNumMacroCommandReasons(); ++i)
	{
		CommandReasons_PendingCommands.Emplace(ACPUPlayerAIController::ArrayIndexToMacroCommandReason(i), 0);
	}
}

void ACPUPlayerAIController::Setup(uint8 InIDAsInt, FName InPlayerID, ETeam InTeam, EFaction InFaction, 
	int16 InStartingSpot, const TArray <int32> & StartingResources, ECPUDifficulty InDifficulty)
{
	SERVER_CHECK;

	Difficulty = InDifficulty;

	PS = CastChecked<ARTSPlayerState>(PlayerState);
	GS = CastChecked<ARTSGameState>(GetWorld()->GetGameState());
	GI = CastChecked<URTSGameInstance>(GetWorld()->GetGameInstance());
	FI = GI->GetFactionInfo(InFaction);
	PS->SetGS(GS);
	PS->SetGI(GI);
	PS->SetAIController(this);

	GS->AddCPUController(this);
	
	/* This starting spot is not final - it can still change. Therefore we should not call 
	this until it has been made final (but we do anyway and correct for the starting location 
	later) */
	PS->Server_SetInitialValues(InIDAsInt, InPlayerID, InTeam, InFaction, InStartingSpot, StartingResources);
}

void ACPUPlayerAIController::SetInitialLocAndRot(int16 InStartingSpotID)
{
	// Store our starting location and starting yaw
	const FMapInfo & MapInfo = GI->GetMapInfo(*Statics::GetMapName(GetWorld()));
	FRotator InitialRotation;
	MapInfo.GetPlayerStartTransform(InStartingSpotID, StartingSpotLocation, InitialRotation);
	StartingViewYaw = InitialRotation.Yaw;
}

void ACPUPlayerAIController::NotifyOfStartingSelectables(const TArray<EBuildingType>& StartingBuildings, 
	const TArray<EUnitType>& StartingUnits)
{
	for (const auto & BuildingType : StartingBuildings)
	{
		/* Note: a single building can count as multiple types */
		
		const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

		if (BuildingInfo->IsAConstructionYardType())
		{ 
			//NumConstructionYards++;
		}

		//if (BuildingInfo->IsAResourceDepot())
		//{
		//	// Do something? Could run logic to check if any resource spot has been 'aquired' 
		//  // but would require the building's location as a param
		//}

		if (BuildingInfo->IsABarracksType())
		{
			NumBarracks++;
		}

		if (BuildingInfo->IsABaseDefenseType())
		{
			NumBaseDefenses++;
		}
	}

	for (const auto & UnitType : StartingUnits)
	{
		/* Note: unit can be more than one type */
		
		const FUnitInfo * UnitInfo = FI->GetUnitInfo(UnitType);

		if (UnitInfo->IsACollectorType())
		{
			// Don't think need to do anything
		}

		if (UnitInfo->IsAWorkerType())
		{
			NumWorkers++;
		}

		if (UnitInfo->IsAArmyUnitType())
		{
			// TODO remove hardcode
			ArmyStrength += 10;
		}
	}
}

void ACPUPlayerAIController::PerformFinalSetup()
{
	/* Add entry to container for each resource type */
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);
		
		for (const auto & Spot : GS->GetResourceSpots(ResourceType))
		{
			NumCollectors.Emplace(Spot, FUint8Set());
		}
	}
}

#if ENABLE_VISUAL_LOG
void ACPUPlayerAIController::GrabDebugSnapshot(FVisualLogEntry * Snapshot) const
{
	/* Super is mainly pawn and path following, things this controller does not use, so leave
	it out */
	//Super::GrabDebugSnapshot(Snapshot);

	FVisualLogStatusCategory MyCategory;
	MyCategory.Category = TEXT("RTS CPU Player AI Controller");

	if (PS != nullptr)
	{
		/* Create string for resource amounts */
		FString ResourcesString;
		for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
		{
			const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);

			ResourcesString += ENUM_TO_STRING(EResourceType, ResourceType) + ": ";
			ResourcesString += FString::FromInt(PS->GetNumResources(i)) + "\n";
		}

		MyCategory.Add(TEXT("Resources"), ResourcesString);
	}

	/* Create massive string for state */
	FString StateString;
	StateString += "NumWorkers: " + FString::FromInt(NumWorkers) + "\n";
	StateString += "NumBarracks: " + FString::FromInt(NumBarracks) + "\n";
	StateString += "NumBaseDefenses: " + FString::FromInt(NumBaseDefenses) + "\n";

	MyCategory.Add(TEXT("State"), StateString);

	/* Create a massive string for all the pending commands we've got going */
	FString PendingCommandReasonsString;
	for (const auto & Elem : CommandReasons_BuildResourceDepot)
	{
		PendingCommandReasonsString += "Build " + ENUM_TO_STRING(EResourceType, Elem.Key) + " Depot: " + FString::FromInt(Elem.Value) + "\n";
	}
	for (const auto & Elem : CommandReasons_TrainCollector)
	{
		PendingCommandReasonsString += "Train " + ENUM_TO_STRING(EResourceType, Elem.Key) + " Collector: " + FString::FromInt(Elem.Value) + "\n";
	}
	PendingCommandReasonsString += "\n";
	for (const auto & Elem : CommandReasons_PendingCommands)
	{
		PendingCommandReasonsString += ENUM_TO_STRING(EMacroCommandType, Elem.Key) + ": " + FString::FromInt(Elem.Value) + "\n";
	}

	MyCategory.Add(TEXT("Pending Commands"), PendingCommandReasonsString);

	Snapshot->Status.Add(MyCategory);
}
#endif // ENABLE_VISUAL_LOG

void ACPUPlayerAIController::TickFromManager()
{
	PreBehaviorPass();

	/* Tell any idle harvesters to go collect resources */
	AssignIdleCollectors();

	if (IsTryingToPlaceBuilding())
	{
		if (TryingToPlaceBuildingInfo.IsProducerUsable()
			&& PS->ArePrerequisitesMet(TryingToPlaceBuildingInfo.GetBuilding(), false)
			&& PS->HasEnoughResources(TryingToPlaceBuildingInfo.GetBuilding(), false))
		{
			// Continue trying to place this building
			TryingToPlaceBuildingInfo.TryPlaceForTick(this);

			/* This return means while trying to place building no more behaviour will run.
			This could mean if no spot can ever be found then AI could potentially always try
			and place a building and do nothing else, but that is a whole nother issue.
			Issues can happen when the available areas to build on become congested. AI will
			need to make a history of failed attempts to place buildings and take that into
			account when deciding whether to try and place a building */
			return;
		}
		else
		{
			GiveUpPlacingBuilding();
		}
	}

	DoBehavior();
}

void ACPUPlayerAIController::PreBehaviorPass()
{
	IdleCollectors.Reset();
	IdleWorkers.Reset();
	
	/* Populate the arrays "IdleCollectors" and "IdleWorkers" */

	/* Would like to try and iterate over possibly a worker or collector array but for now 
	doing all units */
	for (const auto & Unit : PS->GetUnits())
	{
		/* No validity check because we are on server and as long as units remove themselves 
		from this array before calling Destroy this *should* be ok */
		
		if (IsUnitIdle(Unit))
		{
			/* Note: a unit can be placed in both arrays if it is both a collector and a worker */
			
			if (Unit->IsACollector())
			{
				IdleCollectors.Emplace(Unit);
			}
			
			if (Unit->CanBuildBuildings())
			{	
				IdleWorkers.Emplace(Unit);
			}
		}
	}
}

void ACPUPlayerAIController::DoBehavior()
{
	/* The 'Decide' functions return true if they issue a command */
	
	/* Decide if should build a resource collector */
	if (DecideIfShouldBuildCollector())
	{	
		return;
	}
	
	/* Decide if should build a resource depot */
	if (DecideIfShouldBuildResourceDepot())
	{
		return;
	}

	/* Check if building something using the BuildsInTab build method. */
	if (BuildsInTabBuildingInfo.IsBuildingSomething())
	{
		/* This will transfer a completed BuildsInTab building over to try be placed each tick */
		if (BuildsInTabBuildingInfo.NowTryPlaceEachTick(this, TryingToPlaceBuildingInfo))
		{
			return;
		}
	}

	/* Check if we are waiting for a queue to become un-full so we can queue something up in it */
	if (IsWaitingForQueueToBecomeUnFull())
	{
		if (QueueWereWaitingToFinish.IsQueueOwnerValidAndAlive())
		{
			// Check if queue now has free slot
			if (QueueWereWaitingToFinish.IsQueueNowUnFull())
			{
				// Got to check prereqs
				EBuildingType MissingPrereq_Building;
				EUpgradeType MissingPrereq_Upgrade;
				if (QueueWereWaitingToFinish.ArePrerequisitesMetForIt(this, MissingPrereq_Building, MissingPrereq_Upgrade))
				{
					// Got to check resources
					if (QueueWereWaitingToFinish.HasEnoughResourcesForIt(this))
					{
						QueueWereWaitingToFinish.BuyIt();
					}
					else
					{
						// Do not have enough resources. Abandon trying to build it
						QueueWereWaitingToFinish.CancelWaiting();
					}
				}
				else
				{
					// Do not have prerequisite for it. Just abandon trying to build it
					QueueWereWaitingToFinish.CancelWaiting();
				}
			}

			/* Either way if we're waiting for a queue to become un-full then do not do anything
			else this tick. I guess the times we called CancelWaiting() above we could avoid 
			returning but who cares. */
			return;
		}
		else
		{
			QueueWereWaitingToFinish.CancelWaiting();
		}
	}

	/* Check if we are saving up for anything and try buy it */
	if (IsSavingUp())
	{
		/* Check if can afford it now */
		if (ThingSavingUpFor.HasEnoughResourcesForIt(this))
		{
			/* Got to check prereqs */
			EBuildingType MissingPrereq_Building;
			EUpgradeType MissingPrereq_Upgrade;
			if (ThingSavingUpFor.ArePrerequisitesMetForIt(this, MissingPrereq_Building, MissingPrereq_Upgrade))
			{
				const bool bTryCorrectForNullBuilder = false;
				
				/* Check we have a selectable that can produce it */
				if (ThingSavingUpFor.HasBuilderForIt(this, bTryCorrectForNullBuilder))
				{
					ThingSavingUpFor.BuildIt(this);
				}
			}
			else
			{
				if (MissingPrereq_Building != EBuildingType::NotBuilding)
				{
					OnSavingsTargetPrereqNotMet(MissingPrereq_Building);
				}
				else
				{
					OnSavingsTargetPrereqNotMet(MissingPrereq_Upgrade);
				}
			}
		}

		/* Either way if we're saving up for something then do not try and make anymore 
		commands this tick */
		return;
	}

	/* Decide if should build a worker or construction yard */
	if (DecideIfShouldBuildInfastructureProducingThing())
	{
		return;
	}

	/* Decide if should build an army producing building */
	if (DecideIfShouldBuildBarracks())
	{
		return;
	}

	if (DecideIfShouldResearchUpgrade())
	{
		return;
	}

	if (DecideIfShouldBuildBaseDefense())
	{
		return;
	}

	if (DecideIfShouldBuildArmyUnit())
	{	
		return;
	}

	/* If here then we didn't take any action */

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Did not issue macro command this tick "));
}

bool ACPUPlayerAIController::DecideIfShouldBuildCollector()
{
	/* This first check is a common theme  in any of the DecideIf... funcs that build selectables. 
	Not a very costly check but consider doing just one check inside DoBehavior() if possible */
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}

	// Note: TSet iteration here
	for (const auto & SpotWeakPtr : AquiredResourceSpots)
	{	
		AResourceSpot * ResourceSpot = SpotWeakPtr.Get();
		
		const EResourceType ResourceType = ResourceSpot->GetType();

		// Need to implement GetNumCollectorsAssigned. Also I do not currently update the container it queries
		const int32 Difference = GetOptimalNumCollectors(ResourceSpot) - GetNumCollectorsAssigned(ResourceSpot) - CommandReasons_TrainCollector[ResourceType];

		if (Difference > 0)
		{	
			TArray < EUnitType > MissingPrereq;
			TArray < EUnitType > NoBuildingCapableOfProducing;

			/* For each collector type for this resource type this faction has... */
			for (const auto & CollectorType : FI->GetCollectorTypes(ResourceType))
			{	
				if (PS->IsAtUniqueUnitCap(CollectorType, true, false))
				{
					continue;
				}
				
				/* Check if its prereqs are met */
				if (PS->ArePrerequisitesMet(CollectorType, false))
				{
					/* Pick one of the buildings that can build it. */
					ABuilding * Barracks = nullptr;
					for (const auto & Building : PS->GetProductionCapableBuildings(FContextButton(CollectorType)))
					{
						/* Validity check should not be needed given the building removes 
						itself from arrays the moment it hits zero health */
						
						/* Just using first usable building, may want to change this to choose 
						fastest queue in future */
						if (CanTrainFromBuilding(Building, CollectorType))
						{
							Barracks = Building;
							break;
						}
					}

					if (Barracks != nullptr)
					{
						/* Check if we can afford it */
						if (PS->HasEnoughResources(CollectorType, false))
						{
							// Build collector
							ActuallyIssueTrainUnitCommand(CollectorType, Barracks, 
								FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, CollectorType, 
									EMacroCommandType::TrainCollector, ResourceType));
							return true;
						}
						else
						{
							// Cannot afford. Start saving up for it
							SetSavingsTarget(CollectorType, Barracks, 
								FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, CollectorType, 
									EMacroCommandType::TrainCollector, ResourceType));
							return true;
						}
					}
					else
					{
						/* No built building can build what we want */
						NoBuildingCapableOfProducing.Emplace(CollectorType);
					}
				}
				else
				{
					MissingPrereq.Emplace(CollectorType);
				}
			}
				
			/* If there then we need to train a collector for this resource type but cannot 
			either because prereq or no built building can train one. So work towards 
			that */	

			if (NoBuildingCapableOfProducing.Num() > 0)
			{
				return InitialRecursiveTryBuildWrapper(NoBuildingCapableOfProducing[0], 
					FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, NoBuildingCapableOfProducing[0], 
						EMacroCommandType::TrainCollector, ResourceType));
			}
			
			if (MissingPrereq.Num() > 0)
			{
				return InitialRecursiveTryBuildWrapper(MissingPrereq[0], 
					FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, MissingPrereq[0], 
						EMacroCommandType::TrainCollector, ResourceType));
			}
				
			/* If here it means faction does not have any collector types for the resource spots 
			type. Confirm this with assert */
			assert(FI->HasCollectorType(ResourceSpot->GetType()) == false);
		}
	}

	return false;
}

bool ACPUPlayerAIController::DecideIfShouldBuildResourceDepot()
{
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	
	// For each resource type...
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType ResourceType = Statics::ArrayIndexToResourceType(i);

		const int32 ExpectedNumDepots = GetExpectedNumDepots(ResourceType);
		const int32 NumDepots = GetNumAquiredResourceSpots(ResourceType);
		const int32 NumPendingDepotBuildCommands = CommandReasons_BuildResourceDepot[ResourceType];

		const int32 Difference = ExpectedNumDepots - NumDepots - NumPendingDepotBuildCommands;

		if (Difference > 0)
		{
			AResourceSpot * BestResourceSpot = nullptr;
			float BestDistanceSqr = FLT_MAX;
			/* Need to choose resource spot to build depot at. Want to try and choose one
			that is 'unclaimed' */
			for (const auto & ResourceSpot : GS->GetResourceSpots(ResourceType))
			{
				/* Choose the closest 'unclaimed' resource spot to our starting spot. */
				if (!AquiredResourceSpots.Contains(ResourceSpot))
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(StartingSpotLocation, ResourceSpot->GetActorLocation());
					if (DistanceSqr < BestDistanceSqr)
					{
						BestResourceSpot = ResourceSpot;
						BestDistanceSqr = DistanceSqr;
					}
				}
			}

			if (BestResourceSpot != nullptr)
			{
				/* Should build a depot for this resource type. Need to choose which building
				though. Here we just use the first one found in iteration that we can afford
				which will usually be the same but should add some randomness to it */
				EBuildingType DepotToBuild = EBuildingType::NotBuilding;
				TArray < EBuildingType > PrereqsNotMet;
				TArray < EBuildingType > CannotAfford;
				for (const auto & DepotType : FI->GetAllDepotsForResource(ResourceType))
				{
					if (PS->IsAtUniqueBuildingCap(DepotType, true, false))
					{
						continue;
					}
					
					if (PS->HasEnoughResources(DepotType, false))
					{
						if (PS->ArePrerequisitesMet(DepotType, false))
						{
							DepotToBuild = DepotType;
							break;
						}
						else
						{
							PrereqsNotMet.Emplace(DepotType);
						}
					}
					else
					{
						if (PS->ArePrerequisitesMet(DepotType, false))
						{
							CannotAfford.Emplace(DepotType);
						}
					}
				}

				if (DepotToBuild != EBuildingType::NotBuilding)
				{
					/* Need to find a constructionYard/worker that can build it */
					
					const FBuildingInfo * DepotInfo = FI->GetBuildingInfo(DepotToBuild);
					
					// Check if can build from construction yard
					if (DepotInfo->CanBeBuiltByConstructionYard())
					{
						for (const auto & ConYard : PS->GetPersistentQueueSupportingBuildings())
						{
							// Use first one but should try for shortest queue time
							if (CanUseConstructionYard(ConYard))
							{	
								TryIssueBuildResourceDepotCommand(DepotToBuild, ConYard, BestResourceSpot, 
									FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, 
										DepotToBuild, EMacroCommandType::BuildResourceDepot, ResourceType));
								return true;
							}
						}
					}
					
					/* Check if any idle worker can build one. Because resource depots are important 
					should probably even consider non-idle workers */
					if (DepotInfo->CanBeBuiltByWorkers())
					{	
						for (const auto & Worker : IdleWorkers)
						{	
							if (CanUseIdleWorker(Worker.Get(), DepotToBuild))
							{
								TryIssueBuildResourceDepotCommand(DepotToBuild, Worker.Get(), 
									BestResourceSpot, 
									FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, 
										DepotToBuild, EMacroCommandType::BuildResourceDepot, ResourceType));
								return true;
							}
						}
					}
				}
				else
				{
					/* Cannot build a depot for this resource type. Check if can build any
					of the prereqs for it so can work towards it */
					/* These two loops may look like we're building a lot of things but we're 
					actually just building the first in the array */

					for (const auto & Elem : PrereqsNotMet)
					{
						/* Doubling up on the uniqueness checks here because it will be done again
						in RecursiveTryBuild but we really want to just skip over those we can't
						build */
						if (PS->IsAtUniqueBuildingCap(Elem, true, false) == false)
						{
							return InitialRecursiveTryBuildWrapper(Elem,
								FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, Elem,
									EMacroCommandType::BuildResourceDepot, ResourceType));
						}
					}

					for (const auto & Elem : CannotAfford)
					{
						/* Doubling up on the uniqueness checks here because it will be done again 
						in RecursiveTryBuild but we really want to just skip over those we can't 
						build */
						if (PS->IsAtUniqueBuildingCap(Elem, true, false) == false)
						{
							/* Should really do SetSavingsTarget here instead */
							return InitialRecursiveTryBuildWrapper(Elem,
								FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, Elem,
									EMacroCommandType::BuildResourceDepot, ResourceType));
						}
					}
				}
			}
			else
			{
				// All spots on map are claimed
			}
		}
	}

	return false;
}

bool ACPUPlayerAIController::DecideIfShouldBuildInfastructureProducingThing()
{	
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	
	// Check if should build a construction yard type building
	{
		const int32 NumConstructionYards = GetNumConstructionYards();
		const int32 ExpectedNumConstructionYards = GetExpectedNumConstructionYards();
		const int32 PendingBuildConstructionYardOrders = CommandReasons_PendingCommands[EMacroCommandType::BuildConstructionYard];
		const int32 Difference = ExpectedNumConstructionYards - NumConstructionYards - PendingBuildConstructionYardOrders;

		if (Difference > 0)
		{
			// Try build the first construction yard we can. Note: will likely be the same building every 
			// time. Should add some randomness to it instead
			for (const auto & ConstructionYardType : FI->GetPersistentQueueTypes())
			{
				if (PS->IsAtUniqueBuildingCap(ConstructionYardType, true, false) == false)
				{
					return InitialRecursiveTryBuildWrapper(ConstructionYardType,
						FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, ConstructionYardType,
							EMacroCommandType::BuildConstructionYard, ConstructionYardType));
				}
			}
			
			/* If here then faction does not have any construction yard type buildings on its roster */
		}
	}

	// Check if should build a worker type unit
	{
		const int32 CurrentNumWorkers = GetNumWorkers();
		const int32 ExpectedNumWorkers = GetExpectedNumWorkers();
		const int32 NumPendingBuildWorkerCommands = CommandReasons_PendingCommands[EMacroCommandType::TrainWorker];
		const int32 Difference = ExpectedNumWorkers - CurrentNumWorkers - NumPendingBuildWorkerCommands;

		if (Difference > 0)
		{
			/* Same as construction yard way done above. Could perhaps be better to choose a 
			worker we haven't already built */
			for (const auto & WorkerType : FI->GetWorkerTypes())
			{
				if (PS->IsAtUniqueUnitCap(WorkerType, true, false) == false)
				{
					return InitialRecursiveTryBuildWrapper(WorkerType,
						FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, WorkerType,
							EMacroCommandType::TrainWorker));
				}
			}
			
			/* If here then faction has no worker type units */
		}
	}
	
	return false;
}

bool ACPUPlayerAIController::DecideIfShouldBuildBarracks()
{
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	
	const int32 CurrentNumBarracks = GetNumBarracks();
	const int32 ExpectedNumBarracks = GetExpectedNumBarracks();
	const int32 NumBuildBarracksCommandsPending = CommandReasons_PendingCommands[EMacroCommandType::BuildBarracks];
	const int32 Difference = ExpectedNumBarracks - CurrentNumBarracks - NumBuildBarracksCommandsPending;

	if (Difference > 0)
	{
		/* Checks that need to happen: 
		- prereqs 
		- resources 
		- if a building/unit can build it */
		
		/* This will just build the same barracks every time given resources are ok. 
		Need to mix it up a bit */
		for (const auto & BuildingType : FI->GetBarracksBuildingTypes())
		{	
			if (PS->IsAtUniqueBuildingCap(BuildingType, true, false))
			{
				continue;
			}
			
			if (PS->HasEnoughResources(BuildingType, false))
			{
				if (PS->ArePrerequisitesMet(BuildingType, false))
				{
					bool bHasSelectableThatCanBuildIt = false;

					const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

					// If statement checks for fast path
					if (BuildingInfo->CanBeBuiltByConstructionYard())
					{
						ABuilding * BestBuilding = nullptr;
						// Note: iterating TSet here
						for (const auto & Building : PS->GetPersistentQueueSupportingBuildings())
						{
							// Looking for empty queue here
							if (Building->GetPersistentProductionQueue()->AICon_HasRoom())
							{
								BestBuilding = Building;
								break;
							}
						}

						if (BestBuilding != nullptr)
						{
							TryIssueBuildBuildingCommand(BuildingType, BestBuilding, 
								FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, 
									BuildingType, EMacroCommandType::BuildBarracks, BuildingType));
							return true;
						}
					}

					// If statement checks for fast path
					if (BuildingInfo->CanBeBuiltByWorkers())
					{
						for (const auto & IdleWorker : IdleWorkers)
						{
							if (IdleWorker->GetContextMenu()->HasButton(FContextButton(BuildingType)))
							{
								TryIssueBuildBuildingCommand(BuildingType, IdleWorker.Get(), 
									FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, 
										BuildingType, EMacroCommandType::BuildBarracks, BuildingType));
								return true;
							}
						}
					}

					/* If here then prereqs are met and we can afford building, but we don't
					have a building/unit built that can build the building. So choose something
					that can build this and build it */

					// Check if faction has any construction yards and build one if so
					for (const auto & ConYardType : BuildingInfo->GetTechTreeParentBuildings())
					{
						if (PS->IsAtUniqueBuildingCap(ConYardType, true, false) == false)
						{
							return InitialRecursiveTryBuildWrapper(ConYardType,
								FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding,
									ConYardType, EMacroCommandType::BuildBarracks, BuildingType));
						}
					}

					// Check if any workers can build this
					for (const auto & WorkerType : BuildingInfo->GetTechTreeParentUnits())
					{
						if (PS->IsAtUniqueUnitCap(WorkerType, true, false) == false)
						{
							return InitialRecursiveTryBuildWrapper(WorkerType,
								FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit,
									WorkerType, EMacroCommandType::BuildBarracks, BuildingType));
						}
					}

					/* If here then nothing in this faction can build this building. Could be that 
					it is a building that the player only ever starts the match with */
				}
			}
		}
	}

	return false;
}

bool ACPUPlayerAIController::DecideIfShouldResearchUpgrade()
{
	/* Check fast path: whether faction even has any upgrades */
	if (FI->HasUpgrades())
	{
		const AUpgradeManager * UpgradeManager = PS->GetUpgradeManager();
		
		/* Check if all upgrades have already been researched */
		if (UpgradeManager->HasResearchedAllUpgrades() == false)
		{
			const int32 NumUnits = PS->GetUnits().Num();

			/* Base it on how many units we have. Need a better metric */
			const bool bResearchUpgrade = (NumUnits == 10);

			if (bResearchUpgrade)
			{
				/* Choose one randomly */
				const EUpgradeType UpgradeType = UpgradeManager->Random_GetUnresearchedUpgrade();

				return InitialRecursiveTryBuildWrapper(UpgradeType, 
					FMacroCommandReason(EMacroCommandSecondaryType::ResearchingUpgrade, UpgradeType, 
						EMacroCommandType::ResearchUpgrade));
			}
		}
	}
	
	return false;
}

bool ACPUPlayerAIController::DecideIfShouldBuildBaseDefense()
{
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	
	/* Check fast path: if faction even has any base defense buildings on its roster */
	if (FI->HasBaseDefenseTypeBuildings())
	{
		const int32 CurrentNumBaseDefenses = GetNumBaseDefenses();
		const int32 ExpectedNumBaseDefenses = GetExpectedNumBaseDefenses();
		const int32 ExpectedNumFromCommands = CommandReasons_PendingCommands[EMacroCommandType::BuildBaseDefense];
		const int32 Difference = ExpectedNumBaseDefenses - CurrentNumBaseDefenses - ExpectedNumFromCommands;

		if (Difference > 0)
		{
			for (const auto & BaseDefenseType : FI->GetBaseDefenseTypes())
			{
				if (PS->IsAtUniqueBuildingCap(BaseDefenseType, true, false) == false)
				{
					return InitialRecursiveTryBuildWrapper(BaseDefenseType,
						FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, BaseDefenseType,
							EMacroCommandType::BuildBaseDefense, BaseDefenseType));
				}
			}
		}
	}

	return false;
}

bool ACPUPlayerAIController::DecideIfShouldBuildArmyUnit()
{
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	
	/* Check fast path: whether faction even has any army units */
	if (FI->HasAttackingUnitTypes())
	{
		const int32 CurrentArmyStrength = GetArmyStrength();
		const int32 ExpectedArmyStrength = GetExpectedArmyStrength();
		const int32 ExpectedStrengthFromBuildCommands = PendingCommands_ArmyStrength;
		const int32 Difference = ExpectedArmyStrength - CurrentArmyStrength - ExpectedStrengthFromBuildCommands;

		if (Difference > 0)
		{
			const int32 NumUnits = PS->GetUnits().Num();

			const bool bTrainUnit = (NumUnits < 0);

			if (bTrainUnit)
			{
				// Will likely result in training the same unit every time
				for (const auto & UnitType : FI->GetAttackingUnitTypes())
				{
					if (PS->IsAtUniqueUnitCap(UnitType, true, false) == false)
					{
						return InitialRecursiveTryBuildWrapper(UnitType,
							FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, UnitType,
								EMacroCommandType::TrainArmyUnit));
					}
				}
			}
		}
	}

	return false;
}

void ACPUPlayerAIController::AssignIdleCollectors()
{		
	for (const auto & UnitWeakPtr : IdleCollectors)
	{
		AInfantry * Unit = UnitWeakPtr.Get();
		
		/* Just using the first resource type they can harvest here. Would be better to choose 
		the one that is most needed and/or that is close. Note: iterating TMap here */
		for (const auto & Elem : Unit->GetResourceGatheringProperties())
		{
			const EResourceType ResourceType = Elem.Key;
			
			/* Choose the closest resource spot to unit */
			float BestDistanceSqr = FLT_MAX;
			AResourceSpot * BestSpot = nullptr;
			for (const auto & ResourceSpot : GS->GetResourceSpots(ResourceType))
			{
				if (!ResourceSpot->IsDepleted())
				{
					const float DistanceSqr = Statics::GetDistance2DSquared(ResourceSpot->GetActorLocation(), Unit->GetActorLocation());

					if (DistanceSqr < BestDistanceSqr)
					{
						BestSpot = ResourceSpot;
						BestDistanceSqr = DistanceSqr;
					}
				}
			}

			if (BestSpot != nullptr)
			{
				ActuallyIssueCollectResourcesCommand(Unit, BestSpot);
				
				// Move on to next idle collector
				break;
			}
			else
			{
				// Basically means there's no undepleted resource spots on map
			}
		}
	}
}

void ACPUPlayerAIController::TryIssueBuildResourceDepotCommand(EBuildingType BuildingType, 
	ABuilding * ConstructionYard, AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason)
{
	/* This first check doesn't have to be true but it is consistent with the behavior I want
	which is to not try and do anything else other than try place building if a building is
	pending to be placed */
	assert(TryingToPlaceBuildingInfo.IsPending() == false);
	assert(PreBuildBuildingCommandChecks(BuildingType, ConstructionYard));

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

	if (BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsInTab)
	{
		BuildsInTabBuildingInfo.NoteAsProducing(BuildingType, ConstructionYard, ResourceSpot, 
			ConstructionYard->GetPersistentProductionQueue());

		// Start production
		ConstructionYard->AICon_QueueProduction(FCPUPlayerTrainingInfo(BuildingType, CommandReason));
	}
	else // Assuming BuildItself, but other methods *may* be ok, so could try removing assert
	{
		assert(BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsItself);
		
		/* Build methods that require the building to be placed somewhere in the world before 
		beginning production */
		
		TryingToPlaceBuildingInfo.BeginForResourceDepot(BuildingType, ConstructionYard, ResourceSpot, CommandReason);
	}

	IncrementNumPendingCommands(CommandReason);
}

void ACPUPlayerAIController::TryIssueBuildResourceDepotCommand(EBuildingType BuildingType, 
	AInfantry * Worker, AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason)
{
	assert(TryingToPlaceBuildingInfo.IsPending() == false);
	assert(PreBuildBuildingCommandChecks(BuildingType, Worker));

	TryingToPlaceBuildingInfo.BeginForResourceDepot(BuildingType, Worker, ResourceSpot, CommandReason);

	IncrementNumPendingCommands(CommandReason);
}

void ACPUPlayerAIController::TryIssueBuildBuildingCommand(EBuildingType BuildingType, 
	ABuilding * ConstructionYard, const FMacroCommandReason & CommandReason)
{
	assert(TryingToPlaceBuildingInfo.IsPending() == false);
	assert(PreBuildBuildingCommandChecks(BuildingType, ConstructionYard));

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

	if (BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsInTab)
	{
		BuildsInTabBuildingInfo.NoteAsProducing(BuildingType, ConstructionYard, 
			CommandReason, ConstructionYard->GetPersistentProductionQueue());

		// Start production
		ConstructionYard->AICon_QueueProduction(FCPUPlayerTrainingInfo(BuildingType, CommandReason));
	}
	else // Assuming BuildItself
	{
		assert(BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsItself);
		
		TryingToPlaceBuildingInfo.BeginForBuilding(this, BuildingType, ConstructionYard, CommandReason);
	}

	IncrementNumPendingCommands(CommandReason);
}

void ACPUPlayerAIController::TryIssueBuildBuildingCommand(EBuildingType BuildingType, AInfantry * Worker, 
	const FMacroCommandReason & CommandReason)
{
	assert(TryingToPlaceBuildingInfo.IsPending() == false);
	assert(PreBuildBuildingCommandChecks(BuildingType, Worker));

	TryingToPlaceBuildingInfo.BeginForBuilding(this, BuildingType, Worker, CommandReason);

	IncrementNumPendingCommands(CommandReason);
}

#if !UE_BUILD_SHIPPING
bool ACPUPlayerAIController::PreBuildBuildingCommandChecks(EBuildingType BuildingType,
	ABuilding * ConstructionYard) const
{
	/* It is assumed that when issuing an order the player has checked that all this stuff 
	is good */
	
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	if (PS->IsAtUniqueBuildingCap(BuildingType, true, false) == true)
	{
		return false;
	}
	if (PS->ArePrerequisitesMet(BuildingType, false) == false)
	{
		return false;
	}
	if (PS->HasEnoughResources(BuildingType, false) == false)
	{
		return false;
	}
	if (Statics::IsValid(ConstructionYard) == false)
	{
		return false;
	}
	if (CanUseConstructionYard(ConstructionYard) == false)
	{
		return false;
	}
	
	return true;
}

bool ACPUPlayerAIController::PreBuildBuildingCommandChecks(EBuildingType BuildingType, 
	AInfantry * Worker) const
{
	if (PS->IsAtSelectableCap(true, false) == true)
	{
		return false;
	}
	if (PS->IsAtUniqueBuildingCap(BuildingType, true, false) == true)
	{
		return false;
	}
	if (PS->ArePrerequisitesMet(BuildingType, false) == false)
	{
		return false;
	}
	if (PS->HasEnoughResources(BuildingType, false) == false)
	{
		return false;
	}
	if (Statics::IsValid(Worker) == false)
	{
		return false;
	}
	if (CanUseWorker(Worker, BuildingType) == false)
	{
		return false;
	}

	return true;
}

bool ACPUPlayerAIController::PreTrainUnitCommandChecks(EUnitType UnitType, ABuilding * Barracks) const
{
	if (PS->ArePrerequisitesMet(UnitType, false) == false)
	{
		return false;
	}

	if (PS->HasEnoughResources(UnitType, false) == false)
	{
		return false;
	}

	if (Statics::IsValid(Barracks) == false)
	{
		return false;
	}

	if (Statics::HasZeroHealth(Barracks) == true)
	{
		return false;
	}

	if (Barracks->GetContextMenu()->HasButton(FContextButton(UnitType)) == false)
	{
		return false;
	}

	if (Barracks->GetContextProductionQueue().AICon_HasRoom() == false)
	{
		return false;
	}

	return true;
}

bool ACPUPlayerAIController::PreResearchUpgradeCommandChecks(EUpgradeType UpgradeType, ABuilding * Researcher) const
{
	if (PS->ArePrerequisitesMet(UpgradeType, false) == false)
	{
		return false;
	}
	
	if (PS->HasEnoughResources(UpgradeType, false) == false)
	{
		return false;
	}
	
	if (Statics::IsValid(Researcher) == false)
	{
		return false;
	}
	
	if (Statics::HasZeroHealth(Researcher) == true)
	{
		return false;
	}

	AUpgradeManager * UpgradeManager = PS->GetUpgradeManager();
	
	if (UpgradeManager->HasBeenFullyResearched(UpgradeType, false) == true)
	{
		return false; 
	}
	
	if (UpgradeManager->IsUpgradeQueued(UpgradeType, false) == true)
	{
		return false;
	}

	return true;
}
#endif // !UE_BUILD_SHIPPING

void ACPUPlayerAIController::ActuallyIssueBuildResourceDepotCommand(EBuildingType BuildingType, 
	ABuilding * ConstructionYard, const FVector & Location, const FRotator & Rotation, 
	const FMacroCommandReason & CommandReason)
{
	assert(CommandReason.GetActualCommand() == EMacroCommandSecondaryType::BuildingBuilding);
	assert(PreBuildBuildingCommandChecks(BuildingType, ConstructionYard));

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

	if (BuildingInfo->GetBuildingBuildMethod() == EBuildingBuildMethod::BuildsInTab)
	{
		// Perhaps don't need to do anything. Maybe increment pending commands
	}
	else
	{
		/* Spawn the building */
		
		ABuilding * WhatWeAreBuilding = Statics::SpawnBuilding(BuildingInfo, Location, Rotation, 
			PS, GS, GetWorld(), ESelectableCreationMethod::Production);
		
		// Add to production queue
		ConstructionYard->AICon_QueueProduction(FCPUPlayerTrainingInfo(BuildingType, CommandReason), 
			WhatWeAreBuilding);

		// Record that we now have a command being carried out
		//IncrementNumPendingCommands(CommandReason);
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: build resource depot. Building type: %s "
		"construction yard: %s, location: %s"), 
		TO_STRING(EBuildingType, BuildingType), *ConstructionYard->GetName(),
		*Location.ToString());
}

void ACPUPlayerAIController::ActuallyIssueBuildResourceDepotCommand(EBuildingType BuildingType, 
	AInfantry * Worker, const FVector & Location, const FRotator & Rotation, 
	const FMacroCommandReason & CommandReason)
{
	assert(PreBuildBuildingCommandChecks(BuildingType, Worker));

	/* Copying the behavior in PC which is to deduct resources even if the build method is one 
	that doesn't place the building until the worker gets there, but will want to change this */

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
	
	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
	if (BuildMethod == EBuildingBuildMethod::LayFoundationsInstantly)
	{
		/* Deduct resource cost */
		PS->SpendResources(BuildingInfo->GetCosts());
	
		ABuilding * Building = Statics::SpawnBuilding(BuildingInfo, Location, Rotation, PS, GS, 
			GetWorld(), ESelectableCreationMethod::Production);

		/* Order worker to go over and start producing it. Here we are relying on all the 
		building's setup having completed like who owns it etc.
		Hmmm appears pathfinding fails when calling this. Actually just need to make sure 
		building's foundation radius is large enough. Also OnRightClickCommand could probably 
		be used instead */
		Worker->OnContextMenuPlaceBuildingResult(Building, *BuildingInfo, BuildMethod);
	}
	else // Assumed LayFoundationsWhenAtLocation or Protoss
	{
		assert(BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
			|| BuildMethod == EBuildingBuildMethod::Protoss);

		// Will need to do something here
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: build resource depot. Building type: %s " 
		"worker: %s, location: %s"), 
		TO_STRING(EBuildingType, BuildingType), *Worker->GetName(), *Location.ToString());
}

void ACPUPlayerAIController::ActuallyIssueBuildBuildingCommand(EBuildingType BuildingType, 
	ABuilding * ConstructionYard, const FVector & Location, const FRotator & Rotation, 
	const FMacroCommandReason & CommandReason)
{
	assert(PreBuildBuildingCommandChecks(BuildingType, ConstructionYard));
	
	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);
	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

	if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
	{
		// Perhaps don't need to do anything
	}
	else
	{
		assert(BuildMethod == EBuildingBuildMethod::BuildsItself);
		
		/* Spawn the building */

		ABuilding * WhatWeAreBuilding = Statics::SpawnBuilding(BuildingInfo, Location, Rotation,
			PS, GS, GetWorld(), ESelectableCreationMethod::Production);

		// Add item to production queue
		ConstructionYard->AICon_QueueProduction(FCPUPlayerTrainingInfo(BuildingType, CommandReason), 
			WhatWeAreBuilding);

		/* Increment num pending commands. If we assume that this function is never called 
		directly without first queuing that we want to try and find a suitable location then 
		this can be left out since it would have already been incremented then */
		//IncrementNumPendingCommands(CommandReason);
	}

	// Make sure to deduct resources if required

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: build building. Building type: %s, "
		"construction yard: %s, location: %s"), 
		TO_STRING(EBuildingType, BuildingType), 
		*ConstructionYard->GetName(), *Location.ToString());
}

void ACPUPlayerAIController::ActuallyIssueBuildBuildingCommand(EBuildingType BuildingType,
	AInfantry * Worker, const FVector & Location, const FRotator & Rotation, 
	const FMacroCommandReason & CommandReason)
{
	assert(PreBuildBuildingCommandChecks(BuildingType, Worker));

	const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

	const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();
	if (BuildMethod == EBuildingBuildMethod::LayFoundationsInstantly)
	{
		/* Deduct resource cost */
		PS->SpendResources(BuildingInfo->GetCosts());

		// Spawn building

		ABuilding * Building = Statics::SpawnBuilding(BuildingInfo, Location, Rotation, PS, GS, 
			GetWorld(), ESelectableCreationMethod::Production);

		/* Order worker to go over and start producing it */
		Worker->OnContextMenuPlaceBuildingResult(Building, *BuildingInfo, BuildMethod);
	}
	else
	{
		assert(BuildMethod == EBuildingBuildMethod::LayFoundationsWhenAtLocation
			|| BuildMethod == EBuildingBuildMethod::Protoss);

		// Will need to do something here
	}

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: build building. Building type: %s, "
		"worker: %s, location: %s"),
		TO_STRING(EBuildingType, BuildingType), *Worker->GetName(), 
		*Location.ToString());
}

void ACPUPlayerAIController::ActuallyIssueTrainUnitCommand(EUnitType UnitType, ABuilding * Barracks, 
	const FMacroCommandReason & CommandReason)
{
	assert(PreTrainUnitCommandChecks(UnitType, Barracks));
	
	Barracks->AICon_QueueProduction(FCPUPlayerTrainingInfo(UnitType, CommandReason));

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Issued a train unit command. Barracks: %s, unit being trained: %s"),
		*Barracks->GetName(), TO_STRING(EUnitType, UnitType));
}

void ACPUPlayerAIController::ActuallyIssueResearchUpgradeCommand(EUpgradeType UpgradeType, 
	ABuilding * ResearchingBuilding, const FMacroCommandReason & CommandReason)
{
	assert(PreResearchUpgradeCommandChecks(UpgradeType, ResearchingBuilding));

	/* Put upgrade in context queue. Make sure this func deducts resources */ 
	ResearchingBuilding->AICon_QueueProduction(FCPUPlayerTrainingInfo(UpgradeType, CommandReason));

	/* Note down we are now researching this upgrade */
	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: research upgrade. Barracks: %s, upgrade: %s"),
		*ResearchingBuilding->GetName(), TO_STRING(EUpgradeType, UpgradeType));
}

void ACPUPlayerAIController::ActuallyIssueCollectResourcesCommand(AInfantry * Collector, AResourceSpot * ResourceSpot)
{
	/* Note down this resource spot now has this collector assigned to it */
	const uint8 SelectableID = Collector->GetSelectableID();
	NumCollectors[ResourceSpot].Emplace(SelectableID);

	// Note: will need to do this for all the other commands if choosing to use it. 
	// Odd doing this causes weird resource gathering behavior try it
	UnitsWithPendingCommands.Emplace(Collector);

	// Tell collector to go gather resources
	Collector->OnRightClickCommand(ResourceSpot, ResourceSpot->GetAttributesBase());

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: collect resources. Collector: %s, resource spot: %s"),
		*Collector->GetName(), *ResourceSpot->GetName());
}

void ACPUPlayerAIController::ActuallyIssueCancelProductionCommand(ABuilding * ConstructionYard)
{
	UE_VLOG(this, RTSLOG, Verbose, TEXT("Command issued: cancel production. Construction yard: %s"),
		*ConstructionYard->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EBuildingType BuildingType, ABuilding * ConstructionYard, 
	AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason)
{
	// If thrown call regular building version instead
	assert(ResourceSpot != nullptr);
	
	ThingSavingUpFor.SetSavingsTarget(BuildingType, ConstructionYard, ResourceSpot);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Builder: %s. Building to serve as "
		"resource depot? Yes"),
		TO_STRING(EBuildingType, BuildingType),
		*ConstructionYard->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EBuildingType BuildingType, AInfantry * Worker, 
	AResourceSpot * ResourceSpot, const FMacroCommandReason & CommandReason)
{
	// If thrown call regualr building version instead
	assert(ResourceSpot != nullptr);
	
	ThingSavingUpFor.SetSavingsTarget(BuildingType, Worker, ResourceSpot);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Builder: %s. Building to serve as "
		"resource depot ? Yes"),
		TO_STRING(EBuildingType, BuildingType), *Worker->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EBuildingType BuildingType, ABuilding * ConstructionYard, 
	FMacroCommandReason CommandReason)
{
	ThingSavingUpFor.SetSavingsTarget(BuildingType, ConstructionYard, CommandReason);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Builder: %s. "), 
		TO_STRING(EBuildingType, BuildingType), *ConstructionYard->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EBuildingType BuildingType, AInfantry * Worker, 
	FMacroCommandReason CommandReason)
{
	ThingSavingUpFor.SetSavingsTarget(BuildingType, Worker, CommandReason);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Builder: %s. "),
		TO_STRING(EBuildingType, BuildingType), *Worker->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EUnitType UnitType, ABuilding * Barracks, 
	FMacroCommandReason CommandReason)
{
	ThingSavingUpFor.SetSavingsTarget(UnitType, Barracks, CommandReason);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Barracks: %s"),
		TO_STRING(EUnitType, UnitType), *Barracks->GetName());
}

void ACPUPlayerAIController::SetSavingsTarget(EUpgradeType UpgradeType, ABuilding * Researcher, 
	FMacroCommandReason CommandReason)
{
	ThingSavingUpFor.SetSavingsTarget(UpgradeType, Researcher, CommandReason);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Savings target set: %s. Researching building: %s "),
		TO_STRING(EUpgradeType, UpgradeType), *Researcher->GetName());
}

void ACPUPlayerAIController::OnSavingsTargetPrereqNotMet(EBuildingType MissingPrereq)
{
	/* Have to decide here whether to now try build the missing prereq or just stop saving */
}

void ACPUPlayerAIController::OnSavingsTargetPrereqNotMet(EUpgradeType MissingPrereq)
{
	/* Have to decide here whether to now try build the missing prereq or just stop saving */
}

void ACPUPlayerAIController::SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EBuildingType WhatWeWantToBuild, FMacroCommandReason CommandReason)
{
	QueueWereWaitingToFinish.WaitForQueue(InQueueOwner, TargetQueue, WhatWeWantToBuild, CommandReason);

	IncrementNumPendingCommands(CommandReason);

	UE_VLOG(this, RTSLOG, Verbose, TEXT("Set queue wait target. Queue owner: %s, item: %s, reason "
		"for production: %s"), *InQueueOwner->GetName(),
		TO_STRING(EBuildingType, WhatWeWantToBuild),
		TO_STRING(EMacroCommandType, CommandReason.GetOriginalCommandMainReason()));
}

void ACPUPlayerAIController::SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EUnitType WhatWeWantToBuild, FMacroCommandReason CommandReason)
{
	QueueWereWaitingToFinish.WaitForQueue(InQueueOwner, TargetQueue, WhatWeWantToBuild, CommandReason);

	IncrementNumPendingCommands(CommandReason);
}

void ACPUPlayerAIController::SetQueueWaitTarget(ABuilding * InQueueOwner, const FProductionQueue * TargetQueue, 
	EUpgradeType WhatWeWantToBuild, FMacroCommandReason CommandReason)
{
	QueueWereWaitingToFinish.WaitForQueue(InQueueOwner, TargetQueue, WhatWeWantToBuild, CommandReason);

	IncrementNumPendingCommands(CommandReason);
}

bool ACPUPlayerAIController::IsUnitIdle(AActor * Unit) const
{
	return !UnitsWithPendingCommands.Contains(Unit);
}

bool ACPUPlayerAIController::CanUseConstructionYard(ABuilding * ConstructionYard) const
{
	assert(ConstructionYard); // Here for debugging
	return !Statics::HasZeroHealth(ConstructionYard) 
		&& ConstructionYard->GetPersistentProductionQueue()->AICon_HasRoom();
}

bool ACPUPlayerAIController::CanTrainFromBuilding(ABuilding * Barracks, EUnitType UnitWeWantToBuild) const
{
	return !Statics::HasZeroHealth(Barracks) && Barracks->GetContextProductionQueue().AICon_HasRoom();
}

bool ACPUPlayerAIController::CanResearchFromBuilding(ABuilding * ResearchLab, EUpgradeType UpgradeType) const
{
	// May need more here
	return !Statics::HasZeroHealth(ResearchLab) && ResearchLab->GetContextProductionQueue().AICon_HasRoom();
}

bool ACPUPlayerAIController::CanUseIdleWorker(AInfantry * Worker, EBuildingType BuildingType) const
{
	return !Statics::HasZeroHealth(Worker) && Worker->GetContextMenu()->HasButton(FContextButton(BuildingType));
}

bool ACPUPlayerAIController::CanUseWorker(AInfantry * Worker, EBuildingType BuildingType) const
{
	return !Statics::HasZeroHealth(Worker) && Worker->GetContextMenu()->HasButton(FContextButton(BuildingType));
}

void ACPUPlayerAIController::GiveUpPlacingBuilding()
{
	const EBuildingType GivenUpOn = TryingToPlaceBuildingInfo.GiveUp(this);

	UE_VLOG(this, RTSLOG, Warning, TEXT("Gave up trying to find spot to place building [%s]"),
		TO_STRING(EBuildingType, GivenUpOn));

	OnGiveUpPlacingBuilding();
}

void ACPUPlayerAIController::OnGiveUpPlacingBuilding()
{
}

bool ACPUPlayerAIController::TryPlaceResourceDepotInner(const FBuildingInfo & DepotInfo,
	AResourceSpot * ResourceSpot, FVector & OutLocation, FRotator & OutRotation)
{	
	/* Choose how far away we want to build the depot based on how big the resource spot is
	and how big the depot is */
	const float MaxDepotBounds = FMath::Max<float>(DepotInfo.GetScaledBoundsExtent().X, DepotInfo.GetScaledBoundsExtent().Y);
	/* Here using foundation radius to add a bit more, but should probably make a seperate 
	variable for this instead */
	const float Distance = ResourceSpot->AICon_GetBaseBuildRadius() + MaxDepotBounds + (DepotInfo.GetFoundationRadius() * 4.f);

	const FVector OffsetVector = FVector(ResourceDepotBuildLocs[TryingToPlaceBuildingInfo.GetResourceDepotBuildLocsIndex()] * Distance, 0.f);

	/* Add the resource spot's location. Loc is a location that lies on a circle (with radius
	Distance) around the resource spot */
	const FVector Loc = ResourceSpot->GetActorLocation() + OffsetVector;

	const FRotator Rot = TryingToPlaceBuildingInfo.GetRotation(this);

	/* Need to trace down against environment to find the ground and pass that into the
	check function. Player controllers who have a ghost out do this every tick so they have it
	sorted already but AI controllers gots to do it now */
	const float TraceHalfHeight = 2048.f;
	const FVector TraceStart = FVector(Loc.X, Loc.Y, Loc.Z + TraceHalfHeight);
	const FVector TraceEnd = FVector(Loc.X, Loc.Y, Loc.Z - TraceHalfHeight);
	FHitResult HitResult;
	const UWorld * World = GetWorld();
	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GROUND_CHANNEL))
	{
		const FVector FinalLoc = FVector(Loc.X, Loc.Y, HitResult.ImpactPoint.Z);

		OutLocation = FinalLoc;
		OutRotation = Rot;

		return Statics::IsBuildableLocation(World, GS, PS, FI, DepotInfo.GetBuildingType(), 
			OutLocation, OutRotation, false);
	}

	return false;
}

bool ACPUPlayerAIController::TryPlaceResourceDepot(const FBuildingInfo & DepotInfo,
	AResourceSpot * ResourceSpot, ABuilding * ConstructionYard, const FMacroCommandReason & CommandReason)
{
	FVector Location;
	FRotator Rotation;
	const bool bSuccess = TryPlaceResourceDepotInner(DepotInfo, ResourceSpot, Location, Rotation);

	if (bSuccess)
	{
		ActuallyIssueBuildResourceDepotCommand(DepotInfo.GetBuildingType(), ConstructionYard,
			Location, Rotation, CommandReason);

		// Count spot as 'aquired' so we don't try place another depot next to it
		AquiredResourceSpots.Emplace(ResourceSpot);
		NumAquiredResourceSpots[CommandReason.OriginalCommand_GetResourceType()] += 1;
	}

	return bSuccess;
}

bool ACPUPlayerAIController::TryPlaceResourceDepot(const FBuildingInfo & DepotInfo,
	AResourceSpot * ResourceSpot, AInfantry * Worker, const FMacroCommandReason & CommandReason)
{
	FVector Location;
	FRotator Rotation;
	const bool bSuccess = TryPlaceResourceDepotInner(DepotInfo, ResourceSpot, Location, Rotation);

	if (bSuccess)
	{
		ActuallyIssueBuildResourceDepotCommand(DepotInfo.GetBuildingType(), Worker, Location, 
			Rotation, CommandReason);

		// Count spot as 'aquired' so we don't try place another depot next to it
		AquiredResourceSpots.Emplace(ResourceSpot);
		NumAquiredResourceSpots[CommandReason.OriginalCommand_GetResourceType()] += 1;
	}

	return bSuccess;
}

bool ACPUPlayerAIController::TryPlaceBuildingInner(const FBuildingInfo & BuildingInfo, 
	FVector & OutLocation, FRotator & OutRotation)
{
	const EBuildingType BuildingType = BuildingInfo.GetBuildingType();
	
	/* Going to assume resources and prereqs have been checked already */
	assert(PS->ArePrerequisitesMet(BuildingType, false) == true);
	assert(PS->HasEnoughResources(BuildingType, false) == true);

	const FVector BuildingBounds = BuildingInfo.GetScaledBoundsExtent();
	const FVector2D ShiftAmount = FVector2D(BuildingBounds.X, BuildingBounds.Y);
	const FVector Location = TryingToPlaceBuildingInfo.GetCurrentGeneralArea() + FVector((BuildLocMultipliers[TryingToPlaceBuildingInfo.GetBuildLocMultipliersIndex()] * ShiftAmount), 0.f);
	const FRotator Rotation = TryingToPlaceBuildingInfo.GetRotation(this);

	/* Need to line trace against environment here to get a Z value that is on the ground */
	const float TraceHalfHeight = 2048.f;
	const FVector TraceStart = FVector(Location.X, Location.Y, Location.Z + TraceHalfHeight);
	const FVector TraceEnd = FVector(Location.X, Location.Y, Location.Z - TraceHalfHeight);
	FHitResult HitResult;
	const UWorld * World = GetWorld();
	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, GROUND_CHANNEL))
	{
		const FVector FinalLoc = FVector(Location.X, Location.Y, HitResult.ImpactPoint.Z);

		OutLocation = FinalLoc;
		OutRotation = Rotation;

		return Statics::IsBuildableLocation(World, GS, PS, FI, BuildingType, OutLocation, 
			OutRotation, false);
	}

	return false;
}

bool ACPUPlayerAIController::TryPlaceBuilding(const FBuildingInfo & BuildingInfo, 
	ABuilding * ConstructionYard, FMacroCommandReason CommandReason)
{
	FVector Location;
	FRotator Rotation;
	const bool bSuccess = TryPlaceBuildingInner(BuildingInfo, Location, Rotation);

	if (bSuccess)
	{
		ActuallyIssueBuildBuildingCommand(BuildingInfo.GetBuildingType(), ConstructionYard, 
			Location, Rotation, CommandReason);
	}
			
	return bSuccess;
}

bool ACPUPlayerAIController::TryPlaceBuilding(const FBuildingInfo & BuildingInfo, 
	AInfantry * Worker, FMacroCommandReason CommandReason)
{
	FVector Location;
	FRotator Rotation;
	const bool bSuccess = TryPlaceBuildingInner(BuildingInfo, Location, Rotation);

	if (bSuccess)
	{
		ActuallyIssueBuildBuildingCommand(BuildingInfo.GetBuildingType(), Worker, Location, 
			Rotation, CommandReason);
	}

	return bSuccess;
}

void ACPUPlayerAIController::Delay(FTimerHandle & TimerHandle, void(ACPUPlayerAIController::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void ACPUPlayerAIController::IncrementNumPendingCommands(const FMacroCommandReason & CommandReason)
{
	const EMacroCommandSecondaryType ActualCommand = CommandReason.GetActualCommand();
	
	// Increment the actual command
	if (ActualCommand == EMacroCommandSecondaryType::BuildingBuilding)
	{
		PendingCommands_BuildBuilding.Contains(CommandReason.GetBuildingType())
			? PendingCommands_BuildBuilding[CommandReason.GetBuildingType()] += 1
			: PendingCommands_BuildBuilding.Emplace(CommandReason.GetBuildingType(), 1);
	}
	else if (ActualCommand == EMacroCommandSecondaryType::TrainingUnit)
	{
		PendingCommands_TrainUnit.Contains(CommandReason.GetUnitType()) 
			? PendingCommands_TrainUnit[CommandReason.GetUnitType()] += 1
			: PendingCommands_TrainUnit.Emplace(CommandReason.GetUnitType(), 1);

		/* Add army strength of unit */
		const FUnitInfo * UnitInfo = FI->GetUnitInfo(CommandReason.GetUnitType());
		PendingCommands_ArmyStrength += 10; // TODO hardcoded, should come from UnitInfo
	}
	else
	{
		assert(ActualCommand == EMacroCommandSecondaryType::ResearchingUpgrade);

		// Would imply already researching upgrade
		assert(PendingCommands_ResearchUpgrade.Contains(CommandReason.GetUpgradeType()) == false);

		PendingCommands_ResearchUpgrade.Emplace(CommandReason.GetUpgradeType());
	}

	const EMacroCommandType MainReason = CommandReason.GetOriginalCommandMainReason();

	// Increment the 'bigger picture' part of the command
	if (MainReason == EMacroCommandType::BuildResourceDepot)
	{
		assert(CommandReasons_BuildResourceDepot[CommandReason.OriginalCommand_GetResourceType()] >= 0);
		
		CommandReasons_BuildResourceDepot[CommandReason.OriginalCommand_GetResourceType()] += 1;
	}
	else if (MainReason == EMacroCommandType::TrainCollector)
	{
		assert(CommandReasons_TrainCollector[CommandReason.OriginalCommand_GetResourceType()] >= 0);
		
		CommandReasons_TrainCollector[CommandReason.OriginalCommand_GetResourceType()] += 1;
	}
	else
	{
		// Cannot have negative num commands pending
		assert(CommandReasons_PendingCommands[MainReason] >= 0);

		CommandReasons_PendingCommands[MainReason] += 1;
	}
}

void ACPUPlayerAIController::DecrementNumPendingCommands(const FMacroCommandReason & CommandReason)
{
	const EMacroCommandSecondaryType ActualCommand = CommandReason.GetActualCommand();
	
	// Decrement the actual command
	if (ActualCommand == EMacroCommandSecondaryType::BuildingBuilding)
	{
		PendingCommands_BuildBuilding[CommandReason.GetBuildingType()] -= 1;
	}
	else if (ActualCommand == EMacroCommandSecondaryType::TrainingUnit)
	{
		PendingCommands_TrainUnit[CommandReason.GetUnitType()] -= 1;

		/* Decrement army strength */
		const FUnitInfo * UnitInfo = FI->GetUnitInfo(CommandReason.GetUnitType());
		PendingCommands_ArmyStrength -= 10; // TODO hardcoded, should come from UnitInfo
	}
	else
	{
		assert(ActualCommand == EMacroCommandSecondaryType::ResearchingUpgrade);

		PendingCommands_ResearchUpgrade.Remove(CommandReason.GetUpgradeType());
	}
	
	const EMacroCommandType MainReason = CommandReason.GetOriginalCommandMainReason();

	// Decrement the 'bigger picture' part of the command
	if (MainReason == EMacroCommandType::BuildResourceDepot)
	{
		CommandReasons_BuildResourceDepot[CommandReason.OriginalCommand_GetResourceType()] -= 1;

		assert(CommandReasons_BuildResourceDepot[CommandReason.OriginalCommand_GetResourceType()] >= 0);
	}
	else if (MainReason == EMacroCommandType::TrainCollector)
	{
		CommandReasons_TrainCollector[CommandReason.OriginalCommand_GetResourceType()] -= 1;

		assert(CommandReasons_TrainCollector[CommandReason.OriginalCommand_GetResourceType()] >= 0);
	}
	else
	{
		CommandReasons_PendingCommands[MainReason] -= 1;

		// Cannot have negative num commands pending
		assert(CommandReasons_PendingCommands[MainReason] >= 0);
	}
}

void ACPUPlayerAIController::IncrementState(EBuildingType BuildingJustCompleted, const FBuildingInfo * BuildingInfo)
{
	if (BuildingInfo->IsABarracksType())
	{
		NumBarracks++;
	}
	
	if (BuildingInfo->IsABaseDefenseType())
	{
		NumBaseDefenses++;
	}
	
	if (BuildingInfo->IsAConstructionYardType())
	{
		// Currently using persistent queues variable in PS for this so no increment needed
	}
}

void ACPUPlayerAIController::IncrementState(EUnitType UnitJustCompleted, const FUnitInfo * UnitInfo)
{
	if (UnitInfo->IsACollectorType())
	{
		// Don't think have a variable to increment here
	}

	if (UnitInfo->IsAWorkerType())
	{
		NumWorkers++;
	}

	if (UnitInfo->IsAArmyUnitType())
	{
		// Increment army strength. Using hardcoded value right now TODO
		ArmyStrength += 10;
	}
}

uint8 ACPUPlayerAIController::GetNumMacroCommandTypes()
{
	return static_cast<uint8>(EMacroCommandSecondaryType::z_ALWAYS_LAST_IN_ENUM) - 1;
}

EMacroCommandSecondaryType ACPUPlayerAIController::ArrayIndexToMacroCommandType(int32 ArrayIndex)
{
	return static_cast<EMacroCommandSecondaryType>(ArrayIndex + 1);
}

uint8 ACPUPlayerAIController::GetNumMacroCommandReasons()
{
	return static_cast<uint8>(EMacroCommandType::z_ALWAYS_LAST_IN_ENUM) - 1;
}

EMacroCommandType ACPUPlayerAIController::ArrayIndexToMacroCommandReason(int32 ArrayIndex)
{
	return static_cast<EMacroCommandType>(ArrayIndex + 1);
}

void ACPUPlayerAIController::OnQueueProductionComplete(ABuilding * Producer, const FProductionQueue * Queue,
	const FCPUPlayerTrainingInfo & ItemProduced, const TArray<FCPUPlayerTrainingInfo>& RemovedButNotBuilt)
{
	/* Handle the item that was produced */
	if (ItemProduced.IsProductionForBuilding())
	{
		const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(ItemProduced.GetBuildingType());

		const EBuildingBuildMethod BuildMethod = BuildingInfo->GetBuildingBuildMethod();

		if (BuildMethod == EBuildingBuildMethod::BuildsInTab)
		{
			// Flag this so player can try place next tick
			BuildsInTabBuildingInfo.FlagProductionAsComplete(Producer, Queue);
		}
		else
		{
			assert(BuildMethod == EBuildingBuildMethod::BuildsItself);

			// Decrement from pending
			DecrementNumPendingCommands(FMacroCommandReason(ItemProduced));

			/* Increment state command for the building that was completed */
			IncrementState(ItemProduced.GetBuildingType(), BuildingInfo);
		}
	}
	else if (ItemProduced.IsForUnit())
	{
		/* Here assuming that the unit was successfully spawned */

		// Decrement from pending
		DecrementNumPendingCommands(FMacroCommandReason(ItemProduced));

		const FUnitInfo * UnitInfo = FI->GetUnitInfo(ItemProduced.GetUnitType());

		IncrementState(ItemProduced.GetUnitType(), UnitInfo);
	}
	else // Assumed for upgrade
	{
		assert(ItemProduced.IsForUpgrade());

		DecrementNumPendingCommands(FMacroCommandReason(ItemProduced));

		/* Do not think there is a research counter here on controller so do not need to 
		increment anything */
	}

	/* Handle the items removed from the queue because they are no longer producable */
	for (const auto & CancelledItem : RemovedButNotBuilt)
	{
		DecrementNumPendingCommands(FMacroCommandReason(CancelledItem));
	}
}

bool ACPUPlayerAIController::IsTryingToPlaceBuilding() const
{
	return TryingToPlaceBuildingInfo.IsPending();
}

bool ACPUPlayerAIController::IsWaitingForQueueToBecomeUnFull() const
{
	return QueueWereWaitingToFinish.IsWaiting();
}

bool ACPUPlayerAIController::IsSavingUp() const
{
	return ThingSavingUpFor.IsSavingUp();
}

int32 ACPUPlayerAIController::GetNumAquiredResourceSpots(EResourceType ResourceType) const
{
	return NumAquiredResourceSpots[ResourceType];
}

int32 ACPUPlayerAIController::GetNumConstructionYards() const
{
	return PS->GetPersistentQueueSupportingBuildings().Num();
}

int32 ACPUPlayerAIController::GetNumWorkers() const
{
	return NumWorkers;
}

int32 ACPUPlayerAIController::GetNumBarracks() const
{
	return NumBarracks;
}

int32 ACPUPlayerAIController::GetNumBaseDefenses() const
{
	return NumBaseDefenses;
}

int32 ACPUPlayerAIController::GetArmyStrength() const
{
	return ArmyStrength;
}

int32 ACPUPlayerAIController::GetExpectedNumDepots(EResourceType ResourceType) const
{
	const float MatchTime = GS->Server_GetMatchTime();
	
	/* Just based on how long match has been going for */
	if (MatchTime < 3.f * MINUTE)
	{
		return 2;
	}
	else if (MatchTime < 10.f * MINUTE)
	{
		return 2;
	}
	else
	{
		return 3;
	}
}

int32 ACPUPlayerAIController::GetExpectedNumConstructionYards() const
{
	const float MatchTime = GS->Server_GetMatchTime();
	
	if (MatchTime < 15.f * MINUTE)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

int32 ACPUPlayerAIController::GetExpectedNumWorkers() const
{
	const float MatchTime = GS->Server_GetMatchTime();
	
	if (MatchTime < 2.f * MINUTE)
	{
		return 2;
	}
	else
	{
		return 3;
	}
}

int32 ACPUPlayerAIController::GetExpectedNumBarracks() const
{
	const float MatchTime = GS->Server_GetMatchTime();
	
	if (MatchTime < 0.03f * MINUTE)
	{
		return 0;
	}
	else if (MatchTime < 2.f * MINUTE)
	{
		return 2;
	}
	else if (MatchTime < 9.f * MINUTE)
	{
		return 2; 
	}
	else 
	{
		return 3;
	}
}

int32 ACPUPlayerAIController::GetExpectedNumBaseDefenses() const
{
	const float MatchTime = GS->Server_GetMatchTime();

	if (MatchTime < 5.f * MINUTE)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int32 ACPUPlayerAIController::GetExpectedArmyStrength() const
{
	const float MatchTime = GS->Server_GetMatchTime();
	
	return 9000;
}

bool ACPUPlayerAIController::InitialRecursiveTryBuildWrapper(EBuildingType BuildingType, 
	FMacroCommandReason CommandReason)
{
	TriedRecursiveBuildBuildings.Reset();
	TriedRecursiveBuildUnits.Reset();
	
	return RecursiveTryBuild(BuildingType, CommandReason);
}

bool ACPUPlayerAIController::InitialRecursiveTryBuildWrapper(EUnitType UnitType, 
	FMacroCommandReason CommandReason)
{
	TriedRecursiveBuildBuildings.Reset();
	TriedRecursiveBuildUnits.Reset();
	
	return RecursiveTryBuild(UnitType, CommandReason);
}

bool ACPUPlayerAIController::InitialRecursiveTryBuildWrapper(EUpgradeType UpgradeType, 
	FMacroCommandReason CommandReason)
{
	TriedRecursiveBuildBuildings.Reset();
	TriedRecursiveBuildUnits.Reset();
	
	return RecursiveTryBuild(UpgradeType, CommandReason);
}

// ------ Building version ------
bool ACPUPlayerAIController::RecursiveTryBuild(EBuildingType BuildingType, FMacroCommandReason CommandReason)
{
	assert(BuildingType != EBuildingType::NotBuilding);

	/* Check for 'need worker to build building but need building to build worker' infinite 
	recursion case */
	if (TriedRecursiveBuildBuildings.Contains(BuildingType))
	{
		return false;
	}
	else
	{
		TriedRecursiveBuildBuildings.Emplace(BuildingType);
	}

	/* Quick return if we aren't allowed to build anymore of this building type */
	if (PS->IsAtUniqueBuildingCap(BuildingType, true, false))
	{
		return false;
	}

	EBuildingType MissingPrereq_Building;
	EUpgradeType MissingPrereq_Upgrade;
	if (PS->ArePrerequisitesMet(BuildingType, false, MissingPrereq_Building, MissingPrereq_Upgrade))
	{
		const bool bHasEnoughResources = PS->HasEnoughResources(BuildingType, false);

		const FBuildingInfo * BuildingInfo = FI->GetBuildingInfo(BuildingType);

		const bool bCanBeBuiltByConstructionYards = BuildingInfo->CanBeBuiltByConstructionYard();
		const bool bCanBeBuiltByWorkers = BuildingInfo->CanBeBuiltByWorkers();

		if (bCanBeBuiltByConstructionYards)
		{
			/* Check all the buildings that can build this */
			for (const auto & OtherBuilding : PS->GetPersistentQueueSupportingBuildings())
			{
				if (OtherBuilding->GetPersistentProductionQueue()->AICon_HasRoom())
				{
					if (bHasEnoughResources)
					{
						TryIssueBuildBuildingCommand(BuildingType, OtherBuilding, CommandReason);
						return true;
					}
					else
					{
						SetSavingsTarget(BuildingType, OtherBuilding, CommandReason);
						return true;
					}
				}
			}
		}
		
		if (bCanBeBuiltByWorkers)
		{
			/* Check all the units that can build this */
			for (const auto & Unit : IdleWorkers)
			{
				if (Unit->GetContextMenu()->HasButton(FContextButton(BuildingType)))
				{
					if (bHasEnoughResources)
					{
						TryIssueBuildBuildingCommand(BuildingType, Unit.Get(), CommandReason);
						return true;
					}
					else
					{
						SetSavingsTarget(BuildingType, Unit.Get(), CommandReason);
						return true;
					}
				}
			}
		}
		
		/* Nothing we currently have built (that is also idle) is capable of producing
		this building. Pick something that can produce it and build that */

		if (bCanBeBuiltByConstructionYards)
		{
			const EBuildingType ConstructionYardType = BuildingInfo->Random_GetTechTreeParentBuilding();

			// Should not need EBuildingType::NotBuilding check since already checked before
			return RecursiveTryBuild(ConstructionYardType, 
				FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, ConstructionYardType, CommandReason));
		}

		if (bCanBeBuiltByWorkers)
		{
			const EUnitType WorkerType = BuildingInfo->Random_GetTechTreeParentUnit();

			// Should not need EUnitType::None check here since already checked
			return RecursiveTryBuild(WorkerType, 
				FMacroCommandReason(EMacroCommandSecondaryType::TrainingUnit, WorkerType, CommandReason));
		}

		/* If here then nothing on this faction can build the building we want to build.
		Perhaps the building is only a starting selectable and cannot be built during a
		match. */
		return false;
	}
	else
	{
		// Try build the missing prerequisite
		if (MissingPrereq_Building != EBuildingType::NotBuilding)
		{
			return RecursiveTryBuild(MissingPrereq_Building,
				FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, BuildingType, CommandReason));
		}
		else
		{
			// Not 100% sure if the FMacroCommandReason ctor params are correct
			return RecursiveTryBuild(MissingPrereq_Upgrade,
				FMacroCommandReason(EMacroCommandSecondaryType::ResearchingUpgrade, BuildingType, CommandReason));
		}
	}
}

// ----- Unit version ------
bool ACPUPlayerAIController::RecursiveTryBuild(EUnitType UnitType, FMacroCommandReason CommandReason)
{
	assert(UnitType != EUnitType::None && UnitType != EUnitType::NotUnit);

	/* Check for 'need worker to build building but need building to build worker' infinite
	recursion case */
	if (TriedRecursiveBuildUnits.Contains(UnitType))
	{
		return false;
	}
	else
	{
		TriedRecursiveBuildUnits.Emplace(UnitType);
	}

	/* Quick return if we aren't allowed to build anymore of this unit type */
	if (PS->IsAtUniqueUnitCap(UnitType, true, false))
	{
		return false;
	}

	/* Check if unit is even trainable. If only available as a starting unit then this would
	return false */
	const FUnitInfo * UnitInfo = FI->GetUnitInfo(UnitType);
	if (UnitInfo->IsTrainable())
	{
		// Check if prerequisites are met
		EBuildingType MissingPrereq_Building;
		EUpgradeType MissingPrereq_Upgrade;
		if (PS->ArePrerequisitesMet(UnitType, false, MissingPrereq_Building, MissingPrereq_Upgrade))
		{
			// Check if we own a building that can build it 
			bool bOwnBarracksForUnit = false;
			ABuilding * Barracks = nullptr;
			for (const auto & Building : PS->GetBuildings())
			{
				if (Building->GetContextMenu()->HasButton(FContextButton(UnitType))
					&& Building->GetContextProductionQueue().AICon_HasRoom())
				{
					bOwnBarracksForUnit = true;
					Barracks = Building;
					break;
				}
			}

			if (bOwnBarracksForUnit)
			{
				// Check if have enough resources 
				if (PS->HasEnoughResources(UnitType, false))
				{
					ActuallyIssueTrainUnitCommand(UnitType, Barracks, CommandReason);
					return true;
				}
				else
				{
					SetSavingsTarget(UnitType, Barracks, CommandReason);
					return true;
				}
			}
			else
			{
				// Build a barracks that can train this unit

				const EBuildingType BarracksType = UnitInfo->Random_GetTechTreeParent();

				/* No need to check for NotBuilding - already know it is trainable */
				return RecursiveTryBuild(BarracksType, 
					FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, BarracksType, 
						CommandReason));
			}
		}
		else
		{
			if (MissingPrereq_Building != EBuildingType::NotBuilding)
			{
				// Build the prerequisite building
				return RecursiveTryBuild(MissingPrereq_Building,
					FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, MissingPrereq_Building,
						CommandReason));
			}
			else
			{
				// Research the prerequisite upgrade. Not 100% sure I have the FMacroCommandReason ctor params correct
				return RecursiveTryBuild(MissingPrereq_Upgrade, 
					FMacroCommandReason(EMacroCommandSecondaryType::ResearchingUpgrade, MissingPrereq_Upgrade, 
						CommandReason));
			}
		}
	}
	else
	{
		/* Faction cannot train this unit */
		return false;
	}
}

// ----- Upgrade version -----
bool ACPUPlayerAIController::RecursiveTryBuild(EUpgradeType UpgradeType, FMacroCommandReason CommandReason)
{
	assert(UpgradeType != EUpgradeType::None);

	EBuildingType MissingPrereq_Building;
	EUpgradeType MissingPrereq_Upgrade;
	if (PS->ArePrerequisitesMet(UpgradeType, false, MissingPrereq_Building, MissingPrereq_Upgrade))
	{
		// Choose building to research upgrade from 
		ABuilding * QueueFullBuilding = nullptr;
		for (const auto & Building : PS->GetBuildings())
		{
			if (Building->GetContextMenu()->HasButton(FContextButton(UpgradeType)))
			{
				if (Building->GetContextProductionQueue().AICon_HasRoom())
				{
					if (PS->HasEnoughResources(UpgradeType, false))
					{
						ActuallyIssueResearchUpgradeCommand(UpgradeType, Building, CommandReason);
						return true;
					}
					else
					{
						SetSavingsTarget(UpgradeType, Building, CommandReason);
						return true;
					}
				}
				else
				{
					QueueFullBuilding = Building;
				}
			}
		}

		if (QueueFullBuilding != nullptr)
		{
			if (PS->HasEnoughResources(UpgradeType, false))
			{
				/* If here then all buildings capable of researching the upgrade have their
				queues full. We will note this down and try to build it as soon as a queue
				frees up */
				SetQueueWaitTarget(QueueFullBuilding, &QueueFullBuilding->GetContextProductionQueue(),
					UpgradeType, CommandReason);
				return true;
			}
			else
			{
				SetSavingsTarget(UpgradeType, QueueFullBuilding, CommandReason);
				return true;
			}
		}

		/* No building we have built can research this so build one that can. Pick one at
		random from tech tree and work towards building that */
		const FUpgradeInfo & UpgradeInfo = FI->GetUpgradeInfoChecked(UpgradeType);
		const EBuildingType CapableOfResearching = UpgradeInfo.Random_GetTechTreeParent();
		if (CapableOfResearching != EBuildingType::NotBuilding)
		{
			return RecursiveTryBuild(CapableOfResearching, 
				FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, 
					CapableOfResearching, CommandReason));
		}
		else
		{
			/* Nothing on this faction can actually research this. Probably best to avoid
			ever passing an upgrade like this into this func in the first place. */
			UE_LOG(RTSLOG, Fatal, TEXT("Trying to research an upgrade that no building can "
				"research. Best to avoid trying to do this earlier. Faction: %s, upgrade: %s"),
				TO_STRING(EFaction, FI->GetFaction()),
				TO_STRING(EUpgradeType, UpgradeType));

			return false;
		}
	}
	else
	{
		if (MissingPrereq_Building != EBuildingType::NotBuilding)
		{
			// Try build the building to fulfill the prereq
			return RecursiveTryBuild(MissingPrereq_Building,
				FMacroCommandReason(EMacroCommandSecondaryType::BuildingBuilding, MissingPrereq_Building, CommandReason));
		}
		else
		{
			// Try research upgrade to fulfill the prereq. Not 100% sure the params in the FMacroCommandReason ctor are correct
			return RecursiveTryBuild(MissingPrereq_Upgrade, 
				FMacroCommandReason(EMacroCommandSecondaryType::ResearchingUpgrade, MissingPrereq_Upgrade, CommandReason));
		}
	}
}

int32 ACPUPlayerAIController::GetOptimalNumCollectors(AResourceSpot * ResourceSpot) const
{
	/* For now this just returns 2 no matter what, but would like to take into account: 
	- distance from spot to depot 
	- the collectors already gathering from it and their gather rates */
	return 1;
}

int32 ACPUPlayerAIController::GetNumCollectorsAssigned(AResourceSpot * ResourceSpot) const
{
	return NumCollectors[ResourceSpot].Num();
}

FVector ACPUPlayerAIController::GenerateGeneralArea(EBuildingType BuildingWeWantToPlace)
{
	/* Generate a new random spot. Do this by picking a building we own and then picking a
	random point around it */

	const FBuildingInfo * BuildInfo = FI->GetBuildingInfo(BuildingWeWantToPlace);
	const FVector Extent = BuildInfo->GetScaledBoundsExtent();
	const float ProximityRadius = BuildInfo->GetBuildProximityRange();

	/* The idea here is to build near our own buildings whether that is a requirement or not. 
	If we have no buildings then default to pretending our starting location is where a building 
	is */

	assert(PS->GetBuildings().Num() >= 0);
	
	FVector CircleCenter;

	if (PS->GetBuildings().Num() == 0)
	{
		CircleCenter = StartingSpotLocation;
	}
	else
	{
		/* The building we choose is dependent on how many general areas we've tried already. 
		The first general area will be based on the last building built. Then the 2nd last and 
		so on. If we request more general areas than we have buildings then the rest will just 
		use the first building in buildings array */
		int32 Index = PS->GetBuildings().Num() - 1 - TryingToPlaceBuildingInfo.GetNumGeneralAreasExhausted();
		if (Index < 0)
		{
			Index = 0;
		}
		
		CircleCenter = PS->GetBuildings()[Index]->GetActorLocation();
	}
	
	const float CircleRadius = ProximityRadius > 0.f ? ProximityRadius : FMath::Max<float>(Extent.X, Extent.Y) * 4.f;
	const FVector RandomPoint = CircleCenter + FVector(FMath::RandPointInCircle(CircleRadius), 0.f);

	return RandomPoint;
}

ECPUDifficulty ACPUPlayerAIController::GetDifficulty() const
{
	return Difficulty;
}

ARTSPlayerState * ACPUPlayerAIController::GetPS() const
{
	return PS;
}

AFactionInfo * ACPUPlayerAIController::GetFI() const
{
	return FI;
}
