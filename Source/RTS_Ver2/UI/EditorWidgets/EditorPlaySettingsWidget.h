// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"

#include "Statics/Structs/EditorPlaySessionStructs.h"
#include "Statics/Structs_2.h" // For FStartingResourceConfig
#include "EditorPlaySettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UWidget;
enum class EDefeatCondition : uint8;
enum class EEditorPlaySkippingOption : uint8;
class UEnumStringRepresentations;
class UFont;
class UInMatchDeveloperWidget;
enum class EInvalidOwnerIndexAction : uint8;
struct FStartingResourceConfig;
class UCanvasPanelSlot;
class UGridPanel;


/* Widget for adjusting a single starting resource */
UCLASS()
class RTS_VER2_API UEditorPlayStartingResourcesWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	//--------------------------------------------------------
	//	Data
	//--------------------------------------------------------

	/* Text that shows what starting resources will be used */
	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox * Text_Amount;
};


// TODO Automatically create this widget. Right now how it works is if you have this widget 
// created in editor and you do PIE/standalone then a UObject iterator will try find it. 
// If it finds it it will use the settings on it as oppossed to ADevelopmentSettings
// So yeah in future perhaps find a way to create this automatically for user and 
// bonus: let them specify the class 
// Also I have not created a default 'works out of the box' implementation for this widget.


/** 
 *	This is a UObject that just contains FPIEPlayerInfo. It exists so AddDynamic can point 
 *	to it. 
 */
UCLASS()
class RTS_VER2_API UPIEHumanPlayerInfoProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void UIBinding_OnTeamComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnFactionComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnStartingSpotTextChanged(const FText & Text);
	UFUNCTION()
	void UIBinding_OnStartingSpotTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);

	//--------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------

	const UEnumStringRepresentations * EnumStringObject;
	UEditorPlaySettingsWidget * PlaySettingsWidget;

	int32 PlayerIndex;

	FPIEPlayerInfo PlayerInfo;

	/* Widgets */
	UTextBlock * Text_PlayerIndex;
	UComboBoxString * ComboBox_Team;
	UComboBoxString * ComboBox_Faction;
	UEditableTextBox * Text_StartingSpot;
};


/** Same as human version except for CPU players */
UCLASS()
class RTS_VER2_API UPIECPUPlayerInfoProxy : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void UIBinding_OnTeamComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnFactionComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnStartingSpotTextChanged(const FText & Text);
	UFUNCTION()
	void UIBinding_OnStartingSpotTextCommitted(const FText & Text, ETextCommit::Type CommitMethod);
	UFUNCTION()
	void UIBinding_OnCPUDifficultyComboBoxSelectionChanged(FString Item, ESelectInfo::Type SelectionType);

	//--------------------------------------------------------------
	//	Data
	//--------------------------------------------------------------

	const UEnumStringRepresentations * EnumStringObject;
	UEditorPlaySettingsWidget * PlaySettingsWidget;

	int32 PlayerIndex;

	FPIECPUPlayerInfo PlayerInfo;

	/* Widgets */
	UTextBlock * Text_PlayerIndex;
	UComboBoxString * ComboBox_Team;
	UComboBoxString * ComboBox_Faction;
	UEditableTextBox * Text_StartingSpot;
	UComboBoxString * ComboBox_CPUDifficulty;
};


namespace PlayerInfoStatics
{
	static bool SetTeamFromString(const UEnumStringRepresentations * EnumStringObject, ETeam & OutTeam, const FString & String);
	static bool SetFactionFromString(const UEnumStringRepresentations * EnumStringObject, EFaction & OutFaction, const FString & String);
	static bool SetStartingSpotFromString(int16 & OutStartingSpot, const FString & String);
	static bool SetCPUDifficultyFromString(const UEnumStringRepresentations * EnumStringObject, ECPUDifficulty & OutDifficulty, const FString & String);
	static FString GetString_StartingSpot(int16 StartingSpot);
}


/**
 *	Editor utility widget that lets you adjust the PIE/standalone settings. 
 *	Not abstract - has a default 'works out of the box' implementation
 */
