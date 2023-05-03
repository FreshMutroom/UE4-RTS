// Fill out your copyright notice in the Description page of Project Settings.

#include "FactionInfo.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"

#include "Statics/Statics.h"
#include "GameFramework/RTSPlayerState.h"
#include "MapElements/Building.h"
#include "MapElements/GhostBuilding.h"
#include "Managers/ObjectPoolingManager.h"
#include "Miscellaneous/UpgradeEffect.h"
#include "Statics/DevelopmentStatics.h"
#include "MapElements/Infantry.h"
#include "GameFramework/RTSGameInstance.h"


//===========================================================================================
//	Structs
//===========================================================================================

void FButtonArray::Sort()
{
	Buttons.Sort();
}

const TArray<FContextButton>& FButtonArray::GetButtons() const
{
	return Buttons;
}

void FButtonArray::AddButton(const FContextButton & InButton)
{
	/* If thrown possibly a building/unit share the same type on faction */
	assert(!Buttons.Contains(InButton));

	Buttons.Emplace(InButton);
}


TSubclassOf <UUpgradeEffect> FRankInfo::GetBonus() const
{
	return Bonus;
}

UParticleSystem * FRankInfo::GetParticles() const
{
	return Particles;
}

USoundBase * FRankInfo::GetSound() const
{
	return Sound;
}

UTexture2D * FRankInfo::GetIcon() const
{
	return Icon;
}


//===========================================================================================
//	AFactionInfo Implementation
//===========================================================================================

AFactionInfo::AFactionInfo()
{
	PoolingManager = nullptr;

	for (uint8 i = LevelingUpOptions::STARTING_LEVEL; i <= LevelingUpOptions::MAX_LEVEL; ++i)
	{
		LevelUpInfo.Emplace(i, FRankInfo());
	}

	RightClickCommandStaticParticles.Reserve(Statics::NUM_COMMAND_TARGET_TYPES);
	for (uint8 i = 0; i < Statics::NUM_COMMAND_TARGET_TYPES; ++i)
	{
		const ECommandTargetType TargetType = Statics::ArrayIndexToCommandTargetType(i);
		RightClickCommandStaticParticles.Emplace(TargetType, FParticleInfo());
	}
	SelectionParticles.Reserve(Statics::NUM_AFFILIATIONS);
	RightClickCommandParticles.Reserve(Statics::NUM_AFFILIATIONS);
	SelectionDecals.Reserve(Statics::NUM_AFFILIATIONS);
	RightClickCommandDecals.Reserve(Statics::NUM_AFFILIATIONS);
	for (uint8 i = 0; i < Statics::NUM_AFFILIATIONS; ++i)
	{
		const EAffiliation Affiliation = Statics::ArrayIndexToAffiliation(i);
		SelectionParticles.Emplace(Affiliation, FVaryingSizeParticleArray());
		RightClickCommandParticles.Emplace(Affiliation, FVaryingSizeParticleArray());
		SelectionDecals.Emplace(Affiliation, FSelectionDecalInfo());
		RightClickCommandDecals.Emplace(Affiliation, FDecalInfo());
	}

	CommanderLevelUpInfo.Reserve(LevelingUpOptions::MAX_COMMANDER_LEVEL);
	for (uint8 i = 1; i <= LevelingUpOptions::MAX_COMMANDER_LEVEL; ++i)
	{
		CommanderLevelUpInfo.Emplace(i, FCommanderLevelUpInfo());
	}

	DefaultBuildingBuildMethod = EBuildingBuildMethod::LayFoundationsInstantly;
	BuildingProximityRange = 0.f;
	bCanBuildOffAllies = true;

	DefaultMouseCursors_CanGatherFromHoveredResourceSpot.Reserve(Statics::NUM_RESOURCE_TYPES);
	DefaultMouseCursors_CannotGatherFromHoveredResourceSpot.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		DefaultMouseCursors_CanGatherFromHoveredResourceSpot.Emplace(Statics::ArrayIndexToResourceType(i), EMouseCursorType::None);
		DefaultMouseCursors_CannotGatherFromHoveredResourceSpot.Emplace(Statics::ArrayIndexToResourceType(i), EMouseCursorType::None);
	}
}

void AFactionInfo::BeginPlay()
{
	Super::BeginPlay();

	Initialize(CastChecked<URTSGameInstance>(GetGameInstance()));
}

void AFactionInfo::Initialize(URTSGameInstance * GameInst)
{
	/* TODO: just realized that all the buildings/units spawned for InitBuildingInfo/UnitUnitInfo
	replicate. As long as this is done before players join lobby it should be alright right? Or
	maybe destroyed actors still send some info over. Forcing garbage collection might help.
	Look into this */

	/* Get reference to the object pooling manager. Could probably pass it up from
	somewhere else instead of finding it like this */
	for (TActorIterator<AObjectPoolingManager> Iter(GetWorld()); Iter; ++Iter)
	{
		PoolingManager = *Iter;
		break;
	}
	assert(PoolingManager != nullptr);

	InitHUDPersistentTabButtons();
	InitUnitInfo();
	InitBuildingInfo();
	CreatePrerequisitesText();
	InitUpgradeInfo();
	SortHUDPersistentTabButtons();
	InitButtonToPersistentTabMappings();
	InitLevelUpBonuses();
	InitMouseCursors(GameInst);
	InitSelectionDecals();
	CreateTechTree();
	InitHUDWarningInfo(GameInst);
}

void AFactionInfo::InitHUDPersistentTabButtons()
{
	/* Fill with empty arrays */
	for (uint8 i = 0; i < Statics::NUM_PERSISTENT_HUD_TAB_TYPES; ++i)
	{
		HUDPersistentTabButtons.Emplace(Statics::ArrayIndexToPersistentTabType(i), FButtonArray());
	}
}

