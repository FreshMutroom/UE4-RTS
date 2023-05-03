// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"

#include "Statics/Structs/EditorPlaySessionStructs.h"
#include "Statics/Structs_2.h"
#include "DevelopmentSettings.generated.h" 

class UInMatchDeveloperWidget;


/**
 *	All these settings are for development only and have no effect on shipping build.
 *
 *	An alternative to using this class is to use UEditorPlaySettingsWidget
 */
UCLASS(Blueprintable)
class RTS_VER2_API ADevelopmentSettings : public AInfo
{
	GENERATED_BODY()

public:

	ADevelopmentSettings();

protected:

	/* Skip intro and main menu and jump into testing straight away
	When toggling this you will want to change one thing:
	1. You may want to change from your main menu map to match map or vice versa
	NOTE: when changing this PIE's advanced settings AutoConnectToServer will automatically be
	changed to bSkipMainMenu */
	/* NOTE: after changing the name of this variable make sure to change name in
	PostEditChangeChainProperty */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bSkipMainMenu;

	/* Skip just the opening intro movie/whatever and go straight to main menu */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bCanEditSkipIntro))
	bool bSkipIntro;

	/* If bSkipMainMenu is true, array of what team each player should be on. Can set
	observer here too
	Index 0 = server
	Index 1 = client 1
	Index 2 = client 2
	and so on */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (EditCondition = bSkipMainMenu))
	TArray<FPIEPlayerInfo> HumanPlayerConfiguration;

	/* Number of CPU players to use in PIE */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bSkipMainMenu, ClampMin = 0))
	int32 NumCPUPlayers;

	/* Info for CPU players when playing in PIE/standalone */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (EditCondition = bCanEditCPUPlayerInfo))
	TArray<FPIECPUPlayerInfo> CPUPlayerConfiguration;

	/* Resources all players start with */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "RTS", meta = (EditCondition = bSkipMainMenu))
	TMap<EResourceType, int32> StartingResources;

	/** 
	 *	Starting resources as a struct. Edited on post edit.
	 *	
	 *	Wow, this needed to be a UPROPERTY in order for the post edits to actually save. I did not 
	 *	know that 
	 */
	UPROPERTY()
	FStartingResourceConfig StartingResourcesConfig;

	/* Defeat condition to use if skipping main menu to simulate a match. Use NoCondition to
	make it impossible for match to end without leaving */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bSkipMainMenu))
	EDefeatCondition DefeatCondition;

	/* What to do with selectables already placed on map that do not have a valid owner index.
	Couple of points though:
	- selectables will never be spawned if their final owner is on the wrong faction
	- selectables will never be spawned if their final owner is a match observer */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bSkipMainMenu))
	EInvalidOwnerIndexAction InvalidHumanPlayerOwnerBehavior;

	/* Same as above but for selectables placed on map that are trying to be assigned to a
	CPU player as opposed to a human player */
	UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (EditCondition = bSkipMainMenu, DisplayName = "Invalid CPU Player Owner Behavior"))
	EInvalidOwnerIndexAction InvalidCPUPlayerOwnerBehavior;

	/* The widget to use for the cheat widget */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "RTS", meta = (DisplayName = "Cheat Widget"))
	TSubclassOf <UInMatchDeveloperWidget> CheatWidget_BP;

	/** 
	 *	Whether to show the cheat widget by default at the start of the match or not. 
	 *	
	 *	To toggle the cheat widget in match use the "=" button 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bInitiallyShowCheatWidget;

	TArray <FPIEPlayerInfo> InitHumanConfiguration() const;
	TArray <FPIECPUPlayerInfo> InitCPUConfiguration() const;
	TMap <EResourceType, int32> InitStartingResources() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bCanEditSkipIntro;

	UPROPERTY()
	bool bCanEditCPUPlayerInfo;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent & PropertyChangedEvent) override;
#endif

public:

	bool ShouldSkipMainMenu() const;
	bool ShouldSkipOpeningCutscene() const;

	//----------------------------------------------------------------------------
	//	PIE/standalone only
	//----------------------------------------------------------------------------

	// Number of CPU players to try and spawn for PIE
	int32 GetNumCPUPlayers() const;

	EInvalidOwnerIndexAction GetInvalidHumanOwnerRule() const;
	EInvalidOwnerIndexAction GetInvalidCPUOwnerRule() const;
	const TArray<FPIEPlayerInfo> & GetHumanPlayerInfo() const;
	const TArray<FPIECPUPlayerInfo> & GetCPUPlayerInfo() const;

	/* Get how much resources each player starts with. Only for bSkipMainMenu */
	const FStartingResourceConfig & GetStartingResourceConfig() const;

	/* Get the defeat condition to use for PIE match. Only for bSkipMainMenu */
	EDefeatCondition GetDefeatCondition() const;

	bool IsCheatWidgetBPSet() const;
	TSubclassOf<UInMatchDeveloperWidget> GetCheatWidgetBP() const;
	bool ShouldInitiallyShowCheatWidget() const;
};
