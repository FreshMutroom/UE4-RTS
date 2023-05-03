// Fill out your copyright notice in the Description page of Project Settings.

#include "UpgradeManager.h"
#include "Classes/Engine/World.h"
#include "Public/TimerManager.h"
#include "Classes/GameFramework/Character.h"

#include "Statics/DevelopmentStatics.h"
#include "Statics/DamageTypes.h"
#include "GameFramework/FactionInfo.h"
#include "GameFramework/RTSPlayerState.h"
#include "GameFramework/RTSGameInstance.h"
#include "Miscellaneous/UpgradeEffect.h"
#include "MapElements/Infantry.h"
#include "MapElements/Building.h"
#include "Statics/Statics.h"
#include "UI/RTSHUD.h"


AUpgradeManager::AUpgradeManager()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	/* For some strange reason code in here is getting called before statics and if
	referencing GEngine in here it will return nullptr. Therefore have moved
	SetupTimers() and SetupUpgradesCompleted to BeginPlay() */

	bReplicates = false;
	bReplicateMovement = false;
	bAlwaysRelevant = true;	/* Very important for replicating AInfo deriving classes, although 
							we no longer replicate this class so irrelevant */
}

void AUpgradeManager::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AUpgradeManager::SetupReferences()
{
	PS = CastChecked<ARTSPlayerState>(GetOwner());

	/* Problem: Faction info has not been created yet client side */

	URTSGameInstance * GameInstance = GetWorld()->GetGameInstance<URTSGameInstance>();

	/* For testing 2+ players in PIE GI is not initialized by this point, so do it now if it
	hasn't already */
	GameInstance->Initialize();

	FI = GameInstance->GetFactionInfo(PS->GetFaction());
	assert(FI != nullptr);
}

void AUpgradeManager::CreateUpgradeClassesAndPopulateUnresearchedArray()
{
	for (const auto & Elem : FI->GetUpgradesMap())
	{
		if (Elem.Value.GetEffectBP() != nullptr)
		{
			UUpgradeEffect * Effect = NewObject<UUpgradeEffect>(this, Elem.Value.GetEffectBP());
			UpgradeEffects.Emplace(Elem.Key, Effect);
		}
		else
		{
			UpgradeEffects.Emplace(Elem.Key, nullptr);
		}

		/* UnresearchedUpgrades does not contain any upgrades that are aquired by commander 
		abilities so check for that here */
		if (FI->IsUpgradeResearchableThroughBuilding(Elem.Key))
		{
			UnresearchedUpgrades.Emplace(Elem.Key);
		}
	}
}

void AUpgradeManager::SetupUpgradesCompleted()
{
	/* Setup 'times researched' map */
	for (uint8 i = 0; i < Statics::NUM_UPGRADE_TYPES; ++i)
	{
		UpgradesCompleted.Emplace(Statics::ArrayIndexToUpgradeType(i), 0);
	}

	/* Setup each specific selectables upgrades map */
	for (uint8 i = 0; i < Statics::NUM_BUILDING_TYPES; ++i)
	{
		CompletedBuildingUpgrades.Emplace(static_cast<EBuildingType>(i), FUpgradeArray());
	}
	for (uint8 i = 0; i < Statics::NUM_UNIT_TYPES; ++i)
	{
		CompletedUnitUpgrades.Emplace(Statics::ArrayIndexToUnitType(i), FUpgradeArray());
	}
}