void AFactionInfo::InitBuildingInfo()
{
	/* Do this first */
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceDepotTypes.Emplace(Statics::ArrayIndexToResourceType(i), FBuildingTypeArray());
	}
	
	/* Clear array first */
	EmptyBuildingInfo();

	/* Spawn one of every building and create a FBuildInfo for each one, add
	it to player state array, then destroy building */
	for (const auto & Elem_BP : Buildings)
	{
		ABuilding * Building = Statics::SpawnBuildingForFactionInfo(Elem_BP, FVector::ZeroVector,
			FRotator::ZeroRotator, GetWorld());
		assert(Building != nullptr);

		FBuildingInfo BuildInfo;
		Building->SetupBuildingInfo(BuildInfo, this);
		BuildInfo.SetSelectableBP(Elem_BP);
		const TSubclassOf < AGhostBuilding > & GhostBP = Building->GetGhostBP();
		BuildInfo.SetGhostBuildingBP(GhostBP);

		PoolingManager->AddProjectileBP(Building->GetProjectileBP());

#if WITH_EDITORONLY_DATA
		SelectableBPs.Emplace(Elem_BP);
#endif

		const EBuildingType BuildingType = Building->GetType();

		UE_CLOG(BuildingInfo.Contains(BuildingType), RTSLOG, Fatal, TEXT("For faction \"%s\" two "
			"or more buildings share the same type which is \"%s\". The blueprint for one of "
			"those buildings: %s"), TO_STRING(EFaction, Faction), TO_STRING(EBuildingType, BuildingType),
			*Elem_BP->GetName());

		// Possibly add entry to PersistentQueueTypes (construction yards)
		if (BuildInfo.IsAConstructionYardType())
		{
			PersistentQueueTypes.Emplace(BuildingType);
		}

		// Add entries to ResourceDepotTypes
		for (const auto & ResourceType : Building->GetBuildingAttributes()->GetResourceCollectionTypes())
		{
			ResourceDepotTypes[ResourceType].Emplace(BuildingType);
		}

		/* Possibly add entry to BarracksTypes. */
		if (BuildInfo.IsABarracksType())
		{
			BarracksTypes.Emplace(BuildingType);
		}

		/* Possibly add entry to BaseDefenseTypes */
		if (BuildInfo.IsABaseDefenseType())
		{
			BaseDefenseTypes.Emplace(BuildingType);
		}

#if WITH_EDITOR
		/* Spawn ghost building, check its dimensions, then destroy it */

		AGhostBuilding * GhostBuilding = GetWorld()->SpawnActor<AGhostBuilding>(GhostBP,
			Statics::POOLED_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

		/* If this throws even when BP is set there's a possibility that collision is preventing
		ghost from spawning */
		UE_CLOG(GhostBuilding == nullptr, RTSLOG, Fatal, TEXT("Building %s's ghost building "
			"blueprint is null, building needs a ghost building assigned. Can use the default "
			"class"),
			*Building->GetName());

		CheckGhost(GhostBuilding, Building);

		GhostBuilding->Destroy();
		
#endif

		const FBuildingInfo & Info = BuildingInfo.Emplace(BuildingType, BuildInfo);

		/* Add button that would build this to HUDPersistentTabButtons */
	
		/* Even if tab type is None still add it. */
		EHUDPersistentTabType HUDTabType = Info.GetHUDPersistentTabCategory();
		/* Do not allow building using one of these methods to appear on a HUD persistent tab. */
		if (Info.GetBuildingBuildMethod() == EBuildingBuildMethod::LayFoundationsWhenAtLocation
			|| Info.GetBuildingBuildMethod() == EBuildingBuildMethod::Protoss)
		{
			if (HUDTabType != EHUDPersistentTabType::None)
			{
				/* Perhaps log that "building will not appear on HUD persistent panel because it
				uses a build method not suitable for it". Only relevant if HUD persistent panel
				will be used */
			}

			HUDTabType = EHUDPersistentTabType::None;
		}
		const FContextButton Button = Info.ConstructBuildButton();
		HUDPersistentTabButtons[HUDTabType].AddButton(Button);

		LargestContextMenu = FMath::Max(Building->GetContextMenu()->GetButtonsArray().Num(), LargestContextMenu);

		/* Loop through context menu buttons */
		for (const auto & ButtonElem : Building->GetContextMenu()->GetButtonsArray())
		{
			if (ButtonElem.IsForResearchUpgrade())
			{
				UpgradesResearchableThroughBuildings.Emplace(ButtonElem.GetUpgradeType());
			}
		}

		/* Take note of how many items it sells if it is a shop. HUD needs this info later */
		const int32 BuildingShopCapacity = Building->GetBuildingAttributes()->GetShoppingInfo().GetItems().Num();
		MaxNumberOfShopItemsOnAShop = FMath::Max(BuildingShopCapacity, MaxNumberOfShopItemsOnAShop);

		/* Note down it's production queue capacity */
		if (Building->GetBuildingAttributes()->CanProduce())
		{
			const int32 QueueCapacity = Building->GetBuildingAttributes()->GetProductionQueueLimit();
			LargestProductionQueueCapacity = FMath::Max(QueueCapacity, LargestProductionQueueCapacity);
		}

		/* Note down how many garrison slots the building has */
		LargestBuildingGarrisonSlotCapacity = FMath::Max<int32>(Building->GetBuildingAttributes()->GetGarrisonAttributes().GetTotalNumGarrisonSlots(), LargestBuildingGarrisonSlotCapacity);

		Building->CheckAnimationProperties();

		/* Building no longer needed */
		Building->Destroy();
	}
}

void AFactionInfo::InitUnitInfo()
{
	// Do this first
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		CollectorTypes.Emplace(Statics::ArrayIndexToResourceType(i), FUnitTypeArray());
	}
	
	/* Clear array first */
	EmptyUnitInfo();

	/* Spawn one of every unit and create FBuildInfo for each one, add
	it to player state array, then destroy unit */
	for (const auto & Elem_BP : Units)
	{
		UE_CLOG(Elem_BP == nullptr, RTSLOG, Fatal, TEXT("Faction [%s] has a null entry in the container "
			"\"Unit Roster\". Please either remove the entry or make it something valid"), TO_STRING(EFaction, Faction));
		
		/* TODO: make this specific to each map instead of one const value, and use
		that value for any other selectables created here e.g. buildings too. GroundVehicles
		were having trouble without this. Asserts throw in strange places but I assume
		the map is colliding with the vehicle and preventing it from spawning.
		An alternative to this would be to set collision in constructor (not begin play)
		to ignore all, then in begin play change collision to correct profile */
		/* Some location that does not collide with anything on map */
		const FVector SafeSpawnLoc = FVector(0.f, 0.f, 3000.f);

		AInfantry * Unit = Statics::SpawnUnitForFactionInfo(Elem_BP, SafeSpawnLoc,
			FRotator::ZeroRotator, GetWorld());

		FUnitInfo BuildInfo;
		Unit->SetupBuildInfo(BuildInfo, this);
		BuildInfo.SetSelectableBP(Elem_BP);

		PoolingManager->AddProjectileBP(Unit->GetProjectileBP());

#if WITH_EDITORONLY_DATA
		SelectableBPs.Emplace(Elem_BP);
#endif

		const EUnitType UnitType = Unit->GetType();

		UE_CLOG(UnitInfo.Contains(UnitType), RTSLOG, Fatal, TEXT("For faction \"%s\" 2 or more units "
			"share the same unit type \"%s\"."), TO_STRING(EFaction, Faction), TO_STRING(EUnitType, UnitType));

		bool bIsWorker = false;
		bool bIsCollector = false;

		/* Possibly add entry to WorkerTypes. */
		if (BuildInfo.IsAWorkerType())
		{
			WorkerTypes.Emplace(UnitType);
			bIsWorker = true;
		}
		
		/* Possibly add entries to CollectorTypes */
		for (const auto & Elem : Unit->GetInfantryAttributes()->ResourceGatheringProperties.GetTMap())
		{
			const EResourceType ResourceType = Elem.Key;
			
			CollectorTypes[ResourceType].Emplace(UnitType);
			bIsCollector = true;
		}

		/* Possibly add entry to AttackingUnitTypes. */
		if (BuildInfo.IsAArmyUnitType())
		{
			AttackingUnitTypes.Emplace(UnitType);
		}

		const FBuildInfo & Info = UnitInfo.Emplace(UnitType, BuildInfo);

		/* Add button that would build this to HUDPersistentTabButtons */
		
		/* Even if tab type is None still add it. */
		const EHUDPersistentTabType HUDTabType = Info.GetHUDPersistentTabCategory();
		const FContextButton Button = Info.ConstructBuildButton();
		HUDPersistentTabButtons[HUDTabType].AddButton(Button);

		LargestContextMenu = FMath::Max(Unit->GetContextMenu()->GetButtonsArray().Num(), LargestContextMenu);

		/* Take note of how many items this unit can carry. HUD needs this info later */
		const int32 UnitInventoryCapacity = Unit->GetInfantryAttributes()->GetInventory().GetCapacity();
		if (UnitInventoryCapacity > MaxUnitInventoryCapacity)
		{
			MaxUnitInventoryCapacity = UnitInventoryCapacity;
		}

		Unit->CheckAnimationProperties();

		/* Unit is no longer needed */
		Unit->Destroy();
	}
}

