// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSGameInstance.h"
#include "Engine/World.h"
#include "Sound/AudioSettings.h"
#include "UObject/UObjectIterator.h"
#include "Sound/SoundMix.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Kismet/GameplayStatics.h"

#include "Statics/Statics.h"
#include "GameFramework/FactionInfo.h"
#include "Settings/DevelopmentSettings.h"
#include "Managers/ObjectPoolingManager.h"
#include "Statics/DevelopmentStatics.h"
#include "Settings/RTSGameUserSettings.h"
#include "UI/InGameWidgetBase.h"
#include "MapElements/Invisible/RTSLevelVolume.h"
#include "MapElements/ResourceSpot.h"
#include "Managers/BuffAndDebuffManager.h"
#include "MapElements/Abilities/AbilityBase.h" // All for SetupCustomContextActions
#include "Statics/InventoryItems.h"
#include "MapElements/InventoryItem.h"
#include "Managers/HeavyTaskManager.h"


URTSGameInstance::URTSGameInstance()
{
	for (int32 i = 0; i < Statics::NUM_FACTIONS; ++i)
	{
		FactionInfo_BP.Emplace(Statics::ArrayIndexToFaction(i), nullptr);
	}

	/* Make all damage multipliers default values */
	for (const auto & Elem : Statics::DamageTypes)
	{
		DamageMultipliers.Emplace(Elem, FDamageMultipliers());
	}

	/* Default populate BuildingNetworkInfo */
	BuildingNetworkInfo.Reserve(Statics::NUM_BUILDING_GARRISON_NETWORK_TYPES);
	for (uint8 i = 0; i < Statics::NUM_BUILDING_GARRISON_NETWORK_TYPES; ++i)
	{
		BuildingNetworkInfo.Emplace(Statics::ArrayIndexToBuildingNetworkType(i), FBuildingNetworkAttributes());
	}

	/* Setup some common context actions */
	SetupCommonContextActions();
	SetupCustomContextActions();
	/* Fill the TMap with any missing abilities with default struct */
	SetupMissingContextActions();

	/* Setup default values for widgets array */
	for (uint8 i = 0; i < Statics::NUM_MENU_WIDGET_TYPES; ++i)
	{
		Widgets_BP.Emplace(static_cast<EWidgetType>(i), nullptr);
	}

	for (uint8 i = 0; i < Statics::NUM_MESSAGE_RECIPIENT_TYPES; ++i)
	{
		ChatMessageReceivedSounds.Emplace(Statics::ArrayIndexToMessageRecipientType(i), nullptr);
	}

	/* Default intialize buffs/debuffs info TMaps */
	StaticBuffsAndDebuffs.Reserve(Statics::NUM_STATIC_BUFF_AND_DEBUFF_TYPES);
	for (uint8 i = 0; i < Statics::NUM_STATIC_BUFF_AND_DEBUFF_TYPES; ++i)
	{
		const EStaticBuffAndDebuffType Type = Statics::ArrayIndexToStaticBuffOrDebuffType(i);
		
		StaticBuffsAndDebuffs.Emplace(Type, FStaticBuffOrDebuffInfo(Type));
	}
	TickableBuffsAndDebuffs.Reserve(Statics::NUM_TICKABLE_BUFF_AND_DEBUFF_TYPES);
	for (uint8 i = 0; i < Statics::NUM_TICKABLE_BUFF_AND_DEBUFF_TYPES; ++i)
	{
		const ETickableBuffAndDebuffType Type = Statics::ArrayIndexToTickableBuffOrDebuffType(i);
		
		TickableBuffsAndDebuffs.Emplace(Type, FTickableBuffOrDebuffInfo(Type));
	}

	BuffAndDebuffSubTypeInfo.Reserve(Statics::NUM_STATIC_BUFF_AND_DEBUFF_TYPES);
	for (uint8 i = 0; i < Statics::NUM_STATIC_BUFF_AND_DEBUFF_TYPES; ++i)
	{
		BuffAndDebuffSubTypeInfo.Emplace(Statics::ArrayIndexToBuffOrDebuffSubType(i),
			FBuffOrDebuffSubTypeInfo());
	}

	SetupCPUInfo();
	SetupLoadingScreenMessages();
	SetupMatchWidgets_BP();
	SetDefaultHUDMessages();
	SetupResourceInfo();
	SetupDefeatConditionInfo();
	SetupLeaveOnMapList();
	SetupSelectableResourceInfo();
	SetupInventoryItemInfo();
	SetupKeyInfo();

#if WITH_EDITORONLY_DATA
	DevelopmentSettings_BP = ADevelopmentSettings::StaticClass();
#endif

	WidgetTransitionFunction = nullptr;
	bIsColdBooting = true;
	bIsInMainMenuMap = true;

	ExperienceBountyMultiplierPerLevel = 1.5f;

	HUDGameMessageCooldown = 0.5f;
	MarqueeSelectionBoxStyle = EMarqueeBoxDrawMethod::BorderAndFill;
	MarqueeBoxRectangleFillColor = FLinearColor(0.f, 1.f, 0.f, 0.2f);
	MarqueeBoxBorderColor = FLinearColor(0.f, 1.f, 0.f, 1.f);
	MarqueeBoxBorderLineThickness = 2.f;

	BuildingStartHealthPercentage = 0.05f;

	GhostBadLocationParamName = FName("RTS_BadBuildLocation");
	GhostBadLocationParamValue = FLinearColor(1.f, 0.f, 0.f, 1.f);
	PlayerSpawnRule = EPlayerSpawnRule::NearTeammates;
}

void URTSGameInstance::SetupInitialSettings()
{
	// Create one of these 
	SoundMix = NewObject<USoundMix>(this);

	/* Do this to make volume adjustments in the future possible */
	UGameplayStatics::PushSoundMixModifier(GetWorld(), SoundMix);

	/* Now find all sound classes and store them in hashmap */
	int32 NumFound = 0;
	for (TObjectIterator <USoundClass> Iter; Iter; ++Iter)
	{
		USoundClass * SoundClass = *Iter;

		SoundClasses.Emplace(SoundClass->GetName(), SoundClass);
		NumFound++;
	}

#if !UE_BUILD_SHIPPING
	if (NumFound == 0)
	{
		// If you want no audio in your game then you can ignore this!
		UE_LOG(RTSLOG, Warning, TEXT("No sound classes found when initializing game instance for "
			"cold boot. Will not be able to adjust volume"));
	}
	else
	{
		UE_LOG(RTSLOG, Log, TEXT("Number of sound classes found during game instance initialization: %d. "
			"Their names are: "), NumFound);

		for (const auto & Elem : SoundClasses)
		{
			UE_LOG(RTSLOG, Log, TEXT("%s"), *Elem.Key);
		}
	}
#endif

	URTSGameUserSettings * UserSettings = CastChecked<URTSGameUserSettings>(UGameUserSettings::GetGameUserSettings());

	/* Check if this is the very first time starting game. If it is then need to do some extra
	stuff. Need to:
	- Set all the config varaibles in URTSGameUserSettings to defaults
	- Write them to the .ini file
	Assuming UGameUserSettings::PostInitProperties has been called by now (Should have, we call
	this from BeignPlay) */
	UserSettings->InitialSetup(this);

	/* Apply all audio and other settings */
	UserSettings->ApplyAllSettings(this, GetWorld()->GetFirstPlayerController());

	UserSettings->GameStartType = EGameStartType::HasStartedGameBefore;
}