void AUpgradeManager::SetupHasUpgradePrerequisiteContainers()
{
	for (const auto & Pair : FI->GetAllBuildingTypes())
	{
		const EBuildingType BuildingType = Pair.Key;
		
		const TArray<EUpgradeType> & UpgradePrereqArray = Pair.Value.GetUpgradePrerequisites();
		if (UpgradePrereqArray.Num() > 0)
		{
			FUpgradeTypeArray & ArrayStruct = PS->BuildingTypeToMissingUpgradePrereqs.Emplace(BuildingType, FUpgradeTypeArray(UpgradePrereqArray.Num()));
			for (const auto & Prereq : UpgradePrereqArray)
			{
				ArrayStruct.Array.Emplace(Prereq);

				if (HasUpgradePrerequisite_Buildings.Contains(Prereq) == false)
				{
					HasUpgradePrerequisite_Buildings.Emplace(Prereq, FUpgradeTypeArrayPtrArray());
				}

				HasUpgradePrerequisite_Buildings[Prereq].Array.Emplace(&ArrayStruct);
			}
		}
	}

	for (const auto & Pair : FI->GetAllUnitTypes())
	{
		const EUnitType UnitType = Pair.Key;

		const TArray<EUpgradeType> & UpgradePrereqArray = Pair.Value.GetUpgradePrerequisites();
		if (UpgradePrereqArray.Num() > 0)
		{
			FUpgradeTypeArray & ArrayStruct = PS->UnitTypeToMissingUpgradePrereqs.Emplace(UnitType, FUpgradeTypeArray(UpgradePrereqArray.Num()));
			for (const auto & Prereq : UpgradePrereqArray)
			{
				ArrayStruct.Array.Emplace(Prereq);

				if (HasUpgradePrerequisite_Units.Contains(Prereq) == false)
				{
					HasUpgradePrerequisite_Units.Emplace(Prereq, FUpgradeTypeArrayPtrArray());
				}

				HasUpgradePrerequisite_Units[Prereq].Array.Emplace(&ArrayStruct);
			}
		}
	}

	for (const auto & Pair : FI->GetUpgradesMap())
	{
		const EUpgradeType UpgradeType = Pair.Key;

		const TArray<EUpgradeType> & UpgradePrereqArray = Pair.Value.GetUpgradePrerequisites();
		if (UpgradePrereqArray.Num() > 0)
		{
			FUpgradeTypeArray & ArrayStruct = PS->UpgradeTypeToMissingUpgradePrereqs.Emplace(UpgradeType, FUpgradeTypeArray(UpgradePrereqArray.Num()));
			for (const auto & Prereq : UpgradePrereqArray)
			{
				ArrayStruct.Array.Emplace(Prereq);

				if (HasUpgradePrerequisite_Upgrades.Contains(Prereq) == false)
				{
					HasUpgradePrerequisite_Upgrades.Emplace(Prereq, FUpgradeTypeArrayPtrArray());
				}

				HasUpgradePrerequisite_Upgrades[Prereq].Array.Emplace(&ArrayStruct);
			}
		}
	}
}

void AUpgradeManager::Init()
{
	assert(!HasInited());
	
	SetupReferences();
	CreateUpgradeClassesAndPopulateUnresearchedArray();
	SetupUpgradesCompleted();
	SetupHasUpgradePrerequisiteContainers();

	bHasInited = true;
}

bool AUpgradeManager::HasInited() const
{
	return bHasInited;
}

void AUpgradeManager::Delay(FTimerHandle & TimerHandle, void(AUpgradeManager::* Function)(), float Delay)
{
	assert(Delay > 0.f);
	GetWorldTimerManager().SetTimer(TimerHandle, this, Function, Delay, false);
}

void AUpgradeManager::OnUpgradePutInProductionQueue(EUpgradeType UpgradeType, AActor * QueueOwner)
{
	assert(Queued.Contains(UpgradeType) == false);

	assert(GetWorld()->IsServer() || PS->GetAffiliation() == EAffiliation::Owned);

	Queued.Emplace(UpgradeType, QueueOwner);
}

void AUpgradeManager::OnUpgradeProductionStarted(EUpgradeType UpgradeType, AActor * ResearchingActor)
{
	assert(GetWorld()->IsServer() || PS->GetAffiliation() == EAffiliation::Owned);

	// Currently a noop - nothing needed to be done
}

void AUpgradeManager::OnUpgradeCancelledFromProductionQueue(EUpgradeType UpgradeType)
{
	assert(GetWorld()->IsServer() || PS->GetAffiliation() == EAffiliation::Owned);

	Queued.FindAndRemoveChecked(UpgradeType);
}