UCLASS()
class RTS_VER2_API UEditorPlaySettingsWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:

	UEditorPlaySettingsWidget(const FObjectInitializer & ObjectInitializer);

	virtual bool IsEditorOnly() const override final { return true; }

	/**
	 *	Functions to query what is set on the widget. The game instance/mode/whatever 
	 *	will probably want to call these during the start of PIE/standalone to know what to do 
	 */
	bool ShouldSkipOpeningCutscene() const;
	bool ShouldSkipMainMenu() const;
	EDefeatCondition GetDefeatCondition() const;
	const FStartingResourceConfig & GetStartingResourceConfig() const;
	bool IsCheatWidgetBPSet() const;
	bool ShouldInitiallyShowCheatWidget() const; 
	const TSubclassOf<UInMatchDeveloperWidget> & GetCheatWidgetBP() const;
	const TArray<FPIEPlayerInfo> & GetHumanPlayerInfo() const;
	int32 GetNumCPUPlayers() const;
	const TArray<FPIECPUPlayerInfo> & GetCPUPlayerInfo() const;
	EInvalidOwnerIndexAction GetInvalidHumanOwnerRule() const;
	EInvalidOwnerIndexAction GetInvalidCPUOwnerRule() const;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	/* I'm hoping this the begin play type function I want */
	virtual void NativePreConstruct() override;

	static bool IsWidgetBound(UWidget * Widget) { return Widget != nullptr; }

	/* Create an editor utility widget that will be a child of another editor utility widget */
	template <typename T>
	static T * CreateChildEditorUtilityWidget(UWorld * World, TSubclassOf<UEditorUtilityWidget> WidgetClass)
	{
		assert(World != nullptr);
		/* Cool, CreateWidget works */
		return CreateWidget<T>(World, WidgetClass);
	}

	/** 
	 *	These functions should create the widget, add it to the appropriate panel and 
	 *	call SetupAsHumanPlayerWidget/SetupAsCPUPlayerWidget on them. 
	 */
	void CreateSingleHumanPlayerConfigurationWidget(int32 PlayerIndex);
	void CreateSingleCPUPlayerConfigurationWidget(int32 PlayerIndex);

	/**
	 *	Get the size of some text. By size I mean like how many pixels it takes up or 
	 *	something. Have not tested whether this works
	 *	
	 * @param Font - The font used.
	 * @param ScaleX - Scale in X axis.
	 * @param ScaleY - Scale in Y axis.
	 * @param XL - out Horizontal length of string.
	 * @param YL - out Vertical length of string.
	 * @param Text - String to calculate for.
	 */
	static void GetTextSize(const UFont * Font, float ScaleX, float ScaleY, int32 & XL, int32 & YL, const TCHAR * Text);

	/* Functions to get the FText or FString to be displayed on UI */
	FText GetText(EEditorPlaySkippingOption InSkippingOption) const;
	FString GetString(EEditorPlaySkippingOption InSkippingOption) const;
	FText GetText(EDefeatCondition InDefeatCondition) const;
	FString GetString(EDefeatCondition InDefeatCondition) const;
	FText GetText(EInvalidOwnerIndexAction InRule) const;
	FString GetString(EInvalidOwnerIndexAction InRule) const;
	FString GetString(ETeam InTeam) const;
	FString GetString(EFaction InFaction) const;
	FString GetString_StartingSpot(int16 InStartingSpot) const;
	FString GetString(ECPUDifficulty InCPUDifficulty) const;

	/**
	 *	Sets a variable based on a string value. If the string does not corrispond to any value 
	 *	then nothing will happen
	 *
	 *	@return - true if a valid value is set using the string 
	 */
	bool SetSkippingOptionFromString(const FString & String);
	bool SetDefeatConditionFromString(const FString & String);
	bool SetNumCPUPlayersFromString(const FString & String);
	bool SetCheatWidgetBPFromString(const FString & String);
	bool SetInvalidHumanOwnerRuleFromString(const FString & String);
	bool SetInvalidCPUOwnerRuleFromString(const FString & String);

	/* Does like EnumValue-- on param in place */
	template <typename T> 
	static T DecrementEnum(T & EnumValue)
	{
		return static_cast<T>(static_cast<int32>(EnumValue) - 1);
	}

	/* Does like EnumValue++ on param in place */
	template <typename T>
	static T IncrementEnum(T & EnumValue)
	{
		return static_cast<T>(static_cast<int32>(EnumValue) + 1);
	}

	static FString CreateHumanReadableNameForUClassName(const FString & UClassGetNameResult);
	static FString CreateUClassNameFromHumanReadableName(const FString & HumanReadableName);

	static void SetSlotAnchors(UCanvasPanelSlot * Slot);
	static void SetSlotAlignment(UCanvasPanelSlot * Slot);

	UFUNCTION()
	void UIBinding_OnSkippingComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnAdjustSkippingOptionLeftButtonClicked();
	UFUNCTION()
	void UIBinding_OnAdjustSkippingOptionRightButtonClicked();
	UFUNCTION()
	void UIBinding_OnDefeatConditionComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnAdjustDefeatConditionLeftButtonClicked();
	UFUNCTION()
	void UIBinding_OnAdjustDefeatConditionRightButtonClicked();
	UFUNCTION()
	void UIBinding_OnNumCPUPlayersComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnDecreaseNumCPUPlayersButtonClicked();
	UFUNCTION()
	void UIBinding_OnIncreaseNumCPUPlayersButtonClicked();
	UFUNCTION()
	void UIBinding_OnShowCheatWidgetCheckBoxChanged(bool bIsChecked);
	UFUNCTION()
	void UIBinding_OnCheatWidgetComboBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnInvalidHumanOwnerRuleComboxBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void UIBinding_OnInvalidCPUOwnerRuleComboxBoxOptionChanged(FString Item, ESelectInfo::Type SelectionType);

	//-----------------------------------------------------------------------
	//=======================================================================
	//	------- Data -------
	//=======================================================================
	//-----------------------------------------------------------------------

	/* This object mappings from enum values to string representations */
	const UEnumStringRepresentations * EnumStringObject;

	//================================================================
	//	------- Skipping -------
	//================================================================

	/* What is currently set as the skipping option */
	EEditorPlaySkippingOption SkippingOption;

	/* Combo box to change what is skipped */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_Skipping;

	/* Text that shows what will be skipped */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_SkippingOption;

	/* Button that lets you decide what to skip */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AdjustSkippingLeft;

	/* Button that lets you decide what to skip */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AdjustSkippingRight;


	//================================================================
	//	------- Player Configuration -------
	//================================================================

	/* Teams, faction etc for human PIE players */
	UPROPERTY()
	TArray<UPIEHumanPlayerInfoProxy*> HumanPlayerInfo;
	
	/* Teams, faction etc for CPU PIE players */
	UPROPERTY()
	TArray<UPIECPUPlayerInfoProxy*> CPUPlayerInfo;