void URTSGameInstance::InitDevelopmentSettings()
{
#if WITH_EDITOR
	DevelopmentSettings = GetWorld()->SpawnActor<ADevelopmentSettings>(DevelopmentSettings_BP,
		Statics::INFO_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

	/* Assign reference to editor utility widget. This is relatively slow. When I eventually figure out how
	to create editor utility widgets through C++ alone I might be able to save a ref to
	the widget in URTSUnrealEdEngine and then I can just reference it now
	TODO. 
	Also while playing in standalone this does not get anything. Hopefully when I save the 
	widget's ref on my URTSUnrealEdEngine class then that will be valid */
	for (TObjectIterator<UEditorPlaySettingsWidget> Iter; Iter; ++Iter)
	{
		EditorPlaySettingsWidget = *Iter;
		break;
	}
#endif
}

void URTSGameInstance::CreatePoolingManager()
{
	assert(PoolingManager == nullptr);
	PoolingManager = GetWorld()->SpawnActor<AObjectPoolingManager>(Statics::INFO_ACTOR_SPAWN_LOCATION,
		FRotator::ZeroRotator);
	assert(PoolingManager != nullptr);
}

void URTSGameInstance::InitFactionInfo()
{
	/* Ensure no BeginPlays are run by spawned actors */
	bIsInitializingFactionInfo = true;

	/* Fill FactionInfo with nullptr */
	FactionInfo.Init(nullptr, Statics::NUM_FACTIONS);

	/* Create info for all factions and add them to FactionInfo */
	for (const auto & Elem : FactionInfo_BP)
	{
		const EFaction Faction = Elem.Key;
		const TSubclassOf < AFactionInfo > & Faction_BP = Elem.Value;

		UE_CLOG(Faction_BP == nullptr, RTSLOG, Fatal, TEXT("No faction info assigned to faction %s, "
			"need to assign some faction info to them"),
			TO_STRING(EFaction, Faction));

		/* Doing deferred spawn to set faction on faction info before it does BeginPlay and
		sets up. From the small amount of testing I've done this doesn't seem required but
		doing it just in case */
		const FTransform Transform = FTransform(FRotator::ZeroRotator,
			Statics::INFO_ACTOR_SPAWN_LOCATION, FVector::OneVector);
		AFactionInfo * NewInfo = GetWorld()->SpawnActorDeferred<AFactionInfo>(Faction_BP, Transform);

		/* Set this on the faction. For a long time this was exposed to editor so was already
		set when FI spawned */
		NewInfo->SetInitialFaction(Faction);

		AActor * AsActor = UGameplayStatics::FinishSpawningActor(NewInfo, Transform);
		assert(AsActor != nullptr);

		// Potentially update these 2 variables
		if (NewInfo->GetLargestShopCatalogue() > LargestShopCatalogueSize)
		{
			LargestShopCatalogueSize = NewInfo->GetLargestShopCatalogue();
		}
		if (NewInfo->GetLargestInventory() > LargestInventoryCapacity)
		{
			LargestInventoryCapacity = NewInfo->GetLargestInventory();
		}

		// Place in array
		const int32 Index = Statics::FactionToArrayIndex(Faction);
		FactionInfo[Index] = NewInfo;
	}

	/* Apparently no faction info will cause crashes here. Most likely because it was init
	with nullptr and the sorting method tries to dereference them */
	FactionInfo.Sort();

	bIsInitializingFactionInfo = false;
}

void URTSGameInstance::InitContextActions()
{
	UE_CLOG(ContextActionsMap.Num() != Statics::NUM_CONTEXT_ACTIONS, RTSLOG, Fatal,
		TEXT("Not all context actions are defined in game instance or too many. Expected: %d ,"
			"actual amount: %d "), Statics::NUM_CONTEXT_ACTIONS, ContextActionsMap.Num());

	/* TODO: Array expected to be empty here but requires .Empty() call. Cannot figure out why.
	All references to ContextActions seem to indicate it should be empty. Perhaps BP is bugged
	Try and figure this out so assert can be uncommented but for now this gets things working */
	//assert(ContextActions.Num() == 0);
	ContextActions.Empty();

	/* I know this can be done at same time as Empty(), I *think* I've had issues with that
	func where param > 0 so I'm just doing this seperately for now */
	ContextActions.Reserve(ContextActionsMap.Num());
	const ETargetingType MaxEnumValue = static_cast<ETargetingType>(static_cast<int32>(ETargetingType::z_ALWAYS_LAST_IN_ENUM) + 1);

	/* Transfer elements from hashmap to array because marignally faster lookups, and do setup
	actions */
	for (auto & Elem : ContextActionsMap)
	{
		Elem.Value.SetInitialType(Elem.Key);

		ContextActions.Emplace(Elem.Value);

		/* Convert ETargetingTypes to FNames */
		for (const auto & SetEntry : ContextActions.Last().GetAcceptableTargets())
		{
			if (SetEntry != MaxEnumValue)
			{
				ContextActions.Last().AddAcceptableTargetFName(Statics::GetTargetingType(SetEntry));
			}
		}

		/* Set hardware cursor full paths */
		ContextActions.Last().SetupHardwareCursors(this);
	}

	ContextActions.Sort();

	/* Free unneeded memory */
	ContextActionsMap.Empty();
}

void URTSGameInstance::InitCommanderAbilityInfo()
{
	CommanderAbilities.Init(FCommanderAbilityInfo(), Statics::NUM_COMMANDER_ABILITY_TYPES);

	for (const auto & Pair : CommanderAbilitiesTMap)
	{
		const ECommanderAbility AbilityType = Pair.Key;
		
		const int32 Index = Statics::CommanderAbilityTypeToArrayIndex(AbilityType);
		CommanderAbilities[Index] = Pair.Value;

		FCommanderAbilityInfo & Elem = CommanderAbilities[Index];
		Elem.SetType(AbilityType);
		Elem.PopulateAcceptableTargetFNames();
		Elem.SetupHardwareCursors(this);
	}

	/* Free unneeded memory */
	CommanderAbilitiesTMap.Empty();
}

void URTSGameInstance::InitCommanderSkillTreeNodeInfo()
{
	CommanderSkillTreeNodes.Init(FCommanderAbilityTreeNodeInfo(), Statics::NUM_COMMANDER_SKILL_TREE_NODE_TYPES);

	for (const auto & Pair : CommanderSkillTreeNodeTMap)
	{
		const ECommanderSkillTreeNodeType NodeType = Pair.Key;

		const int32 Index = Statics::CommanderSkillTreeNodeToArrayIndex(NodeType);
		CommanderSkillTreeNodes[Index] = Pair.Value;

		FCommanderAbilityTreeNodeInfo & Elem = CommanderSkillTreeNodes[Index];
		Elem.SetupInfo(this, NodeType);
	}

	/* Free unneeded memory */
	CommanderSkillTreeNodeTMap.Empty();
}

void URTSGameInstance::InitBuildingTargetingAbilityInfo()
{
	const EMouseCursorType MaxEnumValue = static_cast<EMouseCursorType>(static_cast<int32>(EMouseCursorType::z_ALWAYS_LAST_IN_ENUM) + 1);
	for (auto & Pair : BuildingTargetingAbilities)
	{
		Pair.Value.InitialSetup(Pair.Key);
		Pair.Value.SetupMouseCursors(this, MaxEnumValue);
	}
}

void URTSGameInstance::SetupBuffsAndDebuffInfos()
{
	/* Add what functions you want your buffs/debuffs to use here */

	//=======================================================
	//	'Static' type buffs/debuffs

	// Guard in case we don't have it defined anymore
	if (StaticBuffsAndDebuffs.Contains(EStaticBuffAndDebuffType::ThePlague))
	{
		StaticBuffsAndDebuffs[EStaticBuffAndDebuffType::ThePlague].SetFunctionPointers(
			&ABuffAndDebuffManager::ThePlague_TryApplyTo,
			&ABuffAndDebuffManager::ThePlague_OnRemoved
		);
	}

	//================================================
	//	'Tickable' type buffs/debuffs

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::BasicHealOverTime))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::BasicHealOverTime].SetFunctionPointers(
			&ABuffAndDebuffManager::BasicHealOverTime_TryApplyTo,
			&ABuffAndDebuffManager::BasicHealOverTime_DoTick,
			&ABuffAndDebuffManager::BasicHealOverTime_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::CleansersMight))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::CleansersMight].SetFunctionPointers(
			&ABuffAndDebuffManager::CleansersMightOrWhatever_TryApplyTo,
			&ABuffAndDebuffManager::CleansersMightOrWhatever_DoTick,
			&ABuffAndDebuffManager::CleansersMightOrWhatever_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::Dash))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::Dash].SetFunctionPointers(
			&ABuffAndDebuffManager::Dash_TryApplyTo,
			&ABuffAndDebuffManager::Dash_DoTick,
			&ABuffAndDebuffManager::Dash_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::IncreasingHealOverTime))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::IncreasingHealOverTime].SetFunctionPointers(
			&ABuffAndDebuffManager::IncreasingHealOverTime_TryApplyTo,
			&ABuffAndDebuffManager::IncreasingHealOverTime_DoTick,
			&ABuffAndDebuffManager::IncreasingHealOverTime_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::PainOverTime))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::PainOverTime].SetFunctionPointers(
			&ABuffAndDebuffManager::PainOverTime_TryApplyTo,
			&ABuffAndDebuffManager::PainOverTime_DoTick,
			&ABuffAndDebuffManager::PainOverTime_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::Haste))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::Haste].SetFunctionPointers(
			&ABuffAndDebuffManager::Haste_TryApplyTo,
			&ABuffAndDebuffManager::Haste_DoTick,
			&ABuffAndDebuffManager::Haste_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::NearInvulnerability))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::NearInvulnerability].SetFunctionPointers(
			&ABuffAndDebuffManager::NearInvulnerability_TryApplyTo,
			&ABuffAndDebuffManager::NearInvulnerability_DoTick,
			&ABuffAndDebuffManager::NearInvulnerability_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::Corruption))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::Corruption].SetFunctionPointers(
			&ABuffAndDebuffManager::Corruption_TryApplyTo,
			&ABuffAndDebuffManager::Corruption_DoTick,
			&ABuffAndDebuffManager::Corruption_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::TempStealth))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::TempStealth].SetFunctionPointers(
			&ABuffAndDebuffManager::TempStealth_TryApplyTo,
			&ABuffAndDebuffManager::TempStealth_DoTick,
			&ABuffAndDebuffManager::TempStealth_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::RottenPumpkinEatEffect))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::RottenPumpkinEatEffect].SetFunctionPointers(
			&ABuffAndDebuffManager::EatPumpkinEffect_TryApplyTo,
			&ABuffAndDebuffManager::EatPumpkinEffect_DoTick,
			&ABuffAndDebuffManager::EatPumpkinEffect_OnRemoved
		);
	}

	if (TickableBuffsAndDebuffs.Contains(ETickableBuffAndDebuffType::Beserk))
	{
		TickableBuffsAndDebuffs[ETickableBuffAndDebuffType::Beserk].SetFunctionPointers(
			&ABuffAndDebuffManager::Beserk_TryApplyTo,
			&ABuffAndDebuffManager::Beserk_DoTick,
			&ABuffAndDebuffManager::Beserk_OnRemoved
		);
	}
}

void URTSGameInstance::InitInventoryItemInfo()
{
	//============================================================================================
	//	Function pointers
	//--------------------------------------------------------------------------------------------
	//	Functions that describe what happens when the item is aquired/removed and possibly more
	//============================================================================================

	InventoryItemInfo[EInventoryItem::Shoes].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::Shoes_OnAquired,
		&InventoryItemBehavior::Shoes_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::SimpleBangle].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::Bangle_OnAquired,
		&InventoryItemBehavior::Bangle_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::RedGem].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::RedGem_OnAquired,
		&InventoryItemBehavior::RedGem_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::GreenGem].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::GreenGem_OnAquired,
		&InventoryItemBehavior::GreenGem_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::GoldenDagger].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::GoldenDagger_OnAquired,
		&InventoryItemBehavior::GoldenDagger_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::Necklace].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::Necklace_OnAquired,
		&InventoryItemBehavior::Necklace_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::GoldenCrown].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::GoldenCrown_OnAquired,
		&InventoryItemBehavior::GoldenCrown_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::StrongSniperRifle].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::StrongSniperRifle_OnAquired,
		&InventoryItemBehavior::StrongSniperRifle_OnRemoved
	);

	InventoryItemInfo[EInventoryItem::RottenPumpkin].SetBehaviorFunctionPointers(
		&InventoryItemBehavior::RottenPumpkin_OnAquired,
		&InventoryItemBehavior::RottenPumpkin_OnRemoved
	);

	//===========================================================================================
	//	Mandatory stuff - do not touch
	//===========================================================================================

	for (auto & ItemInfo : InventoryItemInfo)
	{
		/* Set the item type on the value part of the key/value pair. Can actually do 
		this in post edit instead */
		ItemInfo.Value.SetItemType(ItemInfo.Key);

#if !UE_BUILD_SHIPPING
		// Check if a mesh has been set
		if (ItemInfo.Value.GetMeshInfo().IsMeshSet() == false)
		{
			static uint64 WarningCount = 0;
			/* This warning will be annoying. We'll just do it like 10 times max. Perhaps 
			it would be a good idea to have a variable in DevelopmentStatics like 
			ENABLE_FINAL_CHECKS. It is intended to be changed to true right before deciding 
			to package project for shipping or possibly even development and debug. */
			if (WarningCount++ < 10)
			{
				UE_LOG(RTSLOG, Warning, TEXT("Inventory item \"%s\" does not have a mesh set for "
					"it. Make sure to set one before packaging for shipping."),
					TO_STRING(EInventoryItem, ItemInfo.Key));
			}
		}
#endif

		/* Check if the ingredients are valid. If only one item is an ingredient for this item
		then that is not valid */
		if (ItemInfo.Value.GetIngredients().Num() == 1)
		{
			for (const auto & IngredientPair : ItemInfo.Value.GetIngredients())
			{
				if (IngredientPair.Value.GetInteger() == 1)
				{
					// Might get a crash here because we're iterating
					ItemInfo.Value.ClearIngredients();

					UE_LOG(RTSLOG, Warning, TEXT("Inventory item \"%s\" is made from only 1 item. "
						"This is not valid. The item's ingredients array has been emptied as a result "
						"of this."), TO_STRING(EInventoryItem, ItemInfo.Key));

					/* Still possible to obtain the item in game, but it cannot be created from
					combining items since its ingredients array is now empty */

					continue;
				}
			}
		}

		for (const auto & IngredientPair : ItemInfo.Value.GetIngredients())
		{
			InventoryItemInfo[IngredientPair.Key].AddCombinationResult(ItemInfo.Key);
		}

		/* Set the use ability pointer */
		if (ItemInfo.Value.GetUseAbilityType() != EContextButton::None
			&& ItemInfo.Value.IsUsable())
		{
			ItemInfo.Value.SetUseAbilityInfo(&GetContextInfo(ItemInfo.Value.GetUseAbilityType()));
		}
	}

	//	End mandatory stuff
	//============================================================================================
}

