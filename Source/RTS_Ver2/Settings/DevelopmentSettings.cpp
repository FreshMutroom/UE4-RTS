// Fill out your copyright notice in the Description page of Project Settings.

#include "DevelopmentSettings.h"
#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#endif
#include "Statics/Structs_2.h" // For FStartingResourceConfig

#include "Statics/Statics.h"
#include "UI/MainMenu/Menus.h"
#include "Settings/ProjectSettings.h"
#include "UI/InMatchDeveloperWidget.h"


//----------------------------------------------------------------------
//	ADevelopmentSettings
//----------------------------------------------------------------------

ADevelopmentSettings::ADevelopmentSettings()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = false;

	bSkipMainMenu = false;
	bSkipIntro = false;

	HumanPlayerConfiguration = InitHumanConfiguration();
	CPUPlayerConfiguration = InitCPUConfiguration();
	StartingResources = InitStartingResources();
	DefeatCondition = EDefeatCondition::NoCondition;
	InvalidHumanPlayerOwnerBehavior = EInvalidOwnerIndexAction::AssignToServerPlayer;
	InvalidCPUPlayerOwnerBehavior = EInvalidOwnerIndexAction::DoNotSpawn;
	CheatWidget_BP = UInMatchDeveloperWidget::StaticClass();
	bInitiallyShowCheatWidget = false;

#if WITH_EDITORONLY_DATA
	bCanEditSkipIntro = !bSkipMainMenu;
#endif
}

TArray<FPIEPlayerInfo> ADevelopmentSettings::InitHumanConfiguration() const
{
	TArray < FPIEPlayerInfo > Array;
	Array.Reserve(ProjectSettings::MAX_NUM_PLAYERS);

	for (uint8 i = 0; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
	{
		// Alternate teams
		const ETeam Team = i % 2 ? ETeam::Team2 : ETeam::Team1;
		Array.Emplace(FPIEPlayerInfo(Team, EFaction::Monsters));
	}

	return Array;
}

TArray<FPIECPUPlayerInfo> ADevelopmentSettings::InitCPUConfiguration() const
{
	TArray < FPIECPUPlayerInfo > Array;
	Array.Reserve(ProjectSettings::MAX_NUM_PLAYERS);

	for (uint8 i = 0; i < ProjectSettings::MAX_NUM_PLAYERS; ++i)
	{
		// Alternate teams 
		const ETeam Team = i % 2 ? ETeam::Team2 : ETeam::Team1;
		Array.Emplace(FPIECPUPlayerInfo(Team, EFaction::Monsters, ECPUDifficulty::Easy));
	}

	return Array;
}

TMap<EResourceType, int32> ADevelopmentSettings::InitStartingResources() const
{
	TMap < EResourceType, int32 > ResourceMap;

	for (uint8 i = 0; i < Statics::NUM_RESOURCE_TYPES; ++i)
	{
		ResourceMap.Emplace(static_cast<EResourceType>(i + 1), 50000);
	}

	return ResourceMap;
}

#if WITH_EDITOR
void ADevelopmentSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent & PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == FName("bSkipMainMenu"))
	{
		bCanEditSkipIntro = !bSkipMainMenu;

		/* Change AutoConnectToServer in PIE settings as a convenience because it is assumed if
		skipping main menu then you want to not connect straight away and vice versa */

		ULevelEditorPlaySettings * Settings = const_cast<ULevelEditorPlaySettings*>(GetDefault<ULevelEditorPlaySettings>());
		UProperty * Property = Settings->GetClass()->FindPropertyByName(FName("AutoConnectToServer"));
		UBoolProperty * BoolProp = CastChecked<UBoolProperty>(Property);
		void * StructAddress = Property->ContainerPtrToValuePtr<void>(Settings);
		BoolProp->SetPropertyValue(StructAddress, bSkipMainMenu);
	}
	else if (PropertyChangedEvent.Property->GetFName() == FName("StartingResources"))
	{
		StartingResourcesConfig = FStartingResourceConfig(StartingResources);
	}

	bCanEditCPUPlayerInfo = bSkipMainMenu && NumCPUPlayers > 0;
}
#endif

bool ADevelopmentSettings::ShouldSkipMainMenu() const
{
	return bSkipMainMenu;
}

bool ADevelopmentSettings::ShouldSkipOpeningCutscene() const
{
	return bSkipIntro;
}

int32 ADevelopmentSettings::GetNumCPUPlayers() const
{
	return NumCPUPlayers;
}

EInvalidOwnerIndexAction ADevelopmentSettings::GetInvalidHumanOwnerRule() const
{
	return InvalidHumanPlayerOwnerBehavior;
}

EInvalidOwnerIndexAction ADevelopmentSettings::GetInvalidCPUOwnerRule() const
{
	return InvalidCPUPlayerOwnerBehavior;
}

const TArray<FPIEPlayerInfo>& ADevelopmentSettings::GetHumanPlayerInfo() const
{
	return HumanPlayerConfiguration;
}

const TArray<FPIECPUPlayerInfo>& ADevelopmentSettings::GetCPUPlayerInfo() const
{
	return CPUPlayerConfiguration;
}

const FStartingResourceConfig & ADevelopmentSettings::GetStartingResourceConfig() const
{
	return StartingResourcesConfig;
}

EDefeatCondition ADevelopmentSettings::GetDefeatCondition() const
{
	return DefeatCondition;
}

bool ADevelopmentSettings::IsCheatWidgetBPSet() const
{
	return CheatWidget_BP != nullptr;
}

TSubclassOf<UInMatchDeveloperWidget> ADevelopmentSettings::GetCheatWidgetBP() const
{
	return CheatWidget_BP;
}

bool ADevelopmentSettings::ShouldInitiallyShowCheatWidget() const
{
	return bInitiallyShowCheatWidget;
}