void AFactionInfo::CheckGhost(const AGhostBuilding * GhostBuilding, const ABuilding * Building) const
{
#if !UE_BUILD_SHIPPING && ASSERT_VERBOSITY >= 2

	/* Check bounds are equal */
	const FVector BuildingBoundsExtent = Building->GetBoxComponent()->GetScaledBoxExtent();
	const FVector GhostBoundsExtent = GhostBuilding->GetBoxComponent()->GetScaledBoxExtent();

	UE_CLOG(BuildingBoundsExtent != GhostBoundsExtent, RTSLOG, Fatal, TEXT("Building %s and ghost "
		"building %s do not have the same box extent for their root component"),
		*Building->GetName(), *GhostBuilding->GetName());

	/* Check mesh scale and locations */
	UMeshComponent * BuildingMesh = Building->GetMesh();
	UMeshComponent * GhostMesh = GhostBuilding->GetMesh();

	UE_CLOG(BuildingMesh->RelativeLocation != GhostMesh->RelativeLocation, RTSLOG, Fatal,
		TEXT("Building %s and ghost building %s do not have the same relative location"),
		*Building->GetName(), *GhostBuilding->GetName());

	UE_CLOG(BuildingMesh->RelativeScale3D != GhostMesh->RelativeScale3D, RTSLOG, Fatal,
		TEXT("Building %s and ghost building %s do not have the same relative scale"),
		*Building->GetName(), *GhostBuilding->GetName());

#endif // !UE_BUILD_SHIPPING && ASSERT_VERBOSITY >= 2
}

void AFactionInfo::CreatePrerequisitesText()
{
	/* You know if we call InitBuildingInfo info BEFORE InitUnitInfo then we can do all the 
	unit ones during InitUnitInfo and just need to do the building ones now. Probably a 
	small performance gain */

	for (auto & Elem : BuildingInfo)
	{
		Elem.Value.CreatePrerequisitesText(this);
	}

	for (auto & Elem : UnitInfo)
	{
		Elem.Value.CreatePrerequisitesText(this);
	}
}

void AFactionInfo::InitUpgradeInfo()
{
	for (auto & Elem : Upgrades)
	{
		FUpgradeInfo & UpgradeInfo = Elem.Value;
		
		const EHUDPersistentTabType TabType = UpgradeInfo.GetHUDPersistentTabCategory();

		/* Register what HUD persistent tab they should appear on */
		HUDPersistentTabButtons[TabType].AddButton(FContextButton(Elem.Key));

		/* Create the FText for the prerequisites */
		UpgradeInfo.CreatePrerequisitesText(this);
	}
}

void AFactionInfo::EmptyBuildingInfo()
{
	BuildingInfo.Empty();
}

void AFactionInfo::EmptyUnitInfo()
{
	UnitInfo.Empty();
}

void AFactionInfo::SortHUDPersistentTabButtons()
{
	for (auto & Elem : HUDPersistentTabButtons)
	{
		Elem.Value.Sort();
	}
}

void AFactionInfo::InitButtonToPersistentTabMappings()
{
	for (const auto & Elem : HUDPersistentTabButtons)
	{
		for (const auto & Button : Elem.Value.GetButtons())
		{
			const FTrainingInfo TrainingInfo = FTrainingInfo(Button);
			
			UE_CLOG(ButtonTypeToPersistentTabType.Contains(TrainingInfo), RTSLOG, Fatal,
				TEXT("Duplicate found for faction %s: %s"), 
				TO_STRING(EFaction, Faction), *TrainingInfo.ToString());
			
			ButtonTypeToPersistentTabType.Emplace(TrainingInfo, Elem.Key);
		}
	}
}

void AFactionInfo::InitLevelUpBonuses()
{
	for (const auto & Elem : LevelUpInfo)
	{
		if (Elem.Value.GetBonus() != nullptr)
		{
			UUpgradeEffect * const Bonus = NewObject<UUpgradeEffect>(this, Elem.Value.GetBonus());
			assert(Bonus != nullptr);
			LevelUpBonuses.Emplace(Elem.Key, Bonus);
		}
	}
}