void URTSGameInstance::InitKeyInfo()
{
	/* The 8 for loops are similar. I have just hoisted some if statements out */
	if (KeyMappings_bForceUsePartialBrushes)
	{
		if (KeyMappings_BrushOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
		{
			if (KeyMappings_FontOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetImageDisplayMethod(EInputKeyDisplayMethod::ImageAndAddTextAtRuntime);
					Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
				}
			}
			else // Assumed UseDefaultIfNotSet
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetImageDisplayMethod(EInputKeyDisplayMethod::ImageAndAddTextAtRuntime);
					Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					if (Pair.Value.GetPartialBrushTextFont().FontObject == nullptr)
					{
						Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
					}
				}
			}
		}
		else // Assumed UseDefaultIfNotSet
		{
			if (KeyMappings_FontOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetImageDisplayMethod(EInputKeyDisplayMethod::ImageAndAddTextAtRuntime);
					if (Pair.Value.GetPartialBrush().GetResourceObject() == nullptr)
					{
						Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					}
					Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
				}
			}
			else // Assumed UseDefaultIfFontNotSet
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetImageDisplayMethod(EInputKeyDisplayMethod::ImageAndAddTextAtRuntime);
					if (Pair.Value.GetPartialBrush().GetResourceObject() == nullptr)
					{
						Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					}
					if (Pair.Value.GetPartialBrushTextFont().FontObject == nullptr)
					{
						Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
					}
				}
			}
		}
	}
	else
	{
		if (KeyMappings_BrushOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
		{
			if (KeyMappings_FontOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
				}
			}
			else // Assumed UseDefaultIfNotSet
			{
				for (auto & Pair : KeyInfo)
				{
					Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					if (Pair.Value.GetPartialBrushTextFont().FontObject == nullptr)
					{
						Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
					}
				}
			}
		}
		else // Assumed UseDefaultIfNotSet
		{
			if (KeyMappings_FontOverrideMode == EPropertyOverrideMode::AlwaysUseDefault)
			{
				for (auto & Pair : KeyInfo)
				{
					if (Pair.Value.GetPartialBrush().GetResourceObject() == nullptr)
					{
						Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					}
					Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
				}
			}
			else // Assumed UseDefaultIfFontNotSet
			{
				for (auto & Pair : KeyInfo)
				{
					if (Pair.Value.GetPartialBrush().GetResourceObject() == nullptr)
					{
						Pair.Value.SetPartialBrush(KeyMappings_DefaultBrush);
					}
					if (Pair.Value.GetPartialBrushTextFont().FontObject == nullptr)
					{
						Pair.Value.SetPartialFont(KeyMappings_DefaultFont);
					}
				}
			}
		}
	}
}

void URTSGameInstance::CreateHeavyTaskManager()
{
	HeavyTaskManager = NewObject<UHeavyTaskManager>(this);
}

void URTSGameInstance::InitMouseCursorInfo()
{
	for (auto & Pair : MouseCursors)
	{
		Pair.Value.SetFullPath();
	}

	MenuMouseCursor_Info = GetMouseCursorInfo(MenuMouseCursor);
	DefaultAbilityCursor_Default_Info = GetMouseCursorInfo(DefaultAbilityCursor_Default);
	DefaultAbilityCursor_AcceptableTarget_Info = GetMouseCursorInfo(DefaultAbilityCursor_AcceptableTarget);
	DefaultAbilityCursor_UnacceptableTarget_Info = GetMouseCursorInfo(DefaultAbilityCursor_UnacceptableTarget);

	MenuMouseCursor_Info.SetFullPath();
	DefaultAbilityCursor_Default_Info.SetFullPath();
	DefaultAbilityCursor_AcceptableTarget_Info.SetFullPath();
	DefaultAbilityCursor_UnacceptableTarget_Info.SetFullPath();
}

void URTSGameInstance::SetupDefeatConditionInfo()
{
	DefeatConditions.Reserve(Statics::NUM_DEFEAT_CONDITIONS);
	for (uint8 i = 0; i < Statics::NUM_DEFEAT_CONDITIONS; ++i)
	{
		DefeatConditions.Emplace(Statics::ArrayIndexToDefeatCondition(i), FDefeatConditionInfo());
	}
}

void URTSGameInstance::SetupMapPool()
{
	/* Do the following to each map in the map pool:
	1. Give it a unique uint8 ID
	2. Store all its player start locations
	3. Store its minimap texture */

	uint8 ID = 0;
	for (auto & Elem : MapPool)
	{
		MapPoolIDs.Emplace(ID, Elem.Key);
		
		FMapInfo & MapInfo = Elem.Value;

		// Store key in value - will come in handy later
		MapInfo.SetMapFName(Elem.Key);

		// Give each map info a unique ID
		MapInfo.SetUniqueID(ID++);

		UE_CLOG(MapInfo.GetLevelVolumeBP() == nullptr, RTSLOG, Fatal, TEXT("No level volume BP set "
			"for map %s in game instance BP, cannot show minimap for it in menus"), 
			*Elem.Key.ToString());

		// Spawn one of these to extract info from it
		ARTSLevelVolume * LevelVolume = GetWorld()->SpawnActor<ARTSLevelVolume>(MapInfo.GetLevelVolumeBP(),
			Statics::INFO_ACTOR_SPAWN_LOCATION, FRotator::ZeroRotator);

		// Map bounds
		MapInfo.SetMapBounds(LevelVolume->GetMapBounds(), LevelVolume->GetActorRotation().Yaw);
		
		// Player starts
		MapInfo.SetPlayerStartPoints(LevelVolume->GetPlayerStarts());
		
		// Minimap texture
		MapInfo.SetMinimapTexture(LevelVolume->GetMinimapTexture());

		// No longer needed
		LevelVolume->Destroy();
	}
}

void URTSGameInstance::SetupLoadingScreenMessages()
{
	LoadingScreenMessages.Reserve(Statics::NUM_MATCH_LOADING_STATUSES);

	LoadingScreenMessages.Emplace(ELoadingStatus::LoadingPersistentMap,
		NSLOCTEXT("Loading Screen", "Loading Persistent Map", "Loading persistent map"));

	LoadingScreenMessages.Emplace(ELoadingStatus::WaitingForAllPlayersToConnect,
		NSLOCTEXT("Loading Screen", "Waiting For Connections", "Waiting for all players to connect"));

	LoadingScreenMessages.Emplace(ELoadingStatus::WaitingForPlayerControllerClientSetupForMatchAcknowledgementFromAllPlayers,
		NSLOCTEXT("Loading Screen", "Waiting For PC SetupForMatch", "Waiting for all players to setup their player controllers for match"));

	LoadingScreenMessages.Emplace(ELoadingStatus::WaitingForAllPlayersToStreamInLevel,
		NSLOCTEXT("Loading Screen", "Streaming Level", " Waiting for all players to stream in map"));

	LoadingScreenMessages.Emplace(ELoadingStatus::WaitingForInitialValuesAcknowledgementFromAllPlayers,
		NSLOCTEXT("Loading Screen", "Setting Inital Values", "Waiting for all player states to set initial values"));

	LoadingScreenMessages.Emplace(ELoadingStatus::WaitingForFinalSetupAcks,
		NSLOCTEXT("Loading Screen", "Spawning CPU Players", "Waiting for final setup acknowledgement from all players"));

	LoadingScreenMessages.Emplace(ELoadingStatus::SpawningStartingSelectables,
		NSLOCTEXT("Loading Screen", "Spawning Starting Selectables", "Spawning starting buildings and units"));

	LoadingScreenMessages.Emplace(ELoadingStatus::ShowingBlackScreenRightBeforeMatchStart,
		NSLOCTEXT("Loading Screen", "Blank Screen", "Loading complete"));
}

void URTSGameInstance::SetupCommonContextActions()
{
	/* Add extra definitions here to safeguard against losing definitions in editor from C++
	changes. 
	We use AAbilityBase::StaticClass() as the effect actor just because its IsUsable checks 
	return true. Otherwise we will get crashes when trying to use those abilities. Another 
	option is to highjack them just like BuildBuilding is in PC::OnContextButtonPress. That 
	way we avoid having to spawn an AAbilityBase actor */

	FContextButtonInfo AttackMoveButton = FContextButtonInfo(
		FText::FromString("Attack Move"),
		EContextButton::AttackMove,
		false,
		true, 
		false,
		false,
		true,
		EAnimation::None,
		EAbilityMouseAppearance::NoChange,
		false, true, true, false,
		0.f,
		0.f,
		nullptr,
		0.f,
		AAbilityBase::StaticClass()
	);
	ContextActionsMap.Emplace(EContextButton::AttackMove, AttackMoveButton);

	FContextButtonInfo HoldPositionButton = FContextButtonInfo(
		FText::FromString("Hold Position"),
		EContextButton::HoldPosition,
		true,
		true, 
		false, 
		false,
		true,
		EAnimation::None,
		EAbilityMouseAppearance::NoChange,
		false, false, false, false,
		0.f,
		0.f,
		nullptr,
		0.f,
		AAbilityBase::StaticClass()
	);
	ContextActionsMap.Emplace(EContextButton::HoldPosition, HoldPositionButton);

	FContextButtonInfo BuildBuildingButton = FContextButtonInfo(
		FText::FromString("Should not be seen in game"),
		EContextButton::BuildBuilding,
		true,
		false, 
		false,
		false,
		true,
		EAnimation::None,
		EAbilityMouseAppearance::NoChange,
		false, false, false, false,
		0.f,
		0.f,
		nullptr,
		-1.f,
		nullptr
	);
	ContextActionsMap.Emplace(EContextButton::BuildBuilding, BuildBuildingButton);

	FContextButtonInfo TrainButton = FContextButtonInfo(
		FText::FromString("Should not be seen in game"),
		EContextButton::Train, true, false, false, false, true,
		EAnimation::None, EAbilityMouseAppearance::NoChange,
		false, false, false, false,
		0.f, 0.f, nullptr, 0.f, AAbilityBase::StaticClass()
	);
	ContextActionsMap.Emplace(EContextButton::Train, TrainButton);

	FContextButtonInfo UpgradeButton = FContextButtonInfo(
		FText::FromString("Should not be seen in game"),
		EContextButton::Upgrade,
		true,
		false, 
		false,
		false,
		true,
		EAnimation::None,
		EAbilityMouseAppearance::NoChange,
		false, false, false, false,
		0.f,
		0.f,
		nullptr,
		0.f,
		AAbilityBase::StaticClass()
	);
	ContextActionsMap.Emplace(EContextButton::Upgrade, UpgradeButton);
}