void AUpgradeManager::OnUpgradeComplete(EUpgradeType UpgradeType, EUpgradeAquireMethod HowItWasCompleted)
{
	/* Note to self: remember that this should be called from GS::Multicast_OnUpgradeComplete so 
	replace previous calls to this with calls to the GS version now */
	
	UpgradesCompleted[UpgradeType]++;
	const uint8 UpgradeLevel = UpgradesCompleted[UpgradeType];

	if (HowItWasCompleted == EUpgradeAquireMethod::ResearchedFromBuilding)
	{
		TotalNumUpgradesResearched++;

		const int32 NumRemoved = UnresearchedUpgrades.RemoveSingleSwap(UpgradeType, false);
		assert(NumRemoved == 1);
	}
	
	/* Check building is still standing. Building should have cancelled research
	and removed itself from map when it reached zero health */
	if (GetWorld()->IsServer())
	{
		if (HowItWasCompleted == EUpgradeAquireMethod::ResearchedFromBuilding)
		{
			const AActor * Researcher = Queued[UpgradeType];
			assert(Statics::IsValid(Researcher));
		}

		const bool bUpgradePrereqsNowMetForSomething = OnUpgradeComplete_UpdateSomeContainers(UpgradeType, HowItWasCompleted);

		if (PS->BelongsToLocalPlayer())
		{
			// Tell the HUD
			PS->GetHUDWidget()->OnUpgradeComplete(UpgradeType, bUpgradePrereqsNowMetForSomething);
		}
	}
	else if (PS->BelongsToLocalPlayer())
	{
		const bool bUpgradePrereqsNowMetForSomething = OnUpgradeComplete_UpdateSomeContainers(UpgradeType, HowItWasCompleted);

		// Tell the HUD
		PS->GetHUDWidget()->OnUpgradeComplete(UpgradeType, bUpgradePrereqsNowMetForSomething);
	}

	// TODO: PC->Client_OnUpgradeComplete() (Show message or something)

	const FUpgradeInfo * UpgradeInfo = FI->GetUpgradeInfoNotChecked(UpgradeType);
	
#if !UE_BUILD_SHIPPING
	if (UpgradeInfo == nullptr && HowItWasCompleted == EUpgradeAquireMethod::ResearchedFromBuilding)
	{
		UE_LOG(RTSLOG, Fatal, TEXT("Faction [%s] does not have an entry for upgrade type [%s] "
			"in it's upgrade roster. Please add one"), 
			TO_STRING(EFaction, FI->GetFaction()), TO_STRING(EUpgradeType, UpgradeType));
	}
#endif

	/* Null check. We allow some upgrades obtained through commander abilities to not have 
	entries in upgrade roster if they choose so */
	if (UpgradeInfo != nullptr)
	{
		UUpgradeEffect * Effect = UpgradeEffects[UpgradeType];

		/* We also allow the effect object to be null so check that too. 
		Some upgrades are just used as prerequisites for other things and don't want any effect */
		if (Effect != nullptr)
		{
			// Buildings
			if (UpgradeInfo->AffectsAllBuildings())
			{
				for (const auto & Building : PS->GetBuildings())
				{
					Effect->ApplyEffect_Building(Building, false, UpgradeLevel);
					Building->OnUpgradeComplete(UpgradeType, UpgradeLevel);
				}

				/* Make sure newly spawned buildings get upgraded. Note:: iterating over TMap here */
				for (const auto & Pair : FI->GetAllBuildingTypes())
				{
					const EBuildingType BuildingType = Pair.Key;
					CompletedBuildingUpgrades[BuildingType].AddCompletedUpgrade(UpgradeType);
				}
			}
			else
			{
				for (const auto & Building : PS->GetBuildings())
				{
					if (UpgradeInfo->AffectsBuildingType(Building->GetType()))
					{
						Effect->ApplyEffect_Building(Building, false, UpgradeLevel);
						Building->OnUpgradeComplete(UpgradeType, UpgradeLevel);
					}
				}

				/* Make sure newly spawned buildings get upgraded. N.B iterating over TSet here */
				for (const auto & BuildingType : UpgradeInfo->GetBuildingTypesAffected())
				{
					CompletedBuildingUpgrades[BuildingType].AddCompletedUpgrade(UpgradeType);
				}
			}

			// Units
			if (UpgradeInfo->AffectsAllUnits())
			{
				for (const auto & Infantry : PS->GetUnits())
				{
					Effect->ApplyEffect_Infantry(Infantry, false, UpgradeLevel);
					Infantry->OnUpgradeComplete(UpgradeType, UpgradeLevel);
				}

				/* Make sure newly spawned units get upgrade. Note: iterating over TMap here */
				for (const auto & Pair : FI->GetAllUnitTypes())
				{
					const EUnitType UnitType = Pair.Key;
					CompletedUnitUpgrades[UnitType].AddCompletedUpgrade(UpgradeType);
				}
			}
			else
			{
				for (const auto & Infantry : PS->GetUnits())
				{
					if (UpgradeInfo->AffectsUnitType(Infantry->GetType()))
					{
						Effect->ApplyEffect_Infantry(Infantry, false, UpgradeLevel);
						Infantry->OnUpgradeComplete(UpgradeType, UpgradeLevel);
					}
				}

				/* Make sure newly spawned units get the upgrade */
				for (const auto & UnitType : UpgradeInfo->GetUnitTypesAffected())
				{
					CompletedUnitUpgrades[UnitType].AddCompletedUpgrade(UpgradeType);
				}
			}
		}
	}
}