void AFactionInfo::InitMouseCursors(URTSGameInstance * GameInst)
{
	const EMouseCursorType MaxEnumValue = static_cast<EMouseCursorType>(static_cast<int32>(EMouseCursorType::z_ALWAYS_LAST_IN_ENUM) + 1);

	/* Here if any of the cursors are "None" then assign the value in GI. The GI value could 
	also be None in which case we're buggered */
	MouseCursor = (MouseCursor == EMouseCursorType::None || MouseCursor == MaxEnumValue) ? GameInst->GetMatchMouseCursor() : MouseCursor;
	EdgeScrollingCursor_Top = (EdgeScrollingCursor_Top == EMouseCursorType::None || EdgeScrollingCursor_Top == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_Top() : EdgeScrollingCursor_Top;
	EdgeScrollingCursor_TopRight = (EdgeScrollingCursor_TopRight == EMouseCursorType::None || EdgeScrollingCursor_TopRight == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_TopRight() : EdgeScrollingCursor_TopRight;
	EdgeScrollingCursor_Right = (EdgeScrollingCursor_Right == EMouseCursorType::None || EdgeScrollingCursor_Right == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_Right() : EdgeScrollingCursor_Right;
	EdgeScrollingCursor_BottomRight = (EdgeScrollingCursor_BottomRight == EMouseCursorType::None || EdgeScrollingCursor_BottomRight == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_BottomRight() : EdgeScrollingCursor_BottomRight;
	EdgeScrollingCursor_Bottom = (EdgeScrollingCursor_Bottom == EMouseCursorType::None || EdgeScrollingCursor_Bottom == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_Bottom() : EdgeScrollingCursor_Bottom;
	EdgeScrollingCursor_BottomLeft = (EdgeScrollingCursor_BottomLeft == EMouseCursorType::None || EdgeScrollingCursor_BottomLeft == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_BottomLeft() : EdgeScrollingCursor_BottomLeft;
	EdgeScrollingCursor_Left = (EdgeScrollingCursor_Left == EMouseCursorType::None || EdgeScrollingCursor_Left == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_Left() : EdgeScrollingCursor_Left;
	EdgeScrollingCursor_TopLeft = (EdgeScrollingCursor_TopLeft == EMouseCursorType::None || EdgeScrollingCursor_TopLeft == MaxEnumValue) ? GameInst->GetEdgeScrollingCursor_TopLeft() : EdgeScrollingCursor_TopLeft;

	MouseCursor_Info = GameInst->GetMouseCursorInfo(MouseCursor);
	EdgeScrollingCursor_Infos[2] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_Top);
	EdgeScrollingCursor_Infos[4] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_TopRight);
	EdgeScrollingCursor_Infos[1] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_Right);
	EdgeScrollingCursor_Infos[7] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_BottomRight);
	EdgeScrollingCursor_Infos[5] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_Bottom);
	EdgeScrollingCursor_Infos[6] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_BottomLeft);
	EdgeScrollingCursor_Infos[0] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_Left);
	EdgeScrollingCursor_Infos[3] = GameInst->GetMouseCursorInfo(EdgeScrollingCursor_TopLeft);
	DefaultMouseCursor_CanAttackHoveredHostileUnit_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CanAttackHoveredHostileUnit);
	DefaultMouseCursor_CannotAttackHoveredHostileUnit_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CannotAttackHoveredHostileUnit);
	DefaultMouseCursor_CanAttackHoveredFriendlyUnit_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CanAttackHoveredFriendlyUnit);
	DefaultMouseCursor_CannotAttackHoveredFriendlyUnit_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CannotAttackHoveredFriendlyUnit);
	DefaultMouseCursor_CanAttackHoveredHostileBuilding_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CanAttackHoveredHostileBuilding);
	DefaultMouseCursor_CannotAttackHoveredHostileBuilding_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CannotAttackHoveredHostileBuilding);
	DefaultMouseCursor_CanAttackHoveredFriendlyBuilding_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CanAttackHoveredFriendlyBuilding);
	DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding);
	DefaultMouseCursor_CanPickUpHoveredInventoryItem_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CanPickUpHoveredInventoryItem);
	DefaultMouseCursor_CannotPickUpHoveredInventoryItem_Info = GameInst->GetMouseCursorInfo(DefaultMouseCursor_CannotPickUpHoveredInventoryItem);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		DefaultMouseCursor_CanGatherFromHoveredResourceSpot_Info[i] = GameInst->GetMouseCursorInfo(DefaultMouseCursors_CanGatherFromHoveredResourceSpot[Statics::ArrayIndexToResourceType(i)]);
		DefaultMouseCursor_CannotGatherFromHoveredResourceSpot_Info[i] = GameInst->GetMouseCursorInfo(DefaultMouseCursors_CannotGatherFromHoveredResourceSpot[Statics::ArrayIndexToResourceType(i)]);
	}

	MouseCursor_Info.SetFullPath();
	
	for (int32 i = 0; i < 8; ++i)
	{
		EdgeScrollingCursor_Infos[i].SetFullPath();
	}

	DefaultMouseCursor_CanAttackHoveredHostileUnit_Info.SetFullPath();
	DefaultMouseCursor_CannotAttackHoveredHostileUnit_Info.SetFullPath();
	DefaultMouseCursor_CanAttackHoveredFriendlyUnit_Info.SetFullPath();
	DefaultMouseCursor_CannotAttackHoveredFriendlyUnit_Info.SetFullPath();
	DefaultMouseCursor_CanAttackHoveredHostileBuilding_Info.SetFullPath();
	DefaultMouseCursor_CannotAttackHoveredHostileBuilding_Info.SetFullPath();
	DefaultMouseCursor_CanAttackHoveredFriendlyBuilding_Info.SetFullPath();
	DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding_Info.SetFullPath();
	DefaultMouseCursor_CanPickUpHoveredInventoryItem_Info.SetFullPath();
	DefaultMouseCursor_CannotPickUpHoveredInventoryItem_Info.SetFullPath();
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		DefaultMouseCursor_CanGatherFromHoveredResourceSpot_Info[i].SetFullPath();
		DefaultMouseCursor_CannotGatherFromHoveredResourceSpot_Info[i].SetFullPath();
	}
}

void AFactionInfo::InitSelectionDecals()
{
	TMap < UMaterialInterface *, FName > MaterialsWithNoMatchingParam;

	for (auto & Elem : SelectionDecals)
	{
		bool bHasParamWithName = false;

		UMaterialInterface * Decal = Elem.Value.GetDecal();

		if (Decal != nullptr)
		{
			float ThrowawayValue;
			bHasParamWithName = Decal->GetScalarParameterValue(Elem.Value.GetParamName(),
				ThrowawayValue);

			if (!bHasParamWithName)
			{
				MaterialsWithNoMatchingParam.Emplace(Decal, Elem.Value.GetParamName());
			}
		}

		Elem.Value.SetIsParamNameValid(bHasParamWithName);
	}

	/* Log which materials did not have params with name */
	if (MaterialsWithNoMatchingParam.Num() > 0)
	{
		UE_LOG(RTSLOG, Log, TEXT("For faction %s the following materials have no param with the "
			"specified name: "),
			TO_STRING(EFaction, Faction));

		for (const auto & Elem : MaterialsWithNoMatchingParam)
		{
			UE_LOG(RTSLOG, Log, TEXT("%s had no param with name %s"),
				*Elem.Key->GetFName().ToString(), *Elem.Value.ToString());
		}
	}
}