void URTSGameInstance::SetupCustomContextActions()
{
	FContextButtonInfo ArtilleryStrikeButton = FContextButtonInfo(
		FText::FromString("Artillery Strike"),
		EContextButton::ArtilleryStrike,
		false,
		false, 
		false,
		true,
		true,
		EAnimation::ContextAction_1,
		EAbilityMouseAppearance::HideAndShowDecal,
		false, false, false, false,
		5.f,
		750.f,
		nullptr,
		300.f, 
		nullptr
	);
	ContextActionsMap.Emplace(EContextButton::ArtilleryStrike, ArtilleryStrikeButton);

	FContextButtonInfo HealButton = FContextButtonInfo(
		FText::FromString("Heal"),
		EContextButton::Heal,
		false,
		false, 
		false,
		true,
		true,
		EAnimation::ContextAction_1,
		EAbilityMouseAppearance::CustomMouseCursor,
		true, false, true, false,
		8.f,
		500.f,
		nullptr,
		0.f,
		nullptr
	);
	ContextActionsMap.Emplace(EContextButton::Heal, HealButton);

	FContextButtonInfo IceBarrierButton = FContextButtonInfo(
		FText::FromString("Ice Barrier"),
		EContextButton::IceBarrier,
		true, true, false, true, true, EAnimation::ContextAction_1,
		EAbilityMouseAppearance::NoChange, false, false, false, false,
		2.f, 0.f, nullptr, 300.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::IceBarrier, IceBarrierButton);

	FContextButtonInfo DashButton = FContextButtonInfo(
		FText::FromString("Dash"),
		EContextButton::Dash,
		true, true, false, false, false, EAnimation::None, EAbilityMouseAppearance::NoChange,
		false, false, false, true, 10.f, 0.f, nullptr, 0.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::Dash, DashButton);

	FContextButtonInfo HasteButton = FContextButtonInfo(
		FText::FromString("Haste"),
		EContextButton::Haste, 
		true, false, false, false, false, EAnimation::None, EAbilityMouseAppearance::NoChange,
		true, false, true, true, 8.f, 400.f, nullptr, 0.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::Haste, HasteButton);

	FContextButtonInfo CleanseButton = FContextButtonInfo(
		FText::FromString("Cleanse"),
		EContextButton::Cleanse,
		false, false, false, true, false, EAnimation::None, EAbilityMouseAppearance::CustomMouseCursor,
		true, false, true, true, 6.f, 800.f, nullptr, 0.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::Cleanse, CleanseButton);

	FContextButtonInfo CorruptionButton = FContextButtonInfo(
		FText::FromString("Corruption"), 
		EContextButton::Corruption, 
		false, false, true, true, false, EAnimation::None, EAbilityMouseAppearance::CustomMouseCursor,
		true, true, true, true, 5.f, 1000.f, nullptr, 0.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::Corruption, CorruptionButton);

	FContextButtonInfo StealthButton = FContextButtonInfo(
		FText::FromString("Stealth"),
		EContextButton::StealthSelf,
		true, true, false, false, false, EAnimation::None, EAbilityMouseAppearance::NoChange,
		false, false, false, true, 10.f, 0.f, nullptr, 0.f, nullptr
	);
	ContextActionsMap.Emplace(EContextButton::StealthSelf, StealthButton);
}

void URTSGameInstance::SetupMissingContextActions()
{
	for (uint8 i = 0; i < Statics::NUM_CONTEXT_ACTIONS; ++i)
	{
		const EContextButton AbilityType = Statics::ArrayIndexToContextButton(i);
		
		/* Only add if user has not already defined it */
		if (!ContextActionsMap.Contains(AbilityType))
		{
			ContextActionsMap.Emplace(AbilityType, FContextButtonInfo(AbilityType));
		}
	}
}

void URTSGameInstance::SetupCPUInfo()
{
	for (uint8 i = 0; i < Statics::NUM_CPU_DIFFICULTIES; ++i)
	{
		const ECPUDifficulty Difficulty = Statics::ArrayIndexToCPUDifficulty(i);
		CPUPlayerInfo.Emplace(Difficulty, FCPUDifficultyInfo(ENUM_TO_STRING(ECPUDifficulty, Difficulty)));
	}
}

void URTSGameInstance::SetupMatchWidgets_BP()
{
	for (uint8 i = 0; i < Statics::NUM_MATCH_WIDGET_TYPES; ++i)
	{
		MatchWidgets_BP.Emplace(Statics::ArrayIndexToMatchWidgetType(i), nullptr);
	}
}

void URTSGameInstance::SetDefaultHUDMessages()
{
	// Warning messages. Should really rename that TMap 
	HUDMessages.Reserve(Statics::NUM_GENERIC_GAME_WARNING_TYPES);
	for (uint8 i = 0; i < Statics::NUM_GENERIC_GAME_WARNING_TYPES; ++i)
	{
		HUDMessages.Emplace(Statics::ArrayIndexToGameWarning(i),
			FText::FromString(ENUM_TO_STRING(EGameWarning, Statics::ArrayIndexToGameWarning(i))));
	}

	// More warning messages for abilities
	AbilityHUDMessages.Reserve(Statics::NUM_CUSTOM_ABILITY_CHECK_TYPES);
	for (uint8 i = 0; i < Statics::NUM_CUSTOM_ABILITY_CHECK_TYPES; ++i)
	{
		AbilityHUDMessages.Emplace(Statics::ArrayIndexToAbilityRequirement(i),
			FText::FromString(ENUM_TO_STRING(EAbilityRequirement, Statics::ArrayIndexToAbilityRequirement(i))));
	}

	// More warning messages for 'not enough resources'
	MissingResourceHUDMessages.Reserve(Statics::NUM_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		MissingResourceHUDMessages.Emplace(Statics::ArrayIndexToResourceType(i),
			FText::FromString("Not enough " + ENUM_TO_STRING(EResourceType, Statics::ArrayIndexToResourceType(i))));
	}

	// More warning messages for 'not enough housing resources'
	MissingHousingResourceHUDMessages.Reserve(Statics::NUM_HOUSING_RESOURCE_TYPES);
	for (uint8 i = 0; i < Statics::NUM_HOUSING_RESOURCE_TYPES; ++i)
	{
		MissingHousingResourceHUDMessages.Emplace(Statics::ArrayIndexToHousingResourceType(i),
			FText::FromString("Not enough " + ENUM_TO_STRING(EHousingResourceType, Statics::ArrayIndexToHousingResourceType(i))));
	}

	// Notification messages 
	NotificationMessages.Reserve(Statics::NUM_GAME_NOTIFICATION_TYPES);
	for (uint8 i = 0; i < Statics::NUM_GAME_NOTIFICATION_TYPES; ++i)
	{
		NotificationMessages.Emplace(Statics::ArrayIndexToGameNotification(i),
			FText::FromString(ENUM_TO_STRING(EGameNotification, Statics::ArrayIndexToGameNotification(i))));
	}
}

void URTSGameInstance::SetupResourceInfo()
{
	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceInfo.Emplace(Statics::ArrayIndexToResourceType(i), FResourceInfo());
	}

	// Do starting resource TMap too
	StartingResourceConfigs.Reserve(Statics::NUM_STARTING_RESOURCE_AMOUNT_TYPES);
	for (uint8 i = 0; i < Statics::NUM_STARTING_RESOURCE_AMOUNT_TYPES; ++i)
	{
		StartingResourceConfigs.Emplace(Statics::ArrayIndexToStartingResourceAmount(i),
			FStartingResourceConfig());
	}
}

void URTSGameInstance::SetupLeaveOnMapList()
{
	LeaveOnMapList.Emplace(AResourceSpot::StaticClass());
	LeaveOnMapList.Emplace(AInventoryItem::StaticClass());
}

void URTSGameInstance::SetupSelectableResourceInfo()
{
	for (uint8 i = 0; i < Statics::NUM_SELECTABLE_RESOURCE_TYPES; ++i)
	{
		SelectableResourceInfo.Emplace(Statics::ArrayIndexToSelectableResourceType(i), 
			FSelectableResourceColorInfo());
	}
}

void URTSGameInstance::SetupInventoryItemInfo()
{
	InventoryItemInfo.Reserve(Statics::NUM_INVENTORY_ITEM_TYPES);
	for (uint8 i = 0; i < Statics::NUM_INVENTORY_ITEM_TYPES; ++i)
	{
		InventoryItemInfo.Emplace(Statics::ArrayIndexToInventoryItem(i), 
			FInventoryItemInfo(ENUM_TO_STRING(EInventoryItem, Statics::ArrayIndexToInventoryItem(i))));
	}
}