bool AUpgradeManager::OnUpgradeComplete_UpdateSomeContainers(EUpgradeType UpgradeType, EUpgradeAquireMethod HowItWasCompleted)
{
	if (HowItWasCompleted == EUpgradeAquireMethod::ResearchedFromBuilding)
	{
		Queued.FindAndRemoveChecked(UpgradeType);
	}

	bool bSomethingHasZeroUpgradePrereqsNow = false;

	/* Each of these is O(n^2) although there will likely be very little entries in them */
	
	// Buildings
	if (HasUpgradePrerequisite_Buildings.Contains(UpgradeType))
	{
		const TArray<FUpgradeTypeArray*> & OuterArray = HasUpgradePrerequisite_Buildings[UpgradeType].Array;
		for (int32 i = OuterArray.Num() - 1; i >= 0; --i)
		{
			TArray<EUpgradeType> & InnerArray = OuterArray[i]->Array;
			
			assert(InnerArray.Contains(UpgradeType));
			InnerArray.RemoveSingleSwap(UpgradeType, false);
			bSomethingHasZeroUpgradePrereqsNow |= (InnerArray.Num() == 0);
		}
	}
	
	// Units
	if (HasUpgradePrerequisite_Units.Contains(UpgradeType))
	{
		const TArray<FUpgradeTypeArray*> & OuterArray = HasUpgradePrerequisite_Units[UpgradeType].Array;
		for (int32 i = OuterArray.Num() - 1; i >= 0; --i)
		{
			TArray<EUpgradeType> & InnerArray = OuterArray[i]->Array;

			assert(InnerArray.Contains(UpgradeType));
			InnerArray.RemoveSingleSwap(UpgradeType, false);
			bSomethingHasZeroUpgradePrereqsNow |= (InnerArray.Num() == 0);
		}
	}
	
	// Upgrades
	if (HasUpgradePrerequisite_Upgrades.Contains(UpgradeType))
	{
		const TArray<FUpgradeTypeArray*> & OuterArray = HasUpgradePrerequisite_Upgrades[UpgradeType].Array;
		for (int32 i = OuterArray.Num() - 1; i >= 0; --i)
		{
			TArray<EUpgradeType> & InnerArray = OuterArray[i]->Array;

			assert(InnerArray.Contains(UpgradeType));
			InnerArray.RemoveSingleSwap(UpgradeType, false);
			bSomethingHasZeroUpgradePrereqsNow |= (InnerArray.Num() == 0);
		}
	}

	return bSomethingHasZeroUpgradePrereqsNow;
}

void AUpgradeManager::ApplyAllCompletedUpgrades(ABuilding * Building)
{
	const EBuildingType BuildingType = Building->GetAttributesBase().GetBuildingType();
	const TArray <EUpgradeType> & UpgradesDone = CompletedBuildingUpgrades[BuildingType].GetArray();
	for (const auto & UpgradeType : UpgradesDone)
	{
		UpgradeEffects[UpgradeType]->ApplyEffect_Building(Building, true, UpgradesCompleted[UpgradeType]);
	}
}

void AUpgradeManager::ApplyAllCompletedUpgrades(AInfantry * Infantry)
{
	/* If errors are pointing to this function look at ARangedInfantry::AdjustForUpgrades()
	instead - probably using null upgrade manager reference */

	const EUnitType UnitType = Infantry->GetType();
	const TArray <EUpgradeType> & UpgradesDone = CompletedUnitUpgrades[UnitType].GetArray();
	for (const auto & UpgradeType : UpgradesDone)
	{
		UpgradeEffects[UpgradeType]->ApplyEffect_Infantry(Infantry, true, UpgradesCompleted[UpgradeType]);
	}
}

bool AUpgradeManager::HasBeenFullyResearched(EUpgradeType UpgradeType, bool bShowHUDWarning) const
{
	const bool bFullyResearched = (UpgradesCompleted[UpgradeType] == 1);

	if (bFullyResearched && bShowHUDWarning)
	{
		PS->OnGameWarningHappened(EGameWarning::FullyResearched);
	}

	return bFullyResearched;
}

bool AUpgradeManager::IsUpgradeQueued(EUpgradeType UpgradeType, bool bShowHUDWarning) const
{
	// Since Queued is only updated on server and for owning player
	assert(GetWorld()->IsServer() || PS->GetAffiliation() == EAffiliation::Owned);
	
	/* Using key + value removal on TMap so contains is enough */
	const bool bQueued = Queued.Contains(UpgradeType);

	if (bQueued && bShowHUDWarning)
	{
		PS->OnGameWarningHappened(EGameWarning::AlreadyBeingResearched);
	}

	return bQueued;
}

bool AUpgradeManager::HasResearchedAllUpgrades() const
{
	assert(TotalNumUpgradesResearched <= FI->GetNumUpgrades());
	return TotalNumUpgradesResearched == FI->GetNumUpgrades();
}

EUpgradeType AUpgradeManager::Random_GetUnresearchedUpgrade() const
{
	const int32 RandomIndex = FMath::RandRange(0, UnresearchedUpgrades.Num() - 1);
	return UnresearchedUpgrades[RandomIndex];
}