void AFactionInfo::CreateTechTree()
{
	// Buildings
	for (const auto & Info : BuildingInfo)
	{
		const EBuildingType CurrBuilding = Info.Value.GetBuildingType();
		
		bool bSkipBuildings = false;

		// Check if construction yard type building
		if (Info.Value.GetNumPersistentQueues() > 0)
		{
			/* Add link to every building info that this building can build it */
			for (auto & InnerInfo : BuildingInfo)
			{
				InnerInfo.Value.AddTechTreeParent(CurrBuilding);
			}

			bSkipBuildings = true;
		}

		/* Add links for context menu buttons */
		for (const auto & Button : Info.Value.GetContextMenu()->GetButtonsArray())
		{
			if (Button.IsForBuildBuilding())
			{
				if (!bSkipBuildings)
				{
					FBuildingInfo & Other = BuildingInfo[Button.GetBuildingType()];
					Other.AddTechTreeParent(CurrBuilding);
				}
			}
			else if (Button.IsForTrainUnit())
			{
				UE_CLOG(UnitInfo.Contains(Button.GetUnitType()) == false, RTSLOG, Fatal,
					TEXT("Selectable [%s] has a button on their context menu for [%s]. However "
						"this unit is not on the unit roster for faction [%s]"), *Info.Value.GetName().ToString(),
					TO_STRING(EUnitType, Button.GetUnitType()), TO_STRING(EFaction, GetFaction()));
				
				FUnitInfo & Other = UnitInfo[Button.GetUnitType()];
				Other.AddTechTreeParent(CurrBuilding);
			}
			else if (Button.IsForResearchUpgrade())
			{
				FUpgradeInfo & Other = GetUpgradeInfoMutable(Button.GetUpgradeType());
				Other.AddTechTreeParent(CurrBuilding); 
			}
		}
	}

	// Units
	for (const auto & Info : UnitInfo)
	{
		const EUnitType CurrUnit = Info.Key;
		
		// Add entry for each button in context menu
		for (const auto & Button : Info.Value.GetContextMenu()->GetButtonsArray())
		{
			if (Button.IsForBuildBuilding())
			{
				UE_CLOG(!BuildingInfo.Contains(Button.GetBuildingType()), RTSLOG, Fatal,
					TEXT("For faction \"%s\", unit \"%s\" has a build building button on their "
						"context menu for the building type \"%s\", but there is no building with "
						"this type on this faction's building roster"), TO_STRING(EFaction, Faction),
					*Info.Value.GetSelectableBP()->GetName(), TO_STRING(EBuildingType, Button.GetBuildingType()));
				
				FBuildingInfo & Other = BuildingInfo[Button.GetBuildingType()];
				Other.AddTechTreeParent(CurrUnit);
			}
		}
	}
}

void AFactionInfo::InitHUDWarningInfo(URTSGameInstance * GameInst)
{
	/* Anything with no entry or a blank/null entry will have the value from GI copied to it. 
	This saves a bit of performance during match by doing it now instead of every time a 
	warning happens */

	HUDMessages.Reserve(Statics::NUM_GENERIC_GAME_WARNING_TYPES);
	for (uint8 i = 0; i < Statics::NUM_GENERIC_GAME_WARNING_TYPES; ++i)
	{
		const EGameWarning WarningType = Statics::ArrayIndexToGameWarning(i);

		if (HUDMessages.Contains(WarningType))
		{
			FGameWarningInfo & Elem = HUDMessages[WarningType];
			
			if (Elem.GetMessage().IsEmpty())
			{
				Elem.SetMessage(GameInst->GetGameWarningInfo(WarningType).GetMessage());
			}

			if (Elem.GetSound() == nullptr)
			{
				Elem.SetSound(GameInst->GetGameWarningInfo(WarningType).GetSound());
			}
		}
		else
		{
			HUDMessages.Emplace(WarningType, GameInst->GetGameWarningInfo(WarningType));
		}
	}

	AbilityHUDMessages.Reserve(Statics::NUM_CUSTOM_ABILITY_CHECK_TYPES);
	for (uint8 i = 0; i < Statics::NUM_CUSTOM_ABILITY_CHECK_TYPES; ++i)
	{
		const EAbilityRequirement WarningType = Statics::ArrayIndexToAbilityRequirement(i);
		
		if (AbilityHUDMessages.Contains(WarningType))
		{
			FGameWarningInfo & Elem = AbilityHUDMessages[WarningType];

			if (Elem.GetMessage().IsEmpty())
			{
				Elem.SetMessage(GameInst->GetGameWarningInfo(WarningType).GetMessage());
			}

			if (Elem.GetSound() == nullptr)
			{
				Elem.SetSound(GameInst->GetGameWarningInfo(WarningType).GetSound());
			}
		}
		else
		{
			AbilityHUDMessages.Emplace(WarningType, GameInst->GetGameWarningInfo(WarningType));
		}

		
	}

	MissingResourceHUDMessages.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		const EResourceType WarningType = Statics::ArrayIndexToResourceType(i);
		
		if (MissingResourceHUDMessages.Contains(WarningType))
		{
			FGameWarningInfo & Elem = MissingResourceHUDMessages[WarningType];
			
			if (Elem.GetMessage().IsEmpty())
			{
				Elem.SetMessage(GameInst->GetGameWarningInfo(WarningType).GetMessage());
			}

			if (Elem.GetSound() == nullptr)
			{
				Elem.SetSound(GameInst->GetGameWarningInfo(WarningType).GetSound());
			}
		}
		else
		{
			MissingResourceHUDMessages.Emplace(WarningType, GameInst->GetGameWarningInfo(WarningType));
		}
	}

	MissingHousingResourceHUDMessages.Reserve(Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		const EHousingResourceType WarningType = Statics::ArrayIndexToHousingResourceType(i);
		
		if (MissingHousingResourceHUDMessages.Contains(WarningType))
		{
			FGameWarningInfo & Elem = MissingHousingResourceHUDMessages[WarningType];
			
			if (Elem.GetMessage().IsEmpty())
			{
				Elem.SetMessage(GameInst->GetGameWarningInfo(WarningType).GetMessage());
			}

			if (Elem.GetSound() == nullptr)
			{
				Elem.SetSound(GameInst->GetGameWarningInfo(WarningType).GetSound());
			}
		}
		else
		{
			MissingHousingResourceHUDMessages.Emplace(WarningType, GameInst->GetGameWarningInfo(WarningType));
		}
	}
}