void URTSGameInstance::SetupKeyInfo()
{
	// FKeyDetails::LongDisplayName and FKeyDetails::ShortDisplayName is a thing you know
	
	KeyInfo.Reserve(119);

	/* @param InDisplayName - the full name 
	@param InPartialText - what appears on a button image for this key. This may not be used 
	at all depending on how you choose to show images of keys e.g. for the "Delete" key you 
	may just want to show "DEL" */
	auto AddKey = [&](const FKey & InKey, const FString & InDisplayName, const FString & InPartialText)
	{
		KeyInfo.Emplace(InKey, FKeyInfo(InDisplayName, InPartialText));
	};

	AddKey(EKeys::MouseX, 				"Mouse X", "Mouse X");
	AddKey(EKeys::MouseY,				"Mouse Y", "Mouse Y");
	AddKey(EKeys::MouseScrollUp,		"Mouse Wheel Up", "Mouse Wheel Up");
	AddKey(EKeys::MouseScrollDown,		"Mouse Wheel Down", "Mouse Wheel Down");
	AddKey(EKeys::LeftMouseButton,		"Left Mouse Button", "LMB");
	AddKey(EKeys::RightMouseButton,		"Right Mouse Button", "RMB");
	AddKey(EKeys::MiddleMouseButton,	"Middle Mouse Button", "MMB");
	AddKey(EKeys::BackSpace,			"BackSpace", "BackSpace");
	AddKey(EKeys::Tab,					"TAB", "TAB");
	AddKey(EKeys::Enter,				"Enter", "Enter");
	AddKey(EKeys::Pause,				"Pause", "Pause");
	AddKey(EKeys::CapsLock,				"Caps Lock", "Caps Lock");
	AddKey(EKeys::Escape,				"Escape", "ESC");
	AddKey(EKeys::SpaceBar,				"Space Bar", "Space");
	AddKey(EKeys::PageUp,				"Page Up", "Pg Up");
	AddKey(EKeys::PageDown,				"Page Down", "Pg Dn");
	AddKey(EKeys::End,					"End", "End");
	AddKey(EKeys::Home,					"Home", "Home");
	AddKey(EKeys::Left,					"Left Arrow", "Left");
	AddKey(EKeys::Up,					"Up Arrow", "Up");
	AddKey(EKeys::Right,				"Right Arrow", "Right");
	AddKey(EKeys::Down,					"Down Arrow", "Down");
	AddKey(EKeys::Insert,				"Insert", "INS");
	AddKey(EKeys::Delete,				"Delete", "DEL");
	AddKey(EKeys::Zero,					"Zero", "0");
	AddKey(EKeys::One,					"One", "1");
	AddKey(EKeys::Two,					"Two", "2");
	AddKey(EKeys::Three,				"Three", "3");
	AddKey(EKeys::Four,					"Four", "4");
	AddKey(EKeys::Five,					"Five", "5");
	AddKey(EKeys::Six,					"Six", "6");
	AddKey(EKeys::Seven,				"Seven", "7");
	AddKey(EKeys::Eight,				"Eight", "8");
	AddKey(EKeys::Nine,					"Nine", "9");
	AddKey(EKeys::A,					"A", "A");
	AddKey(EKeys::B,					"B", "B");
	AddKey(EKeys::C,					"C", "C");
	AddKey(EKeys::D,					"D", "D");
	AddKey(EKeys::E,					"E", "E");
	AddKey(EKeys::F,					"F", "F");
	AddKey(EKeys::G,					"G", "G");
	AddKey(EKeys::H,					"H", "H");
	AddKey(EKeys::I,					"I", "I");
	AddKey(EKeys::J,					"J", "J");
	AddKey(EKeys::K,					"K", "K");
	AddKey(EKeys::L,					"L", "L");
	AddKey(EKeys::M,					"M", "M");
	AddKey(EKeys::N,					"N", "N");
	AddKey(EKeys::O,					"O", "O");
	AddKey(EKeys::P,					"P", "P");
	AddKey(EKeys::Q,					"Q", "Q");
	AddKey(EKeys::R,					"R", "R");
	AddKey(EKeys::S,					"S", "S");
	AddKey(EKeys::T,					"T", "T");
	AddKey(EKeys::U,					"U", "U");
	AddKey(EKeys::V,					"V", "V");
	AddKey(EKeys::W,					"W", "W");
	AddKey(EKeys::X,					"X", "X");
	AddKey(EKeys::Y,					"Y", "Y");
	AddKey(EKeys::Z,					"Z", "Z");
	AddKey(EKeys::NumPadZero,			"Num Pad Zero", "Num 0");
	AddKey(EKeys::NumPadOne,			"Num Pad One", "Num 1");
	AddKey(EKeys::NumPadTwo,			"Num Pad Two", "Num 2");
	AddKey(EKeys::NumPadThree,			"Num Pad Three", "Num 3");
	AddKey(EKeys::NumPadFour,			"Num Pad Four", "Num 4");
	AddKey(EKeys::NumPadFive,			"Num Pad Five", "Num 5");
	AddKey(EKeys::NumPadSix,			"Num Pad Six", "Num 6");
	AddKey(EKeys::NumPadSeven,			"Num Pad Seven", "Num 7");
	AddKey(EKeys::NumPadEight,			"Num Pad Eight", "Num 8");
	AddKey(EKeys::NumPadNine,			"Num Pad Nine", "Num 9");
	AddKey(EKeys::Multiply,				"Multiply", "MUL");
	AddKey(EKeys::Add,					"Add", "+");
	AddKey(EKeys::Subtract,				"Subtract", "-");
	AddKey(EKeys::Decimal,				"Decimal", ".");
	AddKey(EKeys::Divide,				"Divide", "DIV");
	AddKey(EKeys::F1,					"F1", "F1");
	AddKey(EKeys::F2,					"F2", "F2");
	AddKey(EKeys::F3,					"F3", "F3");
	AddKey(EKeys::F4,					"F4", "F4");
	AddKey(EKeys::F5,					"F5", "F5");
	AddKey(EKeys::F6,					"F6", "F6");
	AddKey(EKeys::F7,					"F7", "F7");
	AddKey(EKeys::F8,					"F8", "F8");
	AddKey(EKeys::F9,					"F9", "F9");
	AddKey(EKeys::F10,					"F10", "F10");
	AddKey(EKeys::F11,					"F11", "F11");
	AddKey(EKeys::F12,					"F12", "F12");
	AddKey(EKeys::NumLock,				"Num Lock", "NumLock");
	AddKey(EKeys::ScrollLock,			"Scroll Lock", "Scroll Lock");
	AddKey(EKeys::LeftShift,			"Left Shift", "LShift");
	AddKey(EKeys::RightShift,			"Right Shift", "RShift");
	AddKey(EKeys::LeftControl,			"Left Control", "LCTRL");
	AddKey(EKeys::RightControl,			"Right Control", "RCTRL");
	AddKey(EKeys::LeftAlt,				"Left Alt", "LALT");
	AddKey(EKeys::RightAlt,				"Right Alt", "RALT");
	AddKey(EKeys::LeftCommand,			"Left Command", "LCMD");
	AddKey(EKeys::RightCommand,			"Right Command", "RCMD");
	AddKey(EKeys::Semicolon,			"Semicolon", ";");
	AddKey(EKeys::Equals,				"Equals", "=");
	AddKey(EKeys::Comma,				"Comma", ",");
	AddKey(EKeys::Underscore,			"Underscore", "_");
	AddKey(EKeys::Hyphen,				"Hyphen", "-");
	AddKey(EKeys::Period,				"Period", ".");
	AddKey(EKeys::Slash,				"Slash", "/");
	AddKey(EKeys::Tilde,				"Tilde", "`");
	AddKey(EKeys::LeftBracket,			"Left Bracket", "[");
	AddKey(EKeys::Backslash,			"Backslash", "\\");
	AddKey(EKeys::RightBracket,			"Right Bracket", "]");
	AddKey(EKeys::Apostrophe,			"Apostrophe", "'");
	AddKey(EKeys::Ampersand,			"Ampersand", "&");
	AddKey(EKeys::Asterix,				"Asterix", "*");
	AddKey(EKeys::Caret,				"Caret", "^");
	AddKey(EKeys::Colon,				"Colon", ":");
	AddKey(EKeys::Dollar,				"Dollar", "$");
	AddKey(EKeys::Exclamation,			"Exclamation", "!");
	AddKey(EKeys::LeftParantheses,		"Left Parantheses", "(");
	AddKey(EKeys::RightParantheses,		"Right Parantheses", ")");
	AddKey(EKeys::Quote,				"Quote", "\"");
	AddKey(EKeys::Invalid,				"Unbound", "");
}

#if WITH_EDITOR
void URTSGameInstance::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	enum class EEditedProperty : uint8
	{
		InventoryItemInfo, 

		Other
	};

	EEditedProperty EditedProperty = EEditedProperty::Other;

	/* Here determine what property was edited */
	for (const auto & Elem : PropertyChangedEvent.PropertyChain)
	{
		if (Elem->GetFName() == GET_MEMBER_NAME_CHECKED(URTSGameInstance, InventoryItemInfo))
		{
			EditedProperty = EEditedProperty::InventoryItemInfo;
			break;
		}
	}

	if (EditedProperty == EEditedProperty::InventoryItemInfo)
	{
		for (auto & Elem : InventoryItemInfo)
		{
			Elem.Value.OnPostEdit(Elem.Key);
		}

		return; // Probably safe to do this
	}

	for (auto & Elem : ContextActionsMap)
	{
		Elem.Value.OnPostEdit(Elem.Key);
	}

	for (auto & Elem : CommanderAbilitiesTMap)
	{
		Elem.Value.OnPostEdit();
	}

	/* Give elements added to map pool a default unique key */
	static uint64 Num = 0;
	for (auto & Elem : MapPool)
	{
		if (Elem.Key == NAME_None)
		{
			Elem.Key = *("Map name set in editor goes here " + FString::FromInt(Num++));
			break;
		}
	}

	// HUD marquee box options
	bCanEditMarqueeBorderValues = (MarqueeSelectionBoxStyle == EMarqueeBoxDrawMethod::BorderOnly
		|| MarqueeSelectionBoxStyle == EMarqueeBoxDrawMethod::BorderAndFill);
	bCanEditMarqueeFillValues = (MarqueeSelectionBoxStyle == EMarqueeBoxDrawMethod::FilledRectangleOnly 
		|| MarqueeSelectionBoxStyle == EMarqueeBoxDrawMethod::BorderAndFill);

	/* Buff/debuff containers */
	for (auto & Elem : StaticBuffsAndDebuffs)
	{
		Elem.Value.SetSpecificType(Elem.Key);
	}
	for (auto & Elem : TickableBuffsAndDebuffs)
	{
		Elem.Value.SetSpecificType(Elem.Key);
	}

	// Unified HUD asset flags
	UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedHoverImage = UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMouseHoverImage().UsingImage();
	UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedPressedImage = UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMousePressImage().UsingImage();
	UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedHoverSound = UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMouseHoverSound().UsingSound();
	UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedLMBPressSound = UnifiedImagesAndSounds_ActionBarETC.GetUnifiedPressedByLMBSound().UsingSound();
	UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedRMBPressSound = UnifiedImagesAndSounds_ActionBarETC.GetUnifiedPressedByRMBSound().UsingSound();
	UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedHoverImage = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverImage().UsingImage();
	UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedPressedImage = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMousePressImage().UsingImage();
	UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedHoverSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverSound().UsingSound();
	UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedLMBPressSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByLMBSound().UsingSound();
	UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedRMBPressSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByRMBSound().UsingSound();
	UnifiedButtonAssetFlags_Shop.bUsingUnifiedHoverImage = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverImage().UsingImage();
	UnifiedButtonAssetFlags_Shop.bUsingUnifiedPressedImage = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMousePressImage().UsingImage();
	UnifiedButtonAssetFlags_Shop.bUsingUnifiedHoverSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverSound().UsingSound();
	UnifiedButtonAssetFlags_Shop.bUsingUnifiedLMBPressSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByLMBSound().UsingSound();
	UnifiedButtonAssetFlags_Shop.bUsingUnifiedRMBPressSound = UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByRMBSound().UsingSound();
	UnifiedButtonAssetFlags_Menus.bUsingUnifiedHoverImage = UnifiedImagesAndSounds_Menus.GetUnifiedMouseHoverImage().UsingImage();
	UnifiedButtonAssetFlags_Menus.bUsingUnifiedPressedImage = UnifiedImagesAndSounds_Menus.GetUnifiedMousePressImage().UsingImage();
	UnifiedButtonAssetFlags_Menus.bUsingUnifiedHoverSound = UnifiedImagesAndSounds_Menus.GetUnifiedMouseHoverSound().UsingSound();
	UnifiedButtonAssetFlags_Menus.bUsingUnifiedLMBPressSound = UnifiedImagesAndSounds_Menus.GetUnifiedPressedByLMBSound().UsingSound();
	UnifiedButtonAssetFlags_Menus.bUsingUnifiedRMBPressSound = UnifiedImagesAndSounds_Menus.GetUnifiedPressedByRMBSound().UsingSound();
}
#endif