public:

	/* Copy of what's in HumanPlayerInfo and CPUPlayerInfo */
	TArray<FPIEPlayerInfo> HumanPlayerInfoCopy;
	TArray<FPIECPUPlayerInfo> CPUPlayerInfoCopy;

protected:

	/** How many CPU players to have in a PIE/standalone match */
	int32 NumCPUPlayers;

	/** Grid to show human players */
	UPROPERTY(meta = (BindWidgetOptional))
	UGridPanel * Grid_HumanPlayers;

	/** Grid to show CPU players */
	UPROPERTY(meta = (BindWidgetOptional))
	UGridPanel * Grid_CPUPlayers;

	/** Combo box to choose how many CPU players to play PIE/standalone with */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_NumCPUPlayers;

	/** Text that shows how many CPU players to play PIE/standalone with */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_NumCPUPlayers;

	/** Button to decrease how many players to play PIE/standalone with */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_DecreaseNumCPUPlayers;

	/** Button to increase how many players to play PIE/standalone with */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_IncreaseNumCPUPlayers;


	//================================================================
	//	------- Starting Resources Configuration -------
	//================================================================

	/* Starting resources array */
	TArray<int32> StartingResourceAmounts;
	
	/* Make sure to update this as resource amounts change */
	FStartingResourceConfig StartingResourceConfig;

	/** Panel that shows all the single starting resources widgets */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget * Panel_StartingResources;

	/** The widget for a single resource */
	UPROPERTY(EditAnywhere, Category = "RTS", meta = (DisplayName = "Single Starting Resource Widget"))
	TSubclassOf<UEditorPlayStartingResourcesWidget> SingleStartingResourceWidget_BP;


	//================================================================
	//	------- Defeat Condition -------
	//================================================================

	/** The currently set defeat condition */
	EDefeatCondition DefeatCondition;
	
	/** Drop down box to adjust defeat condition */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_DefeatCondition;

	/** Text that shows what the defeat condition is */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock * Text_DefeatCondition;

	/** Button to adjust left what the defeat condition is */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AdjustDefeatConditionLeft;

	/** Button to adjust right what the defeat condition is */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton * Button_AdjustDefeatConditionRight;


	//================================================================
	//	------- Cheat Widget -------
	//================================================================

	/** The currently set cheat widget option */
	bool bInitiallyShowCheatWidget;

	TSubclassOf<UInMatchDeveloperWidget> CheatWidget_BP;

	/** Toggles whether the in match cheat widget is shown by default when PIE/standalone starts */
	UPROPERTY(meta = (BindWidgetOptional))
	UCheckBox * CheckBox_CheatWidget;

	/** A drop down box to select the class for the cheat widget */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_CheatWidget;


	//================================================================
	//	------- Invalid Owner Widgets -------
	//================================================================

	/** 
	 *	Widget to choose the rule for when PIE/standalone starts and ther is a selectable placed 
	 *	on the map that cannot be assigned to it's owner e.g. owner is another faction 
	 */
	EInvalidOwnerIndexAction InvalidHumanOwnerRule;
	EInvalidOwnerIndexAction InvalidCPUOwnerRule;

	/** Drop down box to change the invalid human owner rule */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_InvalidHumanOwnerRule;

	/** Drop down box for invalid CPU owner */
	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString * ComboBox_InvalidCPUOwnerRule;
};