#if WITH_EDITOR
void AFactionInfo::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	/* Set visibility of fields for upgrades */
	for (auto & Elem : Upgrades)
	{
		Elem.Value.OnPostEdit();
	}

	/* Populate the 2 housing resource arrays */
	ConstantHousingResourceAmountProvidedArray.Init(0, Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (const auto & Elem : ConstantHousingResourceAmountProvided)
	{
		if (Elem.Key != EHousingResourceType::None)
		{
			ConstantHousingResourceAmountProvidedArray[Statics::HousingResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}
	// Default values are max. Probably not going to show this in UI so should be ok
	HousingResourceLimitsArray.Init(INT32_MAX, Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (const auto & Elem : HousingResourceLimits)
	{
		if (Elem.Key != EHousingResourceType::None)
		{
			HousingResourceLimitsArray[Statics::HousingResourceTypeToArrayIndex(Elem.Key)] = Elem.Value;
		}
	}

	// Checking for entries in CommanderLevelUpInfo
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FCommanderLevelUpInfo, ExperienceRequired))
	{
		float CumulativeExperience = 0.f;
		/* Set cumulative experience on each container entry */
		for (uint8 i = 1; i <= LevelingUpOptions::MAX_COMMANDER_LEVEL; ++i) 
		{
			CumulativeExperience += CommanderLevelUpInfo[i].GetExperienceRequired();
			CommanderLevelUpInfo[i].SetCumulativeExperienceRequired(CumulativeExperience);
		}
	}
}
#endif // WITH_EDITOR

void AFactionInfo::SetInitialFaction(EFaction InFaction)
{
	Faction = InFaction;
}

const FText & AFactionInfo::GetName() const
{
	return Name;
}

UTexture2D * AFactionInfo::GetFactionIcon() const
{
	return Icon;
}

const FBuildingInfo * AFactionInfo::GetBuildingInfo(EBuildingType Type) const
{
	assert(Type != EBuildingType::NotBuilding);
	
	UE_CLOG(!BuildingInfo.Contains(Type), RTSLOG, Fatal, TEXT("Faction \"%s\": request for "
		"building info of type \"%s\" but this is not on the faction's building roster"),
		TO_STRING(EFaction, Faction), TO_STRING(EBuildingType, Type));

	return &BuildingInfo[Type];
}

const FBuildingInfo * AFactionInfo::GetBuildingInfoSlow(TSubclassOf<AActor> BuildingBP) const
{
	assert(BuildingBP != nullptr);

	for (const auto & Elem : BuildingInfo)
	{
		if (Elem.Value.GetSelectableBP() == BuildingBP)
		{
			return &Elem.Value;
		}
	}
	
	// Building not on this faction's roster
	return nullptr;
}

const FUnitInfo * AFactionInfo::GetUnitInfo(EUnitType Type) const
{
	assert(Type != EUnitType::None && Type != EUnitType::NotUnit);
	
	UE_CLOG(!UnitInfo.Contains(Type), RTSLOG, Fatal, TEXT("Faction: \"%s\", unit info request for "
		"unit type \"%s\" but this unit type is not on this faction's unit roster"), 
		TO_STRING(EFaction, Faction), TO_STRING(EUnitType, Type));

	return &UnitInfo[Type];
}

const FUpgradeInfo & AFactionInfo::GetUpgradeInfoChecked(EUpgradeType Type) const
{
	assert(Type != EUpgradeType::None);

	UE_CLOG(!Upgrades.Contains(Type), RTSLOG, Fatal, TEXT("Faction \"%s\": request for upgrade "
		"info for upgrade type \"%s\" but this upgrade is not on the faction's upgrade roster"),
		TO_STRING(EFaction, Faction), TO_STRING(EUpgradeType, Type));

	return Upgrades[Type];
}

FUpgradeInfo & AFactionInfo::GetUpgradeInfoMutable(EUpgradeType Type)
{
	assert(Type != EUpgradeType::None);

	UE_CLOG(!Upgrades.Contains(Type), RTSLOG, Fatal, TEXT("Faction \"%s\": request for upgrade "
		"info for upgrade type \"%s\" but this upgrade is not on the faction's upgrade roster"),
		TO_STRING(EFaction, Faction), TO_STRING(EUpgradeType, Type));

	return Upgrades[Type];
}

const FUpgradeInfo * AFactionInfo::GetUpgradeInfoNotChecked(EUpgradeType UpgradeType) const
{
	if (Upgrades.Contains(UpgradeType))
	{
		return &Upgrades[UpgradeType];
	}
	else
	{
		return nullptr;
	}

	/* During setup consider populating Upgrades with null if the upgrade type isn't on the
	faction's upgrade roster to make this function run faster */
}

bool AFactionInfo::IsUpgradeResearchableThroughBuilding(EUpgradeType UpgradeType) const
{
	return UpgradesResearchableThroughBuildings.Contains(UpgradeType);
}

#if WITH_EDITORONLY_DATA
bool AFactionInfo::HasSelectable(const TSubclassOf<AActor>& ActorBP) const
{
	assert(Faction != EFaction::None);

	return SelectableBPs.Contains(ActorBP);
}
#endif

float AFactionInfo::GetProductionTime(const FTrainingInfo & TrainingInfo) const
{
	if (TrainingInfo.IsForUnit())
	{
		const FBuildInfo * Info = GetUnitInfo(TrainingInfo.GetUnitType());
		return Info->GetTrainTime();
	}
	else if (TrainingInfo.IsProductionForBuilding())
	{
		const FBuildInfo * Info = GetBuildingInfo(TrainingInfo.GetBuildingType());
		return Info->GetTrainTime();
	}
	else // Assuming for upgrade
	{
		assert(TrainingInfo.IsForUpgrade());

		const FUpgradeInfo & Info = GetUpgradeInfoChecked(TrainingInfo.GetUpgradeType());
		return Info.GetTrainTime();
	}
}

TSubclassOf<AStartingGrid> AFactionInfo::GetStartingGrid() const
{
	return StartingGrid;
}

EFaction AFactionInfo::GetFaction() const
{
	assert(Faction != EFaction::None);
	return Faction;
}

const TMap<EUpgradeType, FUpgradeInfo>& AFactionInfo::GetUpgradesMap() const
{
	return Upgrades;
}

const FRankInfo & AFactionInfo::GetLevelUpInfo(uint8 Level) const
{
	return LevelUpInfo[Level];
}

UUpgradeEffect * AFactionInfo::GetLevelUpBonus(uint8 Level) const
{
	if (LevelUpBonuses.Contains(Level))
	{
		return LevelUpBonuses[Level];
	}
	else
	{
		return nullptr;
	}
}

const FCommanderLevelUpInfo & AFactionInfo::GetCommanderLevelUpInfo(uint8 InRank) const
{
	UE_CLOG(CommanderLevelUpInfo.Contains(InRank) == false, RTSLOG, Fatal, TEXT("For faction [%s] "
		"there is no commander leveling up entry for rank [%d] "), TO_STRING(EFaction, Faction), InRank);
	
	return CommanderLevelUpInfo[InRank];
}

int32 AFactionInfo::GetNumInitialCommanderSkillPoints() const
{
	return NumInitialCommanderSkillPoints;
}

const FCursorInfo & AFactionInfo::GetMouseCursorInfo() const
{
	return MouseCursor_Info;
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_Top() const
{
	return EdgeScrollingCursor_Infos[2];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_TopRight() const
{
	return EdgeScrollingCursor_Infos[4];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_Right() const
{
	return EdgeScrollingCursor_Infos[1];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_BottomRight() const
{
	return EdgeScrollingCursor_Infos[7];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_Bottom() const
{
	return EdgeScrollingCursor_Infos[5];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_BottomLeft() const
{
	return EdgeScrollingCursor_Infos[6];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_Left() const
{
	return EdgeScrollingCursor_Infos[0];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo_TopLeft() const
{
	return EdgeScrollingCursor_Infos[3];
}

const FCursorInfo & AFactionInfo::GetEdgeScrollingCursorInfo(int32 ArrayIndex) const
{
	return EdgeScrollingCursor_Infos[ArrayIndex];
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanAttackHoveredHostileUnit() const
{
	return DefaultMouseCursor_CanAttackHoveredHostileUnit_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotAttackHoveredHostileUnit() const
{
	return DefaultMouseCursor_CannotAttackHoveredHostileUnit_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyUnit() const
{
	return DefaultMouseCursor_CanAttackHoveredFriendlyUnit_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyUnit() const
{
	return DefaultMouseCursor_CannotAttackHoveredFriendlyUnit_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanAttackHoveredHostileBuilding() const
{
	return DefaultMouseCursor_CanAttackHoveredHostileBuilding_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotAttackHoveredHostileBuilding() const
{
	return DefaultMouseCursor_CannotAttackHoveredHostileBuilding_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanAttackHoveredFriendlyBuilding() const
{
	return DefaultMouseCursor_CanAttackHoveredFriendlyBuilding_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotAttackHoveredFriendlyBuilding() const
{
	return DefaultMouseCursor_CannotAttackHoveredFriendlyBuilding_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanPickUpHoveredInventoryItem() const
{
	return DefaultMouseCursor_CanPickUpHoveredInventoryItem_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotPickUpHoveredInventoryItem() const
{
	return DefaultMouseCursor_CannotPickUpHoveredInventoryItem_Info;
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CanGatherFromHoveredResourceSpot(EResourceType ResourceType) const
{
	const int32 Index = Statics::ResourceTypeToArrayIndex(ResourceType);
	return DefaultMouseCursor_CanGatherFromHoveredResourceSpot_Info[Index];
}

const FCursorInfo & AFactionInfo::GetDefaultMouseCursorInfo_CannotGatherFromHoveredResourceSpot(EResourceType ResourceType) const
{
	const int32 Index = Statics::ResourceTypeToArrayIndex(ResourceType);
	return DefaultMouseCursor_CannotGatherFromHoveredResourceSpot_Info[Index];
}

const TSubclassOf<UWorldWidget> & AFactionInfo::GetSelectablePersistentWorldWidget() const
{
	return SelectablePersistentWidget;
}

const TSubclassOf <UWorldWidget> & AFactionInfo::GetSelectableSelectionWorldWidget() const
{
	return SelectableSelectionWidget;
}

const TSubclassOf<UWorldWidget> & AFactionInfo::GetResourceSpotPersistentWorldWidget() const
{
	return ResourceSpotPersistentWidget;
}

const TSubclassOf<UWorldWidget> & AFactionInfo::GetResourceSpotSelectionWorldWidget() const
{
	return ResourceSpotSelectionWidget;
}

const FSelectionDecalInfo & AFactionInfo::GetSelectionDecalInfo(EAffiliation Affiliation) const
{
	return SelectionDecals[Affiliation];
}

UParticleSystem * AFactionInfo::GetSelectionParticles(EAffiliation Affiliation, int32 SelectablesSize) const
{
	return SelectionParticles[Affiliation].GetTemplate(SelectablesSize);
}

UParticleSystem * AFactionInfo::GetRightClickParticles(EAffiliation Affiliation, int32 SelectablesSize) const
{
	return RightClickCommandParticles[Affiliation].GetTemplate(SelectablesSize);
}

const FParticleInfo & AFactionInfo::GetRightClickCommandStaticParticles(ECommandTargetType TargetType) const
{
	return RightClickCommandStaticParticles[TargetType];
}

const FDecalInfo & AFactionInfo::GetRightClickCommandDecal(EAffiliation TargetAffiliation) const
{
	return RightClickCommandDecals[TargetAffiliation];
}

TSubclassOf < UInGameWidgetBase > AFactionInfo::GetMatchWidgetBP(EMatchWidgetType WidgetType) const
{
	return MatchWidgets_BP.Contains(WidgetType) ? MatchWidgets_BP[WidgetType] : nullptr;
}

const FText & AFactionInfo::GetHUDErrorMessage(EGameWarning MessageType) const
{
	return HUDMessages[MessageType].GetMessage();
}

const FText & AFactionInfo::GetHUDErrorMessage(EAbilityRequirement ReasonForMessage) const
{
	return AbilityHUDMessages[ReasonForMessage].GetMessage();
}

const FText & AFactionInfo::GetHUDMissingResourceMessage(EResourceType ResourceType) const
{
	return MissingResourceHUDMessages[ResourceType].GetMessage();
}

const FText & AFactionInfo::GetHUDMissingHousingResourceMessage(EHousingResourceType HousingResourceType) const
{
	return MissingHousingResourceHUDMessages[HousingResourceType].GetMessage();
}

USoundBase * AFactionInfo::GetWarningSound(EGameWarning MessageType) const
{
	return HUDMessages[MessageType].GetSound();
}

USoundBase * AFactionInfo::GetWarningSound(EAbilityRequirement ReasonForMessage) const
{
	return AbilityHUDMessages[ReasonForMessage].GetSound();
}

USoundBase * AFactionInfo::GetWarningSound(EResourceType ResourceType) const
{
	return MissingResourceHUDMessages[ResourceType].GetSound();
}

USoundBase * AFactionInfo::GetWarningSound(EHousingResourceType HousingResourceType) const
{
	return MissingHousingResourceHUDMessages[HousingResourceType].GetSound();
}

const FContextButton * AFactionInfo::GetHUDPersistentTabButton(EHUDPersistentTabType TabType, int32 ButtonIndex) const
{
	/* It is possible for TabType == None to return a valid pointer */

	if (HUDPersistentTabButtons.Contains(TabType))
	{
		if (HUDPersistentTabButtons[TabType].GetButtons().IsValidIndex(ButtonIndex))
		{
			return &HUDPersistentTabButtons[TabType].GetButtons()[ButtonIndex];
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}
}

const TMap<EHUDPersistentTabType, FButtonArray>& AFactionInfo::GetHUDPersistentTabButtons() const
{
	return HUDPersistentTabButtons;
}

EHUDPersistentTabType AFactionInfo::GetHUDPersistentTab(const FTrainingInfo & InInfo) const
{
	return ButtonTypeToPersistentTabType[InInfo];
}

EHUDPersistentTabType AFactionInfo::GetHUDPersistentTab(const FContextButton & InButton) const
{
	return GetHUDPersistentTab(FTrainingInfo(InButton));
}

const TArray<int32>& AFactionInfo::GetConstantHousingResourceAmountProvided() const
{
	return ConstantHousingResourceAmountProvidedArray;
}

const TArray<int32>& AFactionInfo::GetHousingResourceLimits() const
{
	return HousingResourceLimitsArray;
}

bool AFactionInfo::IsQuantityLimited(EBuildingType BuildingType) const
{
	return BuildingQuantityLimits.Contains(BuildingType);
}

bool AFactionInfo::IsQuantityLimited(EUnitType UnitType) const
{
	return UnitQuantityLimits.Contains(UnitType);
}

int32 AFactionInfo::GetQuantityLimit(EBuildingType BuildingType) const
{
	return BuildingQuantityLimits[BuildingType].GetInteger();
}

int32 AFactionInfo::GetQuantityLimit(EUnitType UnitType) const
{
	return UnitQuantityLimits[UnitType].GetInteger();
}

const FBuildInfo * AFactionInfo::GetBuildInfo(const FContextButton & ContextButton) const
{
	if (ContextButton.GetButtonType() == EContextButton::BuildBuilding)
	{
		return GetBuildingInfo(ContextButton.GetBuildingType());
	}
	else  // Assuming GetButtonType == EContextButton::Train
	{
		assert(!ContextButton.IsForResearchUpgrade());

		return GetUnitInfo(ContextButton.GetUnitType());
	}
}

const FDisplayInfoBase * AFactionInfo::GetDisplayInfo(const FContextButton & ContextButton) const
{
	if (ContextButton.IsForBuildBuilding())
	{
		return GetBuildingInfo(ContextButton.GetBuildingType());
	}
	else if (ContextButton.IsForTrainUnit())
	{
		return GetUnitInfo(ContextButton.GetUnitType());
	}
	else // Assuming for upgrade
	{
		assert(ContextButton.IsForResearchUpgrade());

		return &GetUpgradeInfoChecked(ContextButton.GetUpgradeType());
	}
}

const TMap<EUnitType, FAtLeastOneInt32>& AFactionInfo::GetUnitQuantityLimits() const
{
	return UnitQuantityLimits;
}

EBuildingBuildMethod AFactionInfo::GetDefaultBuildingBuildMethod() const
{
	return DefaultBuildingBuildMethod;
}

float AFactionInfo::GetBuildingProximityRange() const
{
	return BuildingProximityRange;
}

bool AFactionInfo::CanBuildOffAllies() const
{
	return bCanBuildOffAllies;
}

bool AFactionInfo::HasOverriddenResourceImage(EResourceType ResourceType) const
{
	return ResourceIconOverrides.Contains(ResourceType);
}

UTexture2D * AFactionInfo::GetResourceImage(EResourceType ResourceType) const
{
	return ResourceIconOverrides[ResourceType];
}

USoundBase * AFactionInfo::GetMatchMusic() const
{
	return MatchMusic;
}

USoundBase * AFactionInfo::GetChangeRallyPointSound() const
{
	return ChangeRallyPointSound;
}

int32 AFactionInfo::GetNumUpgrades() const
{
	return Upgrades.Num();
}

const TMap<EBuildingType, FBuildingInfo>& AFactionInfo::GetAllBuildingTypes() const
{
	return BuildingInfo;
}

const TArray<EBuildingType>& AFactionInfo::GetPersistentQueueTypes() const
{
	return PersistentQueueTypes;
}

const TMap<EUnitType, FUnitInfo>& AFactionInfo::GetAllUnitTypes() const
{
	return UnitInfo;
}

const TArray<EUnitType>& AFactionInfo::GetWorkerTypes() const
{
	return WorkerTypes;
}

const TArray<EUnitType>& AFactionInfo::GetCollectorTypes(EResourceType ResourceType) const
{
	return CollectorTypes[ResourceType].GetArray();
}

const TArray<EBuildingType>& AFactionInfo::GetAllDepotsForResource(EResourceType ResourceType) const
{
	return ResourceDepotTypes[ResourceType].GetArray();
}

const TArray<EBuildingType>& AFactionInfo::GetBarracksBuildingTypes() const
{
	return BarracksTypes;
}

const TArray<EUnitType>& AFactionInfo::GetAttackingUnitTypes() const
{
	return AttackingUnitTypes;
}

const TArray<EBuildingType>& AFactionInfo::GetBaseDefenseTypes() const
{
	return BaseDefenseTypes;
}

bool AFactionInfo::HasCollectorType(EResourceType ResourceType) const
{
	return CollectorTypes[ResourceType].GetArray().Num() > 0;
}

bool AFactionInfo::HasUpgrades() const
{
	return GetNumUpgrades() > 0;
}

bool AFactionInfo::HasBaseDefenseTypeBuildings() const
{
	return BaseDefenseTypes.Num() > 0;
}

bool AFactionInfo::HasAttackingUnitTypes() const
{
	return AttackingUnitTypes.Num() > 0;
}

EBuildingType AFactionInfo::Random_GetPersistentQueueSupportingType() const
{
	if (PersistentQueueTypes.Num() == 0)
	{
		return EBuildingType::NotBuilding;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, PersistentQueueTypes.Num() - 1);
		return PersistentQueueTypes[RandomIndex];
	}
}

EUnitType AFactionInfo::Random_GetWorkerType() const
{
	if (WorkerTypes.Num() == 0)
	{
		return EUnitType::None;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, WorkerTypes.Num() - 1);
		return WorkerTypes[RandomIndex];
	}
}

EUnitType AFactionInfo::Random_GetAttackingUnitType() const
{
	if (AttackingUnitTypes.Num() == 0)
	{
		return EUnitType::None;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, AttackingUnitTypes.Num() - 1);
		return AttackingUnitTypes[RandomIndex];
	}
}

EBuildingType AFactionInfo::Random_GetBaseDefenseType() const
{
	if (BaseDefenseTypes.Num() == 0)
	{
		return EBuildingType::NotBuilding;
	}
	else
	{
		const int32 RandomIndex = FMath::RandRange(0, BaseDefenseTypes.Num() - 1);
		return BaseDefenseTypes[RandomIndex];
	}
}

int32 AFactionInfo::GetLargestContextMenu() const
{
	return LargestContextMenu;
}

int32 AFactionInfo::GetLargestProductionQueueCapacity() const
{
	return LargestProductionQueueCapacity;
}

int32 AFactionInfo::GetLargestBuildingGarrisonSlotCapacity() const
{
	return LargestBuildingGarrisonSlotCapacity;
}

int32 AFactionInfo::GetLargestShopCatalogue() const
{
	return MaxNumberOfShopItemsOnAShop;
}

int32 AFactionInfo::GetLargestInventory() const
{
	return MaxUnitInventoryCapacity;
}

bool operator<(const AFactionInfo & Info1, const AFactionInfo & Info2)
{
	const uint8 Int1 = static_cast<uint8>(Info1.Faction);
	const uint8 Int2 = static_cast<uint8>(Info2.Faction);

	return Int1 < Int2;
}