void URTSGameInstance::Initialize()
{
	if (bHasInitialized && bHasSpawnedInfoActors)
	{
		return;
	}
	
	if (!bHasInitialized)
	{
		/* Doing this now before anything else just because I really don't want spawned
		selectables to have any chance of playing sounds not at the right volume level */
		SetupInitialSettings();
	}

	if (!bHasSpawnedInfoActors)
	{
		bHasSpawnedInfoActors = true;

		CreatePoolingManager();
		InitDevelopmentSettings();
		InitFactionInfo();
		/* Create this on every map change cause it's containers will likely be full of nulls 
		after the map has been torn down. The old one will be GCed */
		CreateHeavyTaskManager();
	}

	if (!bHasInitialized)
	{
		bHasInitialized = true;

		InitContextActions();
		InitCommanderAbilityInfo();
		InitCommanderSkillTreeNodeInfo();
		InitBuildingTargetingAbilityInfo();
		SetupBuffsAndDebuffInfos();
		SetupMapPool();
		InitMouseCursorInfo();
		SetupAudioComponents();
		InitInventoryItemInfo();
		InitKeyInfo();
	}

	/* Spawned a lot of actors that were destroyed soon after. Good idea to call this now */
	GEngine->ForceGarbageCollection(true);
}

void URTSGameInstance::OnMapChange()
{
	assert(bHasInitialized);

	if (!bHasSpawnedInfoActors)
	{
		bHasSpawnedInfoActors = true;

		CreatePoolingManager();
		InitDevelopmentSettings();
		InitFactionInfo();
	}
}

void URTSGameInstance::Shutdown()
{
	Super::Shutdown();

	/* Stop all looping audio components cause audio keeps playing when quitting PIE */
	if (MusicAudioComp != nullptr)
	{
		MusicAudioComp->Stop();
	}

#if WITH_EDITOR
	PoolingManager->LogSuggestProjectileVelocityFails();
	PoolingManager->LogWorstProjectileFrame();
#endif
}

const TArray<AFactionInfo*>& URTSGameInstance::GetAllFactionInfo() const
{
	return FactionInfo;
}

AFactionInfo * URTSGameInstance::GetFactionInfo(EFaction Faction) const
{
	const uint8 Index = Statics::FactionToArrayIndex(Faction);
	assert(FactionInfo[Index] != nullptr);

	return FactionInfo[Index];
}

AFactionInfo * URTSGameInstance::GetRandomFactionInfo() const
{
	/* Should not call if no faction info */
	assert(FactionInfo.Num() > 0);

	const int32 RandomIndex = FMath::RandRange(0, FactionInfo.Num() - 1);

	return FactionInfo[RandomIndex];
}

EFaction URTSGameInstance::GetRandomFaction() const
{
	return GetRandomFactionInfo()->GetFaction();
}

const FContextButtonInfo & URTSGameInstance::GetContextInfo(EContextButton ButtonType) const
{
	UE_CLOG(ButtonType == EContextButton::None, RTSLOG, Fatal, TEXT("Trying to get ability info "
		"for \"None\" type"));
	
	const uint8 Index = Statics::ContextButtonToArrayIndex(ButtonType);

	UE_CLOG(ContextActions.IsValidIndex(Index) == false, RTSLOG, Fatal, TEXT("Ability [%s] causing "
		"array index out of bounds"), TO_STRING(EContextButton, ButtonType));

	return ContextActions[Index];
}

const FCommanderAbilityInfo & URTSGameInstance::GetCommanderAbilityInfo(ECommanderAbility AbilityType) const
{
	const int32 Index = Statics::CommanderAbilityTypeToArrayIndex(AbilityType);
	return CommanderAbilities[Index];
}

const FCommanderAbilityTreeNodeInfo & URTSGameInstance::GetCommanderSkillTreeNodeInfo(ECommanderSkillTreeNodeType NodeType) const
{
	const int32 Index = Statics::CommanderSkillTreeNodeToArrayIndex(NodeType);
	return CommanderSkillTreeNodes[Index];
}

const TArray<FCommanderAbilityInfo>& URTSGameInstance::GetAllCommanderAbilities() const
{
	return CommanderAbilities;
}

const FBuildingTargetingAbilityStaticInfo & URTSGameInstance::GetBuildingTargetingAbilityInfo(EBuildingTargetingAbility AbilityType) const
{
	return BuildingTargetingAbilities[AbilityType];
}

const FResourceInfo & URTSGameInstance::GetResourceInfo(EResourceType ResourceType) const
{
	return ResourceInfo[ResourceType];
}

const FStartingResourceConfig & URTSGameInstance::GetStartingResourceConfig(EStartingResourceAmount Amount) const
{
#if WITH_EDITOR 
	
	if (Amount == EStartingResourceAmount::DevelopmentSettings)
	{ 
		return EditorPlaySession_GetStartingResourceConfig();
	}
	else
	{
		return StartingResourceConfigs[Amount];
	}

#else // (or dev settings not available)
	
	return StartingResourceConfigs[Amount];

#endif // !WITH_EDITOR 
}

const TMap<EStartingResourceAmount, FStartingResourceConfig>& URTSGameInstance::GetAllStartingResourceConfigs() const
{
	return StartingResourceConfigs;
}

AObjectPoolingManager * URTSGameInstance::GetPoolingManager() const
{
	// Just because I think anyone calling this expects a non null
	assert(PoolingManager != nullptr);
	return PoolingManager;
}

UHeavyTaskManager * URTSGameInstance::GetHeavyTaskManager() const
{
	return HeavyTaskManager;
}

float URTSGameInstance::GetDamageMultiplier(TSubclassOf <UDamageType> DamageType, EArmourType ArmourType) const
{
	/* If crash here then no entry for DamageType has been added to DamageMultipliers */
	return DamageMultipliers[DamageType].GetMultiplier(ArmourType);
}

const FBuildingNetworkAttributes & URTSGameInstance::GetBuildingNetworkInfo(EBuildingNetworkType NetworkType) const
{
	return BuildingNetworkInfo[NetworkType];
}

float URTSGameInstance::GetExperienceBountyMultiplierPerLevel() const
{
	return ExperienceBountyMultiplierPerLevel;
}

bool URTSGameInstance::IsInitializingFactionInfo() const
{
	return bIsInitializingFactionInfo;
}

const TArray<FContextButtonInfo>& URTSGameInstance::GetAllContextInfo() const
{
	return ContextActions;
}

const FDefeatConditionInfo & URTSGameInstance::GetDefeatConditionInfo(EDefeatCondition Condition) const
{
	return DefeatConditions[Condition]; 
}

const TMap<EDefeatCondition, FDefeatConditionInfo>& URTSGameInstance::GetAllDefeatConditions() const
{
	return DefeatConditions;
}

const TMap <FName, FMapInfo>& URTSGameInstance::GetMapPool() const
{
	return MapPool;
}

const FMapInfo & URTSGameInstance::GetMapInfo(const FName & MapName) const
{
	UE_CLOG(!MapPool.Contains(MapName), RTSLOG, Fatal, TEXT("Game instance does not contain entry "
		"for map [%s]. Add an entry for it in MapPool."), *MapName.ToString());
	
	return MapPool[MapName];
}

const FMapInfo & URTSGameInstance::GetMapInfo(FMapID MapID) const
{
	UE_CLOG(!MapPoolIDs.Contains(MapID), RTSLOG, Fatal, TEXT("Request for map info but map is "
		"not in map pool, likely that game instance BP does not have every map in its map pool"));
	
	const FName & MapName = MapPoolIDs[MapID];

	return GetMapInfo(MapName);
}

bool URTSGameInstance::IsLocationInsideMapBounds(const FVector & Location) const
{
	/* Wrote this func not even knowing 100% what axis-aligned means. I hope I got it right */

	const FMapInfo & CurrentMapInfo = GetMapInfo(MatchInfo.GetMapUniqueID());

	/* Rotate the world bounds so they are axis aligned */
	const FBoxSphereBounds & MapBounds = CurrentMapInfo.GetMapBounds();
	const FVector RotatedPoint = FRotator(0.f, CurrentMapInfo.GetMapBoundsYawRotation(), 0.f).RotateVector(Location);
	const FVector AxisAlignedOrigin = FRotator(0.f, CurrentMapInfo.GetMapBoundsYawRotation(), 0.f).RotateVector(MapBounds.Origin);

	return FMath::PointBoxIntersection(RotatedPoint, FBox(AxisAlignedOrigin - MapBounds.BoxExtent, AxisAlignedOrigin + MapBounds.BoxExtent));
}

const FCursorInfo & URTSGameInstance::GetMouseCursorInfo(EMouseCursorType MouseCursorType) const
{
	UE_CLOG(MouseCursorType == EMouseCursorType::None, RTSLOG, Fatal, TEXT("Trying to get cursor "
		"info for default value. Need to change default value to one of your own custom enum values"));
	
	UE_CLOG(MouseCursors.Contains(MouseCursorType) == false, RTSLOG, Fatal, TEXT("No entry in game "
		"instance for mouse cursor type [%s] "), TO_STRING(EMouseCursorType, MouseCursorType));

	return MouseCursors[MouseCursorType];
}

EMouseCursorType URTSGameInstance::GetMatchMouseCursor() const
{
	return MatchMouseCursor;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_Top() const
{
	return EdgeScrollingCursor_Top;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_TopRight() const
{
	return EdgeScrollingCursor_TopRight;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_Right() const
{
	return EdgeScrollingCursor_Right;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_BottomRight() const
{
	return EdgeScrollingCursor_BottomRight;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_Bottom() const
{
	return EdgeScrollingCursor_Bottom;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_BottomLeft() const
{
	return EdgeScrollingCursor_BottomLeft;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_Left() const
{
	return EdgeScrollingCursor_Left;
}

EMouseCursorType URTSGameInstance::GetEdgeScrollingCursor_TopLeft() const
{
	return EdgeScrollingCursor_TopLeft;
}

EMouseCursorType URTSGameInstance::GetAbilityDefaultMouseCursor_Default() const
{
	return DefaultAbilityCursor_Default;
}

EMouseCursorType URTSGameInstance::GetAbilityDefaultMouseCursor_AcceptableTarget() const
{
	return DefaultAbilityCursor_AcceptableTarget;
}

EMouseCursorType URTSGameInstance::GetAbilityDefaultMouseCursor_UnacceptableTarget() const
{
	return DefaultAbilityCursor_UnacceptableTarget;
}

USoundMix * URTSGameInstance::GetSoundMix() const
{
	assert(SoundMix != nullptr);
	return SoundMix;
}

const TMap<FString, USoundClass*>& URTSGameInstance::GetSoundClasses() const
{
	return SoundClasses;
}

USoundClass * URTSGameInstance::GetSoundClass(const FString & SoundClassName) const
{
	return SoundClasses[SoundClassName];
}

const FStaticBuffOrDebuffInfo * URTSGameInstance::GetBuffOrDebuffInfo(EStaticBuffAndDebuffType Type) const
{
	return &StaticBuffsAndDebuffs[Type];
}

const FTickableBuffOrDebuffInfo * URTSGameInstance::GetBuffOrDebuffInfo(ETickableBuffAndDebuffType Type) const
{
	return &TickableBuffsAndDebuffs[Type];
}

const FBuffOrDebuffSubTypeInfo & URTSGameInstance::GetBuffOrDebuffSubTypeInfo(EBuffAndDebuffSubType SubType) const
{
	return BuffAndDebuffSubTypeInfo[SubType];
}

const FSelectableResourceColorInfo & URTSGameInstance::GetSelectableResourceInfo(ESelectableResourceType SelectableResourceType) const
{
	return SelectableResourceInfo[SelectableResourceType];
}

const FInventoryItemInfo * URTSGameInstance::GetInventoryItemInfo(EInventoryItem ItemType) const
{
	UE_CLOG(ItemType == EInventoryItem::None, RTSLOG, Fatal, TEXT("Trying to get inventory item "
		"info for \"None\" "));
	
	return &InventoryItemInfo[ItemType];
}

EInventoryItem URTSGameInstance::GetRandomInventoryItem() const
{
	if (Statics::NUM_INVENTORY_ITEM_TYPES > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, Statics::NUM_INVENTORY_ITEM_TYPES - 1);
		return Statics::ArrayIndexToInventoryItem(RandomIndex);
	}
	else
	{
		return EInventoryItem::None;
	}
}

const FSlateBrush & URTSGameInstance::GetEmptyInventorySlotImage_Normal() const
{
	return EmptyInventorySlotImage;
}

const FSlateBrush & URTSGameInstance::GetEmptyInventorySlotImage_Hovered() const
{
	return EmptyHoveredInventorySlotImage;
}

const FSlateBrush & URTSGameInstance::GetEmptyInventorySlotImage_Pressed() const
{
	return EmptyPressedInventorySlotImage;
}

USoundBase * URTSGameInstance::GetEmptyInventorySlotSound_Hovered() const
{
	return EmptyInventorySlotHoverSound;
}

USoundBase * URTSGameInstance::GetEmptyInventorySlotSound_PressedByLMB() const
{
	return EmptyInventorySlotPressedByLMBSound;
}

USoundBase * URTSGameInstance::GetEmptyInventorySlotSound_PressedByRMB() const
{
	return EmptyInventorySlotPressedByRMBSound;
}

#if !UE_BUILD_SHIPPING
bool URTSGameInstance::IsBuff(EStaticBuffAndDebuffType Type) const
{
	return GetBuffOrDebuffInfo(Type)->IsBuff();
}

bool URTSGameInstance::IsBuff(ETickableBuffAndDebuffType Type) const
{
	return GetBuffOrDebuffInfo(Type)->IsBuff();
}

bool URTSGameInstance::IsDebuff(EStaticBuffAndDebuffType Type) const
{
	return GetBuffOrDebuffInfo(Type)->IsDebuff();
}

bool URTSGameInstance::IsDebuff(ETickableBuffAndDebuffType Type) const
{
	return GetBuffOrDebuffInfo(Type)->IsDebuff();
}
#endif // !UE_BUILD_SHIPPING

const FCursorInfo & URTSGameInstance::GetMenuCursorInfo() const
{
	return MenuMouseCursor_Info;
}

const FSelectionDecalInfo & URTSGameInstance::GetObserverSelectionDecal() const
{
	return ObserverSelectionDecal;
}

const TSubclassOf<UWorldWidget> & URTSGameInstance::GetObserverSelectableSelectionWorldWidget() const
{
	return ObserverSelectableSelectionWorldWidget;
}

const TSubclassOf<UWorldWidget> & URTSGameInstance::GetObserverSelectablePersistentWorldWidget() const
{
	return ObserverSelectablePersistentWorldWidget;
}

const TSubclassOf<UWorldWidget>& URTSGameInstance::GetObserverResourceSpotSelectionWorldWidget() const
{
	return ObserverResourceSpotSelectionWorldWidget;
}

const TSubclassOf<UWorldWidget>& URTSGameInstance::GetObserverResourceSpotPersistentWorldWidget() const
{
	return ObserverResourceSpotPersistentWorldWidget;
}

EWorldWidgetViewMode URTSGameInstance::GetWorldWidgetViewMode() const
{
	return WorldWidgetViewMode;
}

const FString & URTSGameInstance::GetCPUDifficulty(ECPUDifficulty AsEnum) const
{
	return CPUPlayerInfo[AsEnum].GetName();
}

const TMap<ECPUDifficulty, FCPUDifficultyInfo>& URTSGameInstance::GetCPUDifficultyInfo() const
{
	return CPUPlayerInfo;
}

bool URTSGameInstance::IsMatchWidgetBlueprintSet(EMatchWidgetType WidgetType) const
{
	return MatchWidgets_BP.Contains(WidgetType) && MatchWidgets_BP[WidgetType] != nullptr;
}

const TSubclassOf<UInGameWidgetBase>& URTSGameInstance::GetMatchWidgetBP(EMatchWidgetType WidgetType) const
{
	return MatchWidgets_BP[WidgetType];
}

const FGameNotificationInfo & URTSGameInstance::GetHUDNotificationMessageInfo(EGameNotification NotificationType) const
{
	return NotificationMessages[NotificationType];
}

const FGameWarningInfo & URTSGameInstance::GetGameWarningInfo(EGameWarning MessageType) const
{
	assert(MessageType != EGameWarning::None);

	return HUDMessages[MessageType];
}

const FGameWarningInfo & URTSGameInstance::GetGameWarningInfo(EAbilityRequirement ReasonForMessage) const
{
	assert(ReasonForMessage != EAbilityRequirement::Uninitialized && ReasonForMessage != EAbilityRequirement::NoMissingRequirement);
	
	return AbilityHUDMessages[ReasonForMessage];
}

const FGameWarningInfo & URTSGameInstance::GetGameWarningInfo(EResourceType ResourceType) const
{
	return MissingResourceHUDMessages[ResourceType];
}

const FGameWarningInfo & URTSGameInstance::GetGameWarningInfo(EHousingResourceType HousingResourceType) const
{
	return MissingHousingResourceHUDMessages[HousingResourceType];
}

float URTSGameInstance::GetHUDGameMessageCooldown() const
{
	return HUDGameMessageCooldown;
}

EMarqueeBoxDrawMethod URTSGameInstance::GetMarqueeSelectionBoxStyle() const
{
	return MarqueeSelectionBoxStyle;
}

const FLinearColor & URTSGameInstance::GetMarqueeBoxRectangleFillColor() const
{
	return MarqueeBoxRectangleFillColor;
}

const FLinearColor & URTSGameInstance::GetMarqueeBoxBorderColor() const
{
	return MarqueeBoxBorderColor;
}

float URTSGameInstance::GetMarqueeBoxBorderLineThickness() const
{
	return MarqueeBoxBorderLineThickness;
}

float URTSGameInstance::GetBuildingStartHealthPercentage() const
{
	/* If repped floats lose decimal precision then this values editor min value may need to be
	increased. A zero might cause OnRep_Health to think building has been destroyed */

	assert(BuildingStartHealthPercentage > 0.f);
	return BuildingStartHealthPercentage;
}

EBuildingRallyPointDisplayRule URTSGameInstance::GetBuildingRallyPointDisplayRule() const
{
	return BuildingRallyPointDisplayRule;
}

const FName & URTSGameInstance::GetGhostBadLocationParamName() const
{
	return GhostBadLocationParamName;
}

const FLinearColor & URTSGameInstance::GetGhostBadLocationParamValue() const
{
	return GhostBadLocationParamValue;
}

UMaterialInterface * URTSGameInstance::GetFogOfWarMaterial() const
{
	return FogOfWarMaterial;
}

bool URTSGameInstance::UsingAtLeastOneUnifiedMouseFocusImage() const
{
	return UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedHoverImage
		|| UnifiedButtonAssetFlags_ActionBarETC.bUsingUnifiedPressedImage
		|| UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedHoverImage
		|| UnifiedButtonAssetFlags_InventoryItems.bUsingUnifiedPressedImage
		|| UnifiedButtonAssetFlags_Menus.bUsingUnifiedHoverImage
		|| UnifiedButtonAssetFlags_Menus.bUsingUnifiedPressedImage;
}

const FUnifiedImageAndSoundAssets & URTSGameInstance::GetUnifiedButtonAssets_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC;
}

const FUnifiedImageAndSoundAssets & URTSGameInstance::GetUnifiedButtonAssets_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems;
}

const FUnifiedImageAndSoundAssets & URTSGameInstance::GetUnifiedButtonAssets_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems;
}

const FUnifiedImageAndSoundAssets & URTSGameInstance::GetUnifiedButtonAssets_Menus() const
{
	return UnifiedImagesAndSounds_Menus;
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedHoverBrush_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMouseHoverImage();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedPressedBrush_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMousePressImage();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedHoverSound_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC.GetUnifiedMouseHoverSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedLMBPressedSound_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC.GetUnifiedPressedByLMBSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedRMBPressedSound_ActionBar() const
{
	return UnifiedImagesAndSounds_ActionBarETC.GetUnifiedPressedByRMBSound();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedHoverBrush_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverImage();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedPressedBrush_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMousePressImage();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedHoverSound_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedLMBPressedSound_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByLMBSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedRMBPressedSound_Inventory() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByRMBSound();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedHoverBrush_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverImage();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedPressedBrush_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMousePressImage();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedHoverSound_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedMouseHoverSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedLMBPressedSound_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByLMBSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedRMBPressedSound_Shop() const
{
	return UnifiedImagesAndSounds_InventoryItems.GetUnifiedPressedByRMBSound();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedHoverBrush_Menus() const
{
	return UnifiedImagesAndSounds_Menus.GetUnifiedMouseHoverImage();
}

const FUnifiedMouseFocusImage & URTSGameInstance::GetUnifiedPressedBrush_Menus() const
{
	return UnifiedImagesAndSounds_Menus.GetUnifiedMousePressImage();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedHoverSound_Menus() const
{
	return UnifiedImagesAndSounds_Menus.GetUnifiedMouseHoverSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedLMBPressedSound_Menus() const
{
	return UnifiedImagesAndSounds_Menus.GetUnifiedPressedByLMBSound();
}

const FUnifiedUIButtonSound & URTSGameInstance::GetUnifiedRMBPressedSound_Menus() const
{
	return UnifiedImagesAndSounds_Menus.GetUnifiedPressedByRMBSound();
}

FUnifiedImageAndSoundFlags URTSGameInstance::GetUnifiedButtonAssetFlags_ActionBar() const
{
	return UnifiedButtonAssetFlags_ActionBarETC;
}

FUnifiedImageAndSoundFlags URTSGameInstance::GetUnifiedButtonAssetFlags_InventoryItems() const
{
	return UnifiedButtonAssetFlags_InventoryItems;
}

FUnifiedImageAndSoundFlags URTSGameInstance::GetUnifiedButtonAssetFlags_Shop() const
{
	return UnifiedButtonAssetFlags_Shop;
}

FUnifiedImageAndSoundFlags URTSGameInstance::GetUnifiedButtonAssetFlags_Menus() const
{
	return UnifiedButtonAssetFlags_Menus;
}

UCurveFloat * URTSGameInstance::GetCamaraZoomToMoveSpeedCurve() const
{
	return CamaraZoomToMoveSpeedCurve;
}

UCurveFloat * URTSGameInstance::GetCameraMouseWheelZoomCurve() const
{
	return CameraMouseWheelZoomCurve;
}

UCurveFloat * URTSGameInstance::GetCameraResetCurve() const
{
	return CameraResetCurve;
}

const FKeyInfo & URTSGameInstance::GetInputKeyInfo(const FKey & Key) const
{
	UE_CLOG(KeyInfo.Contains(Key) == false, RTSLOG, Fatal, TEXT("No key info added for key %s"), *Key.ToString());

	return KeyInfo[Key];
}

const FSlateBrush & URTSGameInstance::KeyMappings_GetPlusSymbolImage() const
{
	return KeyMappings_PlusSymbol;
}

const FSlateBrush & URTSGameInstance::KeyMappings_GetCTRLModifierImage() const
{
	return KeyMappings_CTRLModifier;
}

const FSlateBrush & URTSGameInstance::KeyMappings_GetALTModifierImage() const
{
	return KeyMappings_ALTModifier;
}

const FSlateBrush & URTSGameInstance::KeyMappings_GetSHIFTModifierImage() const
{
	return KeyMappings_SHIFTModifier;
}

bool URTSGameInstance::ShouldStayOnMap(const AActor * Selectable) const
{
	/* GetClass() will likely not work. LeaveOnMapList contains C++ classes so blueprint 
	generated classes will give incorrect result. GetSuperClass() will probably work most 
	of the time but requires: 
	1. no blueprint childs of blueprints 
	2. all C++ child classes to be added to LeaveOnMapList 
	Because of these 2 reasons we need to check whole class hierarchy. */
	
	UClass * Curr = Selectable->GetClass();
	while (Curr != nullptr)
	{
		if (LeaveOnMapList.Contains(Curr))
		{	
			return true;
		}
		else
		{	
			Curr = Curr->GetSuperClass();
		}
	}

	return false;
}

int32 URTSGameInstance::GetLargestShopCatalogueSize() const
{
	return LargestShopCatalogueSize;
}

int32 URTSGameInstance::GetLargestInventoryCapacity() const
{
	return LargestInventoryCapacity;
}


//--------------------------------------------------------
//	Audio
//--------------------------------------------------------

void URTSGameInstance::SetupAudioComponents()
{
	//------------------------------------
	//	Music 
	//------------------------------------

	MusicAudioComp = NewObject<UAudioComponent>(this);
	MusicAudioComp->SetUISound(true);
	// What is spatialization?
	MusicAudioComp->bAllowSpatialization = false;
	/* So persists across map changes, probably happens anyway since comps outer is GI */
	MusicAudioComp->bIgnoreForFlushing = true;

	/* From source this appears to disable reverb, who knows since it's a 2D sound it probably
	doesn't have reverb anyway. I'm not even 100% what reverb is */
	MusicAudioComp->bIsMusic = true;

	//------------------------------------
	//	Player Selection 
	//------------------------------------

	PlayerSelectionAudioComp = NewObject<UAudioComponent>(this);
	PlayerSelectionAudioComp->SetUISound(true);
	PlayerSelectionAudioComp->bAllowSpatialization = false;

	//------------------------------------
	//	Player Orders 
	//------------------------------------

	PlayerCommandAudioComp = NewObject<UAudioComponent>(this);
	PlayerCommandAudioComp->SetUISound(true);
	PlayerCommandAudioComp->bAllowSpatialization = false;

	//------------------------------------
	//	Zero Health
	//------------------------------------

	ZeroHealthAudioComp = NewObject<UAudioComponent>(this);
	ZeroHealthAudioComp->SetUISound(true);
	ZeroHealthAudioComp->bAllowSpatialization = false;

	//------------------------------------
	//	Effects
	//------------------------------------

	EffectsAudioComp = NewObject<UAudioComponent>(this);
	EffectsAudioComp->SetUISound(true);
	EffectsAudioComp->bAllowSpatialization = false;
}

void URTSGameInstance::PlayMusic(USoundBase * InMusic)
{
	if (InMusic != nullptr)
	{
		MusicAudioComp->SetSound(InMusic);

		// Play if not playing already. Otherwise SetSound will play sound automatically
		if (!MusicAudioComp->IsPlaying())
		{
			MusicAudioComp->Play();
		}
	}
}

bool URTSGameInstance::IsPlayingPlayerSelectionSound() const
{
	return PlayerSelectionAudioComp->IsPlaying();
}

void URTSGameInstance::PlayPlayerSelectionSound(USoundBase * InSound)
{
	assert(InSound != nullptr);

	PlayerSelectionAudioComp->SetSound(InSound);

	if (!PlayerSelectionAudioComp->IsPlaying())
	{
		PlayerSelectionAudioComp->Play();
	}
}

bool URTSGameInstance::IsPlayingPlayerCommandSound() const
{
	return PlayerCommandAudioComp->IsPlaying();
}

void URTSGameInstance::PlayPlayerCommandSound(USoundBase * SoundToPlay)
{
	assert(SoundToPlay != nullptr);

	PlayerCommandAudioComp->SetSound(SoundToPlay);

	if (!PlayerCommandAudioComp->IsPlaying())
	{
		PlayerCommandAudioComp->Play();
	}
}

bool URTSGameInstance::IsPlayingZeroHealthSound() const
{
	return ZeroHealthAudioComp->IsPlaying();
}

void URTSGameInstance::PlayZeroHealthSound(USoundBase * SoundToPlay)
{
	assert(SoundToPlay != nullptr);

	ZeroHealthAudioComp->SetSound(SoundToPlay);

	if (!ZeroHealthAudioComp->IsPlaying())
	{
		ZeroHealthAudioComp->Play();
	}
}

void URTSGameInstance::PlayEffectSound(USoundBase * SoundToPlay)
{
	assert(SoundToPlay != nullptr);

	EffectsAudioComp->SetSound(SoundToPlay);

	if (!EffectsAudioComp->IsPlaying())
	{
		EffectsAudioComp->Play();
	}
}

USoundBase * URTSGameInstance::GetChatMessageReceivedSound(EMessageRecipients MessageRecipients) const
{
	return ChatMessageReceivedSounds[MessageRecipients];
}

#if WITH_EDITORONLY_DATA
bool URTSGameInstance::EditorPlaySession_ShouldSkipOpeningCutscene() const
{
	/* Note: I have not added this variable yet. I should, and assign a value to it some time 
	during setup. Might have to try do object iterator on its class using the editor world 
	see if that works */
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->ShouldSkipOpeningCutscene() : DevelopmentSettings->ShouldSkipOpeningCutscene();
}

bool URTSGameInstance::EditorPlaySession_ShouldSkipMainMenu() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->ShouldSkipMainMenu() : DevelopmentSettings->ShouldSkipMainMenu();
}

EDefeatCondition URTSGameInstance::EditorPlaySession_GetDefeatCondition() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetDefeatCondition() : DevelopmentSettings->GetDefeatCondition();
}

const TArray<FPIEPlayerInfo>& URTSGameInstance::EditorPlaySession_GetHumanPlayerInfo() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetHumanPlayerInfo() : DevelopmentSettings->GetHumanPlayerInfo();
}

int32 URTSGameInstance::EditorPlaySession_GetNumCPUPlayers() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetNumCPUPlayers() : DevelopmentSettings->GetNumCPUPlayers();
}

const TArray<FPIECPUPlayerInfo>& URTSGameInstance::EditorPlaySession_GetCPUPlayerInfo() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetCPUPlayerInfo() : DevelopmentSettings->GetCPUPlayerInfo();
}

EInvalidOwnerIndexAction URTSGameInstance::EditorPlaySession_GetInvalidHumanOwnerRule() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetInvalidHumanOwnerRule() : DevelopmentSettings->GetInvalidHumanOwnerRule();
}

EInvalidOwnerIndexAction URTSGameInstance::EditorPlaySession_GetInvalidCPUOwnerRule() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetInvalidCPUOwnerRule() : DevelopmentSettings->GetInvalidCPUOwnerRule();
}

bool URTSGameInstance::EditorPlaySession_IsCheatWidgetBPSet() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->IsCheatWidgetBPSet() : DevelopmentSettings->IsCheatWidgetBPSet();
}

bool URTSGameInstance::EditorPlaySession_ShouldInitiallyShowCheatWidget() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->ShouldInitiallyShowCheatWidget() : DevelopmentSettings->ShouldInitiallyShowCheatWidget();
}

TSubclassOf<UInMatchDeveloperWidget> URTSGameInstance::EditorPlaySession_GetCheatWidgetBP() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetCheatWidgetBP() : DevelopmentSettings->GetCheatWidgetBP();
}

bool URTSGameInstance::EditorPlaySession_ShowInfantryControllerDebugWidgets() const
{
	/* Just hardcoded TODO add stuff to ADevelopmentSettings and UEditorPlaySettingsWidget */
	return true;
}

#include "UI/InMatchWidgets/InfantryControllerDebugWidget.h"
TSubclassOf<UInfantryControllerDebugWidget> URTSGameInstance::EditorPlaySession_GetInfantryControllerDebugWidgetBP() const
{
	/* Just hardcoded TODO add stuff to ADevelopmentSettings and UEditorPlaySettingsWidget */
	return UInfantryControllerDebugWidget::StaticClass();
}

const FStartingResourceConfig & URTSGameInstance::EditorPlaySession_GetStartingResourceConfig() const
{
	return EditorPlaySettingsWidget != nullptr ? EditorPlaySettingsWidget->GetStartingResourceConfig() : DevelopmentSettings->GetStartingResourceConfig();
}
#endif
